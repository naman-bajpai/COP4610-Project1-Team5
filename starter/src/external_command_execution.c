#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void run_command(const char *command_path, char *const argv[]){
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
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
        } else if (WIFEXITED(status)) {
            // printf("Child exited with status %d\n", WEXITSTATUS(status));
        }
    }

}

