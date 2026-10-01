//
// Created by vikram on 9/30/26.
// Goal: handle the overall removable drive workflow

#include "include/drive.h"
#include "include/workspace.h"
#include "include/manifest.h"
#include "include/crypto.h"
#include "include/inotify.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sodium/crypto_generichash.h>

// TODO: organize files and move hash checker to crypto.c

static char *duplicate_string(const char *input) {
    const size_t len = strlen(input) + 1;
    char *copy = malloc(len);
    if (copy != NULL) {
        memcpy(copy, input, len);
    }
    return copy;
}

static char *join_path(const char *base, const char *name) {
    size_t len = strlen(base) + 1 + strlen(name) + 1;
    char *path = malloc(len);
    if (path == NULL) {
        return NULL;
    }
    snprintf(path, len, "%s/%s", base, name);
    return path;
}

static int ensure_parent_directory(const char *path) {
    char *copy = duplicate_string(path);
    if (copy == NULL) {
        return -1;
    }

    for (char *slash = copy + 1; *slash != '\0'; slash++) {
        if (*slash == '/') {
            *slash = '\0';
            if (mkdir(copy, 0700) != 0 && errno != EEXIST) {
                free(copy);
                return -1;
            }
            *slash = '/';
        }
    }

    free(copy);
    return 0;
}

static int copy_file(const char *source_path, const char *destination_path) {
    FILE *source = fopen(source_path, "rb");
    FILE *destination = fopen(destination_path, "wb");
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, bytes_read, destination) != bytes_read) {
            printf("Failed to write copied data to: %s\n", destination_path);
            fclose(source);
            fclose(destination);
            return -1;
        }
    }

    fclose(source);
    fclose(destination);
    return 0;
}

