#define _POSIX_C_SOURCE 200809L 
#define _XOPEN_SOURCE 700  

#include "lexer.h"
#include "shell.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


static void free_token_array(char **tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
}

static int is_var_name_char(char c) {
    return (c=='_') || (c>='0' && c<='9') || (c>='A' && c<='Z') || (c>='a' && c<='z');
}

static char *expand_token(const char *tok) {
    if (!tok || !*tok) return strdup(tok ? tok : "");

    if (tok[0] == '~' && (tok[1] == '\0' || tok[1] == '/')) {
        const char *home = getenv("HOME");
        if (!home) home = "";
        size_t hl = strlen(home);
        size_t tl = strlen(tok);
        size_t outl = hl + (tok[1] ? (tl - 1) : 0);
        char *out = (char*)malloc(outl + 1);
        memcpy(out, home, hl);
        if (tok[1]) memcpy(out + hl, tok + 1, tl - 1);
        out[outl] = '\0';
        return out;
    }

    if (tok[0] == '$' && tok[1] != '\0') {
        for (int i = 1; tok[i]; i++) {
            if (!is_var_name_char(tok[i])) {
                return strdup(tok);
            }
        }
        const char *val = getenv(tok + 1);
        return strdup(val ? val : "");
    }

    return strdup(tok);
}

static void print_prompt(void) {
    const char *user = getenv("USER");
    const char *pwd  = getenv("PWD");
    char host[256];
    if (gethostname(host, sizeof(host)) != 0) strncpy(host, "machine", sizeof(host));
    if (!user) user = "user";
    if (!pwd)  pwd  = "";
    printf("%s@%s:%s> ", user, host, pwd);
    fflush(stdout);
}

static int resolve_executable(const char *cmd, char *out, size_t out_sz) {
    if (!cmd || !*cmd) return -1;

    if (strchr(cmd, '/')) {
        if (access(cmd, X_OK) == 0) {
            strncpy(out, cmd, out_sz - 1);
            out[out_sz - 1] = '\0';
            return 0;
        }
        return -1;
    }

    const char *path = getenv("PATH");
    if (!path) path = "/bin:/usr/bin";

char *paths = strdup(path);
for (char *dir = strtok(paths, ":"); dir; dir = strtok(NULL, ":")) {
    size_t need = strlen(dir) + 1 + strlen(cmd) + 1;
    if (need > out_sz) continue;
    snprintf(out, out_sz, "%s/%s", dir, cmd);
    if (access(out, X_OK) == 0) {
        free(paths);
        return 0;
    }
}
free(paths);
return -1;
}


static void init_pipeline(Pipeline *p) {
    memset(p, 0, sizeof(*p));
    p->ncmd = 1;
}

static int parse_tokens_to_pipeline(char **toks, int ntok, Pipeline *p) {
    init_pipeline(p);
    Command *cur = &p->cmd[0];

    for (int i = 0; i < ntok; i++) {
        char *t = toks[i];

        if (strcmp(t, "|") == 0) {
            if (p->ncmd >= MAX_CMDS) {
                fprintf(stderr, "error: too many pipes\n");
                return -1;
            }
            p->ncmd++;
            cur = &p->cmd[p->ncmd - 1];
            continue;
        }
        if (strcmp(t, "<") == 0) {
            if (i + 1 >= ntok) {
                fprintf(stderr, "error: missing input file\n");
                return -1;
            }
            cur->in_file = toks[++i];
            continue;
        }
        if (strcmp(t, ">") == 0) {
            if (i + 1 >= ntok) {
                fprintf(stderr, "error: missing output file\n");
                return -1;
            }
            cur->out_file = toks[++i];
            continue;
        }
        if (strcmp(t, "&") == 0) {
            p->background = 1;
            continue;
        }
        cur->argv[cur->argc++] = t;
    }

    for (int c = 0; c < p->ncmd; c++) {
        p->cmd[c].argv[p->cmd[c].argc] = NULL;
    }
    return 0;
}


static Job jobs[MAX_JOBS];
static int  next_job_no = 1;

static void add_job(pid_t pid, const char *cmdline) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].active = 1;
            jobs[i].pid = pid;
            jobs[i].job_no = next_job_no++;
            strncpy(jobs[i].cmdline, cmdline ? cmdline : "", CMDLINE_MAX - 1);
            jobs[i].cmdline[CMDLINE_MAX - 1] = '\0';
            printf("[%d] %d\n", jobs[i].job_no, (int)pid);
            fflush(stdout);
            return;
        }
    }
    fprintf(stderr, "warning: job table full\n");
}

static void reap_finished_jobs(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].active && jobs[i].pid == pid) {
                jobs[i].active = 0;
                printf("[%d] + done %s\n", jobs[i].job_no, jobs[i].cmdline);
                fflush(stdout);
                break;
            }
        }
    }
}

static void print_jobs(void) {
    int any = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            any = 1;
            printf("[%d]+ %d %s\n", jobs[i].job_no, (int)jobs[i].pid,
                   jobs[i].cmdline);
        }
    }
    if (!any) {
        printf("no active background processes\n");
    }
}


static int is_builtin(const Pipeline *p) {
    if (p->ncmd != 1 || p->background) return 0;
    Command const *c = &p->cmd[0];
    if (c->argc == 0) return 0;
    return (strcmp(c->argv[0], "exit") == 0) ||
           (strcmp(c->argv[0], "cd") == 0)   ||
           (strcmp(c->argv[0], "jobs") == 0);
}

