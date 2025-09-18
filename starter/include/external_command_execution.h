#pragma once

#ifndef EXTERNAL_COMMAND_EXECUTION_H
#define EXTERNAL_COMMAND_EXECUTION_H 
void run_command(const char *command_path, char *const argv[], int isBackgroundProcess);
void run_command_with_redirection(char* command_path, char *const argv[], char *file_in, char *file_out, int isBackgroundProcess);
#endif
