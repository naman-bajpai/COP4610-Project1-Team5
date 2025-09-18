#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

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
        int status;
        if (isBackgroundProcess){
            // printf("not going to wait\n");
        }
        else{
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid failed");
            } else if (WIFEXITED(status)) {
                // printf("Child exited with status %d\n", WEXITSTATUS(status));
            }

        }
    }

}

void run_command_with_redirection(char* command_path, char *const argv[], char *file_in, char *file_out, int isBackgroundProcess) {
    pid_t pid = fork();
    // make sure fork is successful
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
        int status;
        if (isBackgroundProcess){
            // printf("not going to wait\n");
        }
        else{

            waitpid(pid, &status, 0);
        }
    }
}
