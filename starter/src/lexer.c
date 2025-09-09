#include "lexer.h"
#include "prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read a whole line from stdin into a dynamically-grown buffer.
 * The returned buffer is NUL-terminated and has NO trailing newline.
 * Caller must free() the returned pointer.
 */
char *get_input(void) {
    char *buffer = NULL;
    int bufsize = 0;

    /* Small chunked read to avoid getline() portability issues. */
    char line[5];
    while (fgets(line, (int)sizeof(line), stdin) != NULL) {
        int addby = 0;
        char *newln = strchr(line, '\n');
        if (newln != NULL) {
            addby = (int)(newln - line);
        } else {
            addby = (int)sizeof(line) - 1;
        }

        char *tmp = (char *)realloc(buffer, (size_t)bufsize + (size_t)addby);
        if (!tmp) {
            free(buffer);
            return NULL;
        }
        buffer = tmp;

        memcpy(&buffer[bufsize], line, (size_t)addby);
        bufsize += addby;

        if (newln != NULL) break; /* stop at newline */
    }

    /* EOF with no data: return empty string for convenience */
    if (buffer == NULL && feof(stdin)) {
        buffer = (char *)malloc(1);
        if (buffer) buffer[0] = '\0';
        return buffer;
    }

    /* NUL-terminate */
    char *tmp = (char *)realloc(buffer, (size_t)bufsize + 1u);
    if (!tmp) {
        free(buffer);
        return NULL;
    }
    buffer = tmp;
    buffer[bufsize] = '\0';
    return buffer;
}

/* Create an empty tokenlist. */
tokenlist *new_tokenlist(void) {
    tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
    if (!tokens) return NULL;

    tokens->size = 0;
    tokens->items = (char **)malloc(sizeof(char *));
    if (!tokens->items) {
        free(tokens);
        return NULL;
    }
    tokens->items[0] = NULL; /* make NULL-terminated */
    return tokens;
}

/* Append a copy of 'item' to tokens->items. */
void add_token(tokenlist *tokens, char *item) {
    if (!tokens || !item) return;

    int i = (int)tokens->size;
    char **new_items = (char **)realloc(tokens->items, (size_t)(i + 2) * sizeof(char *));
    if (!new_items) return;
    tokens->items = new_items;

    tokens->items[i] = (char *)malloc(strlen(item) + 1u);
    if (!tokens->items[i]) return;

    strcpy(tokens->items[i], item);
    tokens->items[i + 1] = NULL; /* keep NULL-terminated */

    tokens->size += 1u;
}

/* Tokenize by whitespace (space, tab, CR, NL). */
tokenlist *get_tokens(char *input) {
    if (!input) return new_tokenlist();

    char *buf = (char *)malloc(strlen(input) + 1u);
    if (!buf) return new_tokenlist();
    strcpy(buf, input);

    tokenlist *tokens = new_tokenlist();
    if (!tokens) {
        free(buf);
        return NULL;
    }

    /* Split on any whitespace (more robust than just space). */
    char *tok = strtok(buf, " \t\r\n");
    while (tok != NULL) {
        add_token(tokens, tok);
        tok = strtok(NULL, " \t\r\n");
    }

    free(buf);
    return tokens;
}

/* Free all token strings and the tokenlist itself. */
void free_tokens(tokenlist *tokens) {
    if (!tokens) return;
    for (int i = 0; i < (int)tokens->size; i++) {
        free(tokens->items[i]);
    }
    free(tokens->items);
    free(tokens);
}

/* Tokenize input into an array of strings.
 * Returns the number of tokens found, or -1 on error.
 * The tokens array will be populated with heap-allocated strings.
 */
int tokenize(const char *input, char **tokens, int max_tokens) {
    if (!input || !tokens || max_tokens <= 0) return -1;
    
    char *buf = (char *)malloc(strlen(input) + 1u);
    if (!buf) return -1;
    strcpy(buf, input);
    
    int count = 0;
    char *tok = strtok(buf, " \t\r\n");
    while (tok != NULL && count < max_tokens) {
        tokens[count] = (char *)malloc(strlen(tok) + 1u);
        if (!tokens[count]) {
            // Clean up already allocated tokens
            for (int i = 0; i < count; i++) {
                free(tokens[i]);
                tokens[i] = NULL;
            }
            free(buf);
            return -1;
        }
        strcpy(tokens[count], tok);
        count++;
        tok = strtok(NULL, " \t\r\n");
    }
    
    free(buf);
    return count;
}

#ifdef LEXER_TEST
/* Simple interactive test harness for the lexer.
 * Build with: gcc -DLEXER_TEST -o bin/lexer_test src/lexer.c
 */
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
#endif
