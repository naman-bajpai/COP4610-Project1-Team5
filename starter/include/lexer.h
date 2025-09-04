#pragma once

#include <stdlib.h>
#include <stdbool.h>

/* A simple NULL-terminated token array with a size counter. */
typedef struct {
    char **items;  /* items[size] is guaranteed to be NULL */
    size_t size;   /* number of actual tokens (not counting the NULL) */
} tokenlist;

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
int tokenize(const char *input, char **tokens, int max_tokens);
