//
// Created by vikram on 9/30/26.
// Goal: Read manifest.list in the dir and return the list of files

#include "include/manifest.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void read_manifest(const char *manifest_path, char ***file_list, int *file_count) {
    FILE *file = fopen(manifest_path, "r");
    if (!file) {
        *file_list = NULL;
        *file_count = 0;
        return;
    }

    size_t capacity = 10; // Initial capacity for the file list; arbitrary
    *file_list = malloc(capacity * sizeof(char *));
    if (*file_list == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return;
    }
    *file_count = 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        if (strstr(line, "..") != NULL || line[0] == '/' || strstr(line, "~") != NULL) {
            fprintf(stderr, "Error: Invalid path in manifest file: %s\n", line);
            continue;
        }

        if (*file_count >= capacity) { // basically an arraylist
            capacity *= 2;
            *file_list = realloc(*file_list, capacity * sizeof(char *));
        }

        // strdup isn't available on C17
        (*file_list)[*file_count] = malloc(strlen(line) + 1);
        strcpy((*file_list)[*file_count], line);
        (*file_count)++;
    }

    fclose(file);
}
