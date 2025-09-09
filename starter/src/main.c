#include "lexer.h"
#include "prompt.h"
#include <stdio.h>
int main(void) {
    while (1) {
        printf("%s>",get_prompt());
        fflush(stdout);

        char *input = get_input();
        if (!input) break;        /* allocation failure */
        if (input[0] == '\0' && feof(stdin)) { /* EOF */
            free(input);
            break;
        }

        printf("whole input: %s\n", input);

        tokenlist *tokens = get_tokens(input);
        for (int i = 0; i < (int)tokens->size; i++) {
            printf("token %d: (%s)\n", i, tokens->items[i]);
        }

        free(input);
        free_tokens(tokens);
    }
    return 0;
}
