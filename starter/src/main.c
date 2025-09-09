#define _POSIX_C_SOURCE 200809L
#include "lexer.h"
#include "prompt.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    while (1) {
        printf("%s>",get_prompt());
        fflush(stdout);

        char *input = get_input();
        if (!input) break;        
        if (input[0] == '\0' && feof(stdin)) { /* EOF */
            free(input);
            break;
        }

        printf("whole input: %s\n", input);

        tokenlist *tokens = get_tokens(input);
        for (int i = 0; i < (int)tokens->size; i++) {
            if (tokens->items[i][0]== '$'){
                const char *env_val = getenv(tokens->items[i] + 1);
                if (env_val){
                    free(tokens->items[i]);
                    tokens->items[i] = strdup(env_val);
                }
                else {
                    free(tokens->items[i]);
                    tokens->items[i] = strdup(""); 
                }
                printf("token %d: (%s)\n", i, tokens->items[i]);

            }
        }

        free(input);
        free_tokens(tokens);
    }
    return 0;
}
