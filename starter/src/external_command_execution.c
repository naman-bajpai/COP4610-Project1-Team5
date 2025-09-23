#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "external_command_execution.h"

//global job tracking
static Job jobs[MAX_JOBS];
static int next_job_no = 1;

static pid_t g_last_child_pid = -1;
pid_t last_spawned_pid(void) { return g_last_child_pid; }

// Safe append helper
static void safe_cat(char *dst, size_t cap, const char *src) {
    size_t dlen = strlen(dst);
    if (dlen >= cap) return;
    size_t r = cap - 1 - dlen;
    if (r == 0) return;
    strncat(dst, src, r);
}

//helper function to add a job to the job list
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

//helper function to build command line string
static void build_cmdline(char *cmdline, char *const argv[]) { 
    cmdline[0] = '\0';
    for (int i = 0; argv[i] != NULL; i++) { 
        if (i > 0) strcat(cmdline, " ");
        strcat(cmdline, argv[i]);
    }
}

void run_command(const char *command_path, char *const argv[], int isBackgroundProcess){
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return;
    }
    if (pid == 0) {
        execv(command_path, argv);
        perror("execv failed");
        exit(EXIT_FAILURE);
    }
    else{
        if (isBackgroundProcess){
            char cmdline[CMDLINE_MAX];
            build_cmdline(cmdline, argv);
            add_job(pid, cmdline);
        }
        else{
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid failed");
            } else if (WIFEXITED(status)) {
                //child exited normally
            }
        }
    }
}

void run_command_with_redirection(char* command_path, char *const argv[], char *file_in, char *file_out, int isBackgroundProcess) {
    pid_t pid = fork();
    //make sure fork is successful
    if (pid == -1) {
        perror("fork failed");
        return;
    }
    //check if child or parent   
    if (pid == 0) {//child
        if (file_in) {
            int fd_in = open(file_in, O_RDONLY);
            if (fd_in < 0) {
                perror("open input failed");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (file_out) {
            int fd_out = open(file_out,
                              O_WRONLY | O_CREAT | O_TRUNC,
                              S_IRUSR | S_IWUSR);
            if (fd_out < 0) {
                perror("open output failed");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        execvp(command_path, argv);
        perror("exec failed");
        exit(1);
    } else {//parent
        if (isBackgroundProcess){
            char cmdline[CMDLINE_MAX];
            build_cmdline(cmdline, argv);
            //add redirection info to command line     
            if (file_in) {
                strcat(cmdline, " < ");
                strcat(cmdline, file_in);
            }
            if (file_out) {
                strcat(cmdline, " > ");
                strcat(cmdline, file_out);
            }
            add_job(pid, cmdline);
        }
        else{
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

//check for finished background jobs   
void check_finished_jobs(void) {
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

//print list of active background jobs     
void print_jobs(void) {
    int any = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            any = 1;
            printf("[%d]+ %d %s\n", jobs[i].job_no, (int)jobs[i].pid, jobs[i].cmdline);
        }
    }
    if (!any) {
        printf("no active background processes\n");
    }
}

// Check if command is a built-in command   
int is_builtin_command(const char *command) {
    if (!command) return 0;
    return (strcmp(command, "exit") == 0) ||
           (strcmp(command, "cd") == 0) ||
           (strcmp(command, "jobs") == 0);
}

//execute built-in commands
int execute_builtin_command(char **argv, int argc, char history[][CMDLINE_MAX], int hist_count) {
    if (argc == 0) return 0;
    
    const char *command = argv[0];
    
    if (strcmp(command, "cd") == 0) {
        if (argc > 2) {
            fprintf(stderr, "cd: too many arguments\n");
            return -1;
        }
        
        const char *target = (argc == 1) ? getenv("HOME") : argv[1];
        if (!target) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
        
        if (chdir(target) != 0) {
            perror("cd");
            return -1;
        }
        
        // Update PWD environment variable
        char buf[4096];
        if (getcwd(buf, sizeof(buf))) {
            setenv("PWD", buf, 1);
        }
        return 0;
    }
    
    if (strcmp(command, "jobs") == 0) {
        print_jobs();
        return 0;
    }
    
    if (strcmp(command, "exit") == 0) {
        //wait for all background processes to finish
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].active) {
                waitpid(jobs[i].pid, NULL, 0);
                jobs[i].active = 0;
            }
        }
        
        //display command history
        if (hist_count == 0) {
            printf("no valid commands in history\n");
        } else {
            int count = hist_count < 3 ? hist_count : 3;
            for (int i = 0; i < count; i++) {
                printf("%s\n", history[(hist_count - 1 - i) % 3]);
            }
        }
        exit(0);
    }
    
    return 0;
}

//add command to history
void add_to_history(char history[][CMDLINE_MAX], int *hist_count, const char *cmdline) {
    if (!cmdline) return;
    
    strncpy(history[*hist_count % 3], cmdline, CMDLINE_MAX - 1);
    history[*hist_count % 3][CMDLINE_MAX - 1] = '\0';
    (*hist_count)++;
}