static void commit_workspace_files(const char *workspace_root_path, const char *current_path, const char *drive_path, const char *secretpath, FILE *manifest_file) {
    DIR *directory = opendir(current_path);
    if (directory == NULL) {
        printf("Failed to open workspace directory: %s\n", current_path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (strcmp(entry->d_name, "COMMIT_FILES") == 0 || strcmp(entry->d_name, "manifest.list") == 0) {
            continue;
        }

        char *full_path = join_path(current_path, entry->d_name);
        if (full_path == NULL) {
            printf("Failed to allocate workspace path for: %s\n", entry->d_name);
            continue;
        }

        struct stat path_stat;
        if (stat(full_path, &path_stat) != 0) {
            printf("Failed to stat workspace file: %s\n", full_path);
            free(full_path);
            continue;
        }

        if (S_ISDIR(path_stat.st_mode)) {
            commit_workspace_files(workspace_root_path, full_path, drive_path, secretpath, manifest_file);
            free(full_path);
            continue;
        }

        if (!S_ISREG(path_stat.st_mode)) {
            free(full_path);
            continue;
        }

        const char *relative_path = full_path + strlen(workspace_root_path) + 1;
        const size_t manifest_entry_len = strlen(relative_path) + 5; // +4 for .enc + null terminator
        char *manifest_entry = malloc(manifest_entry_len);
        if (manifest_entry == NULL) {
            printf("Failed to allocate manifest entry for: %s\n", full_path);
            free(full_path);
            continue;
        }
        snprintf(manifest_entry, manifest_entry_len, "%s.enc", relative_path);
        fprintf(manifest_file, "%s\n", manifest_entry);

        const size_t encrypted_path_len = strlen(full_path) + 5; // +4 for .enc + null terminator
        char *encrypted_path = malloc(encrypted_path_len);
        if (encrypted_path == NULL) {
            printf("Failed to allocate encrypted output path for: %s\n", full_path);
            free(manifest_entry);
            free(full_path);
            continue;
        }
        snprintf(encrypted_path, encrypted_path_len, "%s.enc", full_path);

        if (encrypt_file(full_path, encrypted_path, secretpath) != 0) {
            printf("Failed to encrypt file: %s\n", full_path);
            free(encrypted_path);
            free(manifest_entry);
            free(full_path);
            continue;
        }

        unsigned char *encrypted_hash = hash_file(encrypted_path);
        if (encrypted_hash == NULL) {
            printf("Failed to hash encrypted file: %s\n", encrypted_path);
            free(encrypted_path);
            free(manifest_entry);
            free(full_path);
            continue;
        }

        char *destination_path = join_path(drive_path, manifest_entry);
        if (destination_path == NULL) {
            printf("Failed to allocate destination path for: %s\n", manifest_entry);
            free(encrypted_hash);
            free(encrypted_path);
            free(manifest_entry);
            free(full_path);
            continue;
        }

        if (ensure_parent_directory(destination_path) != 0) {
            printf("Failed to create destination directory for: %s\n", destination_path);
            free(destination_path);
            free(encrypted_hash);
            free(encrypted_path);
            free(manifest_entry);
            free(full_path);
            continue;
        }

        if (copy_file(encrypted_path, destination_path) != 0) {
            printf("Failed to copy encrypted file: %s\n", destination_path);
            free(destination_path);
            free(encrypted_hash);
            free(encrypted_path);
            free(manifest_entry);
            free(full_path);
            continue;
        }

        unsigned char *destination_hash = hash_file(destination_path);
        if (destination_hash == NULL || memcmp(encrypted_hash, destination_hash, crypto_generichash_BYTES) != 0) {
            printf("Hash verification failed for file: %s while committing files.\n", destination_path);
            free(destination_hash);
            free(destination_path);
            free(encrypted_hash);
            free(encrypted_path);
            free(manifest_entry);
            free(full_path);
            continue;
        }

        if (remove(full_path) != 0) {
            printf("Failed to delete original file: %s\n", full_path);
        }
        if (remove(encrypted_path) != 0) {
            printf("Failed to delete encrypted file: %s\n", encrypted_path);
        }

        free(destination_hash);
        free(destination_path);
        free(encrypted_hash);
        free(encrypted_path);
        free(manifest_entry);
        free(full_path);
    }

    closedir(directory);
}

int run(const char *drive_path, const char *secretpath) {
    const size_t manifest_path_len = strlen(drive_path) + 14 + 1; // manifest.list
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

    // loop through files in the manifest and move them to the workspace,
    // decrypting if it ends with .enc, then delete the original file
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
        if (new_hash == NULL || memcmp(hash, new_hash, crypto_generichash_BYTES) != 0) {
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

    if (remove(manifest_path) != 0) {
        printf("Failed to delete manifest file: %s\n", manifest_path);
    }
    free(manifest_path);

    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);

    const size_t workspace_manifest_path_len = strlen(workspace_path) + 14 + 1; // manifest.list
    char* workspace_manifest_path = malloc(workspace_manifest_path_len);
    snprintf(workspace_manifest_path, workspace_manifest_path_len, "%s/%s", workspace_path, "manifest.list");
    FILE *fptr = fopen(workspace_manifest_path, "wb");
    fclose(fptr);

    const size_t commit_file_path_len = strlen(workspace_path) + 13 + 1; //COMMIT_FILES
    char* commit_file_path = malloc(commit_file_path_len);
    snprintf(commit_file_path, commit_file_path_len, "%s/%s", workspace_path, "COMMIT_FILES");
    fptr = fopen(commit_file_path, "w");
    fputs("0", fptr);
    fclose(fptr);

    watch(workspace_path);

    char *drive_manifest_path = join_path(drive_path, "manifest.list");
    FILE *manifest_file = fopen(workspace_manifest_path, "w");
    if (manifest_file != NULL) {
        commit_workspace_files(workspace_path, workspace_path, drive_path, secretpath, manifest_file);
        fclose(manifest_file);
    } else {
        printf("Failed to open workspace manifest file for writing: %s\n", workspace_manifest_path);
    }

    if (copy_file(workspace_manifest_path, drive_manifest_path) != 0) {
        printf("Failed to copy updated manifest to drive: %s\n", drive_manifest_path);
    }

    free(drive_manifest_path);
    free(workspace_manifest_path);
    free(commit_file_path);
    free((void*)workspace_path);
    return 0;
}
