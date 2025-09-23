#pragma once
#ifndef EXTERNAL_COMMAND_EXECUTION_H
#define EXTERNAL_COMMAND_EXECUTION_H

#include <sys/types.h>   // for pid_t

#define MAX_JOBS     10
#define CMDLINE_MAX  256   // keep consistent across the project

typedef struct {
    int job_no;
    pid_t pid;
    int active;  // 1 = running, 0 = finished
    char cmdline[CMDLINE_MAX];
} Job;

void run_command(const char *command_path, char *const argv[], int isBackgroundProcess);
void run_command_with_redirection(char* command_path, char *const argv[], char *file_in, char *file_out, int isBackgroundProcess);
void check_finished_jobs(void);
void print_jobs(void);
int is_builtin_command(const char *command);
int execute_builtin_command(char **argv, int argc, char history[][CMDLINE_MAX], int hist_count);
void add_to_history(char history[][CMDLINE_MAX], int *hist_count, const char *cmdline);
pid_t last_spawned_pid(void);

#endif