static int run_builtin(Pipeline *p, char history[][CMDLINE_MAX], int hist_n) {
    Command *c = &p->cmd[0];
    const char *name = c->argv[0];

    if (strcmp(name, "cd") == 0) {
        if (c->argc > 2) {
            fprintf(stderr, "cd: too many arguments\n");
            return 0;
        }
        const char *target = (c->argc == 1) ? getenv("HOME") : c->argv[1];
        if (!target) target = "";
        if (chdir(target) != 0) {
            perror("cd");
            return -1;
        }
        char buf[PATH_MAX];
        if (getcwd(buf, sizeof(buf))) setenv("PWD", buf, 1);
        return 0;
    }

    if (strcmp(name, "jobs") == 0) {
        print_jobs();
        return 0;
    }

    if (strcmp(name, "exit") == 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].active) {
                waitpid(jobs[i].pid, NULL, 0);
                jobs[i].active = 0;
            }
        }
        if (hist_n == 0) {
            printf("no valid commands in history\n");
        } else {
            int count = hist_n < 3 ? hist_n : 3;
            for (int i = 0; i < count; i++) {
                printf("%s\n", history[(hist_n - 1 - i) % 3]);
            }
        }
        exit(0);
    }
    return 0;
}


static int open_input(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        perror("input file");
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "error: input is not a regular file\n");
        return -1;
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0) perror("open input");
    return fd;
}

static int open_output(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) perror("open output");
    return fd;
}

static int run_pipeline(Pipeline *p, const char *cmdline) {
    int n = p->ncmd;
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];
    memset(pids, 0, sizeof(pids));

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i]) != 0) {
            perror("pipe");
            return -1;
        }
    }

    for (int i = 0; i < n; i++) {
        int in_fd = -1, out_fd = -1;
        if (p->cmd[i].in_file) {
            in_fd = open_input(p->cmd[i].in_file);
            if (in_fd < 0) return -1;
        }
        if (p->cmd[i].out_file) {
            out_fd = open_output(p->cmd[i].out_file);
            if (out_fd < 0) return -1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        }
        if (pid == 0) {
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    perror("dup2 in");
                    _exit(127);
                }
            }
            if (in_fd >= 0) {
                if (dup2(in_fd, STDIN_FILENO) < 0) {
                    perror("dup2 in file");
                    _exit(127);
                }
            }
            if (i < n - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 out");
                    _exit(127);
                }
            }
            if (out_fd >= 0) {
                if (dup2(out_fd, STDOUT_FILENO) < 0) {
                    perror("dup2 out file");
                    _exit(127);
                }
            }

            for (int k = 0; k < n - 1; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }
            if (in_fd  >= 0) close(in_fd);
            if (out_fd >= 0) close(out_fd);

            char path[PATH_MAX];
            if (resolve_executable(p->cmd[i].argv[0], path, sizeof(path)) != 0) {
                fprintf(stderr, "command not found: %s\n", p->cmd[i].argv[0]);
                _exit(127);
            }

            execv(path, p->cmd[i].argv);
            perror("execv");
            _exit(127);
        } else {
            pids[i] = pid;
            if (in_fd  >= 0) close(in_fd);
            if (out_fd >= 0) close(out_fd);
        }
    }

    for (int i = 0; i < n - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (p->background) {
        add_job(pids[n - 1], cmdline);
        return 0;
    }

    for (int i = 0; i < n; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) perror("waitpid");
    }
    return 0;
}


#ifdef ReferenceTest
int main(void) {
    struct sigaction sa = {0};
    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);

    char *line = NULL;
    size_t cap = 0;
    char history[3][CMDLINE_MAX] = {{0}};
    int hist_n = 0;

    for (;;) {
        reap_finished_jobs();
        print_prompt();

        ssize_t nread = getline(&line, &cap, stdin);
        if (nread < 0) {
            Pipeline dummy = {0};
            Command c = {0};
            dummy.cmd[0] = c;
            dummy.ncmd = 1;
            char *argv[] = {"exit", NULL};
            dummy.cmd[0].argv[0] = argv[0];
            dummy.cmd[0].argc = 1;
            run_builtin(&dummy, history, hist_n);
            break;
        }
        if (nread == 0) continue;

        // strip trailing newline
        if (nread > 0 && line[nread - 1] == '\n') line[nread - 1] = '\0';
        if (line[0] == '\0') continue;

        // Tokenize
        char *raw[MAX_TOKENS] = {0};
        int ntok = tokenize(line, raw, MAX_TOKENS);
        if (ntok <= 0) continue;

        // Expand tokens
        char *toks[MAX_TOKENS] = {0};
        for (int i = 0; i < ntok; i++) toks[i] = expand_token(raw[i]);
        free_token_array(raw, ntok);

        // Parse -> Pipeline
        Pipeline p;
        if (parse_tokens_to_pipeline(toks, ntok, &p) != 0) {
            free_token_array(toks, ntok);
            continue;
        }

        // Quick handle built-ins
        if (is_builtin(&p)) {
            run_builtin(&p, history, hist_n);
            // record into history
            strncpy(history[hist_n % 3], line, CMDLINE_MAX - 1);
            history[hist_n % 3][CMDLINE_MAX - 1] = '\0';
            hist_n++;
            free_token_array(toks, ntok);
            continue;
        }

        // Require command present
        if (p.cmd[0].argc == 0) {
            free_token_array(toks, ntok);
            continue;
        }

        // Execute
        if (run_pipeline(&p, line) == 0) {
            // record into history as a "valid" command
            strncpy(history[hist_n % 3], line, CMDLINE_MAX - 1);
            history[hist_n % 3][CMDLINE_MAX - 1] = '\0';
            hist_n++;
        }

        free_token_array(toks, ntok);
    }

    free(line);
    return 0;
}
#endif
