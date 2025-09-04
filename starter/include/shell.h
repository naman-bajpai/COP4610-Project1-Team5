#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>

#define MAX_TOKENS   256
#define MAX_CMDS     3      // supports up to two pipes: cmd1 | cmd2 | cmd3
#define CMDLINE_MAX  2048
#define MAX_JOBS     32

typedef struct {
    char *argv[MAX_TOKENS];
    int   argc;
    char *in_file;          // or NULL
    char *out_file;         // or NULL
} Command;

typedef struct {
    Command cmd[MAX_CMDS];
    int     ncmd;
    int     background;     // &
} Pipeline;

typedef struct {
    int   job_no;
    pid_t pid;              // PID of the *last* process in pipeline
    int   active;           // 1 = running, 0 = finished
    char  cmdline[CMDLINE_MAX];
} Job;

#endif // SHELL_H
