#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

char* search_path(char *command_name) {
    if (strchr(command_name, '/')) {
        if (access(command_name, X_OK) == 0) {
            return strdup(command_name);
        }
        return NULL; //if not executable, return NULL
    }
    char *path = getenv("PATH");
    if (!path) {
        path = "/bin:/usr/bin";
    }

    char *path_copy = strdup(path);
    if (!path_copy) {
        return NULL;
    }
     
    char *directory = strtok(path_copy, ":"); //split path b colon to get each directory
    char *full_path = malloc(PATH_MAX); //allocate memory for full path
    if (!full_path) {
        free(path_copy);
        return NULL;
    }
    
    while (directory != NULL) {
        snprintf(full_path, PATH_MAX, "%s/%s", directory, command_name); 
        
        if (access(full_path, X_OK) == 0) {
            free(path_copy);  
            return full_path;
        }
        
        directory = strtok(NULL, ":"); //get next directory
    }
    
    free(path_copy);
    free(full_path);
    return NULL;
}
