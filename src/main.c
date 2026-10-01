#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "include/crypto.h"
#include "include/dbus.h"

static int printHelp(void) {
    printf("No arguments provided.\n"
               "Available arguments:\n"
               "generate-secret \"destination\" (including the file itself) \n"
               "encrypt \"input\" \"output\" \"secret path\"\n"
               "decrypt \"input\" \"output\" \"secret path\"\n"
               "monitor \"secret path\"\n");
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printHelp();
        return 1;
    }
    if (strcmp(argv[1], "generate-secret") == 0) {
        if (argc != 3) {
            printf("Invalid number of arguments for generate-secret.\n");
            return 1;
        }
        create_secret_key(argv[2]);
    } else if (strcmp(argv[1], "encrypt") == 0) {
        if (argc != 5) {
            printf("Invalid number of arguments for encrypt.\n");
            return 1;
        }
        if (access(argv[2], F_OK) == 0) {
            encrypt_file(argv[2], argv[3], argv[4]);
        } else {
            printf("Input file does not exist.\n");
            return 1;
        }

    } else if (strcmp(argv[1], "decrypt") == 0) {
        if (argc != 5) {
            printf("Invalid number of arguments for decrypt.\n");
            return 1;
        }
        if (access(argv[2], F_OK) == 0) {
            decrypt_file(argv[2], argv[3], argv[4]);
        } else {
            printf("Input file does not exist.\n");
            return 1;
        }
    } else if (strcmp(argv[1], "monitor") == 0) {
        if (argc != 3) {
            printf("Invalid number of arguments for monitor.\n");
            return 1;
        }
        monitor_dbus(argv[2]);
    } else {
        printHelp();
        return 1;
    }
    return 0;
}



