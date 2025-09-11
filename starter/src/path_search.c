#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void search_path(char * command_name){
    char *path = getenv("PATH");
    char * token;
    token = strtok(path,":");

    while (token != NULL) {
        printf("Token: %s\n", token);
        token = strtok(NULL, ":");
    }
    // printf("command: %s\nlocation: %s\n",command_name,location);


}
