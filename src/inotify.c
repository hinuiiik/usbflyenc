//
// Created by vikram on 9/30/26.
// Goal: monitor for COMMIT_FILES file being modified and read the value, if 1 then exit
#include "include/inotify.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <string.h>
#include <stdlib.h>

void watch(const char* directory) {
    const int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return;
    }

    const int wd = inotify_add_watch(fd, directory, IN_MODIFY);
    if (wd < 0) {
        perror("inotify_add_watch");
        close(fd);
        return;
    }

    char buffer[1024];
    while (1) {
        const int length = read(fd, buffer, sizeof(buffer));
        if (length < 0) {
            perror("read");
            break;
        }

        int i = 0;
        while (i < length) {
            const struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len) {
                if (event->mask & IN_MODIFY) {
                    if (strcmp(event->name, "COMMIT_FILES") == 0) {
                        char commit_file_path[512];
                        snprintf(commit_file_path, sizeof(commit_file_path), "%s/COMMIT_FILES", directory);
                        FILE *fptr = fopen(commit_file_path, "r");
                        if (fptr != NULL) {
                            char value[2];
                            fgets(value, sizeof(value), fptr);
                            fclose(fptr);
                            if (strcmp(value, "1") == 0) {
                                printf("Commit file modified and value is 1. Exiting watch.\n");
                                inotify_rm_watch(fd, wd);
                                close(fd);
                                return;
                            }
                        }
                    }
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
}