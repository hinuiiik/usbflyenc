//
// Created by vikram on 9/30/26.
// Goal: handle the temporary decrypted workspace

#include "include/workspace.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

char* create_workspace(void) {
    time_t t = time(NULL);
    char *workspace_path = malloc((15 + 10 + 1) * sizeof(char)); // "/tmp/workspace_" + 10 digits for time_t + null terminator
    workspace_path[15+10+1] = '\0';
    snprintf(workspace_path, 50, "/tmp/workspace_%ld", t);
    if (mkdir(workspace_path, 0700) != 0) {
        return NULL;
    }
    return workspace_path;
}
