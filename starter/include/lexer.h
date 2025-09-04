#pragma once

#include <stdlib.h>
#include <stdbool.h>

/* A simple NULL-terminated token array with a size counter. */
typedef struct {
    char **items;  /* items[size] is guaranteed to be NULL */
    size_t size;   /* number of actual tokens (not counting the NULL) */
} tokenlist;

/* Reads a full line from stdin (without the trailing '\n').
 * Returns a heap-allocated C-string you must free(), or an empty string on EOF.
 */
char *get_input(void);

/* Splits input into tokens using whitespace as delimiters.
 * Returns a heap-allocated tokenlist; free with free_tokens().
 *
 * Note: This tokenizer does NOT split special characters like < > | &.
 * If you need those as standalone tokens, either: (a) ensure the user
 * puts spaces around them, or (b) post-process the tokens in your shell.
 */
tokenlist *get_tokens(char *input);

/* Allocate a new empty tokenlist. */
tokenlist *new_tokenlist(void);

/* Append a token (makes an owned copy). */
void add_token(tokenlist *tokens, char *item);

/* Free all memory owned by the tokenlist (including token strings). */
void free_tokens(tokenlist *tokens);

/* Tokenize input into an array of strings.
 * Returns the number of tokens found, or -1 on error.
 * The tokens array will be populated with heap-allocated strings.
 */
int tokenize(const char *input, char **tokens, int max_tokens);
