//
// Created by vikram on 9/30/26.
// Goal: handle the overall removable drive workflow

#include "include/drive.h"
#include "include/workspace.h"
#include "include/manifest.h"
#include "include/crypto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sodium/crypto_generichash.h>

int run(const char *drive_path, const char *secretpath) {
    size_t manifest_path_len = strlen(drive_path) + 14 + 1; // manifest.list
    char* manifest_path = malloc(manifest_path_len);
    strcpy(manifest_path, drive_path);
    strcat(manifest_path, "/manifest.list");
    char **file_list = NULL;
    int file_count=0;
    read_manifest(manifest_path, &file_list, &file_count);
    if (file_list == NULL) {
        printf("Manifest file failed to open in %s. This is not an error unless you expect a manifest file in that location.\n", drive_path);
        free(manifest_path);
        return 1;
    }
    free(manifest_path);
    if (file_count == 0) {
        printf("No files to process.\n");
        return 0;
    }

    const char *workspace_path = create_workspace();
    if (workspace_path == NULL) {
        printf("Failed to create workspace.\n");
        for (int i = 0; i < file_count; i++) {
            free(file_list[i]);
        }
        free(file_list);
        return 1;
    }

    for (int i = 0; i < file_count; i++) {
        const size_t file_path_len = strlen(drive_path) + 1 + strlen(file_list[i]) + 1;
        const size_t file_path_relative_len = strlen(file_list[i]) + 1;
        char *file_path = malloc(file_path_len);
        char *file_path_relative = malloc(file_path_relative_len);
        snprintf(file_path, file_path_len, "%s/%s", drive_path, file_list[i]);
        strncpy(file_path_relative, file_list[i], file_path_relative_len);
        unsigned char* hash = hash_file(file_path);
        if (hash == NULL) {
            printf("Failed to hash file: %s\n", file_path);
            continue;
        }

        // Copy the file to the workspace
        const size_t new_file_path_len = strlen(workspace_path) + 1 + file_path_relative_len;
        char *new_file_path = malloc(new_file_path_len);
        snprintf(new_file_path, new_file_path_len, "%s/%s", workspace_path, file_path_relative);

        // Checks if a directory needs to be created for the new file path and creates it if necessary
        char *last_slash = strrchr(new_file_path, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
            mkdir(new_file_path, 0700);
            *last_slash = '/';
        }

        FILE *src = fopen(file_path, "rb");
        FILE *dest = fopen(new_file_path, "wb");
        if (src == NULL || dest == NULL) {
            printf("Failed to open source or destination file for copying: %s\n", file_path);
            free(hash);
            free(new_file_path);
            continue;
        }

        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, bytes, dest);
        }

        fclose(src);
        fclose(dest);

        unsigned char *new_hash = hash_file(new_file_path);
        if (new_hash == NULL || strncmp((const char*)hash, (const char*)new_hash, crypto_generichash_BYTES) != 0) {
            printf("Hash verification failed for file: %s\n", new_file_path);
            free(hash);
            free(new_hash);
            free(new_file_path);
            continue;
        }


        free(hash);
        free(new_hash);

        if (remove(file_path) != 0) {
            printf("Failed to delete original file: %s\n", file_path);
        }

        // if it ends with .enc, decrypt it and write the new file to the workspace without the .enc extension and delete the original .enc file
        if (strstr(file_path, ".enc") != NULL) {
            const size_t decrypted_file_path_len = strlen(workspace_path) + 1 + file_path_relative_len - 4; // -4 for .enc
            char *decrypted_file_path = malloc(decrypted_file_path_len);
            snprintf(decrypted_file_path, decrypted_file_path_len, "%s/%.*s", workspace_path, (int)(file_path_relative_len - 5), file_path_relative); // -5 for .enc and null terminator

            if (decrypt_file(new_file_path, decrypted_file_path, secretpath) != 0) {
                printf("Failed to decrypt file: %s\n", file_path);
                free(decrypted_file_path);
                continue;
            }

            if (remove(new_file_path) != 0) {
                printf("Failed to delete original encrypted file: %s\n", file_path);
            }

            free(decrypted_file_path);
        }

        free(file_path);
        free(new_file_path);
        free(file_path_relative);
    }

    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }

    free((void*)workspace_path);
    free(file_list);
    return 0;
}
