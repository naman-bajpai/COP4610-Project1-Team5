#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "prompt.h"
#include "path_search.h"
#include "external_command_execution.h"

void env_var_expansion(char ** token_ptr){
    const char *env_val = getenv(*token_ptr + 1);
    *token_ptr = strdup(env_val?env_val:"");
}

void tilde_expansion(char ** token_ptr){
    const char *home = getenv("HOME");
    *token_ptr = strdup(home?home:"");
}


int main(void) {
    char history[3][CMDLINE_MAX] = {{0}};
    int hist_count = 0;
    
    while (1) {
        // check for finished background jobs
        check_finished_jobs();
        
        printf("%s>",get_prompt());
        fflush(stdout);

        char *input = get_input();
        if (!input) break; 
        if (input[0] == '\0' && feof(stdin)) { 
            free(input);
            break;
        }

        char *file_in = NULL;
        char *file_out = NULL;
        int isBackgroundProcess = 0;
        tokenlist *tokens = get_tokens(input);
        for (int i = 0; i < (int)tokens->size; i++) {
            //enviornment variable expansion
            if (tokens->items[i][0]== '$'){
                env_var_expansion(&tokens->items[i]);
            }
            //tilde expansion
            if (tokens->items[i][0] == '~'){
                if (strlen(tokens->items[i])==1){
                    tilde_expansion(&tokens->items[i]);
                }
                else if (tokens->items[i][1]=='/'){
                    tilde_expansion(&tokens->items[i]);
                } 
            }
            //io redirection
            if (tokens->items[i][0] == '<'){
                 if (i + 1 < tokens->size) {
                    file_in = tokens->items[i + 1];
                    tokens->items[i] = NULL;
                    i++;
                } else {
                    fprintf(stderr, "No input file specified\n");
                }
            }
            else if (tokens->items[i][0] == '>') {
                 if (i + 1 < tokens->size) {
                    file_out = tokens->items[i + 1];
                    tokens->items[i] = NULL;
                    i++;
                } else {
                    fprintf(stderr, "No output file specified\n");
                }
            }
            //background processing
            if (tokens->items[i][0] == '&'){
                isBackgroundProcess = 1;
                tokens->items[i] = NULL;
            }
            // printf("token: (%s)\n",tokens->items[i]);
        }
        // Check if it's a built-in command
        if (is_builtin_command(tokens->items[0])) {
            execute_builtin_command(tokens->items, tokens->size, history, hist_count);
            add_to_history(history, &hist_count, input);
        } else {
            //path search
            char *command_path = search_path(tokens->items[0]); 
            if (command_path) {
                // printf("Found command: %s at: %s\n", tokens->items[0], command_path);
                if (file_in || file_out){
                    run_command_with_redirection(command_path,tokens->items,file_in,file_out, isBackgroundProcess);
                }
                else{
                    run_command(command_path, tokens->items, isBackgroundProcess);
                }
                free(command_path);
                add_to_history(history, &hist_count, input);
                isBackgroundProcess = 0;
            } else {
                printf("command not found: %s\n", tokens->items[0]);
            }
        }

        free(input);
        free_tokens(tokens);
    }
    return 0;
}
