#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef USER_MAX
#define USER_MAX 256
#endif
#ifndef MACHINE_MAX
#define MACHINE_MAX 256
#endif

char* get_prompt(){
    static char prompt[PATH_MAX + USER_MAX + MACHINE_MAX+3];
    static char cwd[PATH_MAX];

    const char *user = getenv("USER");
    if (!user) user = "user";

    char host[MACHINE_MAX + 1];
    if (gethostname(host, sizeof(host)) != 0) {
        strncpy(host, "host", MACHINE_MAX);
    }


    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(prompt,sizeof(prompt),"%s@%s:%s",user,host,cwd);
    } else {
        snprintf(prompt,sizeof(prompt),">");
    }

    return prompt;
}
