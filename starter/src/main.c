#define _POSIX_C_SOURCE 200809L
#include "lexer.h"
#include "prompt.h"
#include <stdio.h>
#include <string.h>

void env_var_expansion(char ** token_ptr){
    const char *env_val = getenv(*token_ptr + 1);
    *token_ptr = strdup(env_val?env_val:"");
}

void tilde_expansion(char ** token_ptr){
    const char *home = getenv("HOME");
    *token_ptr = strdup(home?home:"");
}

int main(void) {
    while (1) {
        printf("%s>",get_prompt());
        fflush(stdout);

        char *input = get_input();
        if (!input) break;
        if (input[0] == '\0' && feof(stdin)) { 
            free(input);
            break;
        }

        printf("whole input: %s\n", input);

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

        }

        free(input);
        free_tokens(tokens);
    }
    return 0;
}
