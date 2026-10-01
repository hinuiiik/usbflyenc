//
// Created by vikram on 9/30/26.
// Goal: Provide encryption and decryption functions for files using libsodium
//

#include "../include/crypto.h"

#include <inttypes.h>
#include <sodium.h>
#include <string.h>

int get_file_length(FILE *fptr) {
    size_t pos = ftell(fptr);
    fseek(fptr, 0, SEEK_END);
    size_t length = ftell(fptr);
    fseek(fptr, pos, SEEK_SET);
    return length;
}

int encrypt_file(const char input[], const char output[], const char secretpath[]) {
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized; it is not safe to use */
    }
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];

    FILE *fptr = fopen(secretpath, "rb");
    fread(key, sizeof(unsigned char), 32, fptr);
    fclose(fptr);

    // copied from stackoverflow; gets length
    fptr = fopen(input, "rb");
    const size_t plaintext_length = get_file_length(fptr);

    unsigned char plaintext[plaintext_length];
    fread(plaintext, sizeof(unsigned char), plaintext_length, fptr);
    fclose(fptr);

    uint32_t ciphertext_length = crypto_secretbox_MACBYTES + plaintext_length;
    unsigned char ciphertext[ciphertext_length];

    randombytes_buf(nonce, sizeof nonce);
    crypto_secretbox_easy(ciphertext, plaintext, plaintext_length, nonce, key);
    fptr = fopen(output, "wb");
    fwrite(nonce, sizeof(unsigned char), crypto_secretbox_NONCEBYTES, fptr);
    fwrite(&ciphertext_length, sizeof(uint32_t), 1, fptr);
    fwrite(ciphertext, sizeof(unsigned char), ciphertext_length, fptr);
    fclose(fptr);

    memset(nonce, 0, sizeof nonce);
    memset(key, 0, crypto_secretbox_KEYBYTES);
    memset(plaintext, 0, plaintext_length);
    memset(ciphertext, 0, ciphertext_length);
    return 0;
}

int decrypt_file(const char input[], const char output[], const char secretpath[]) {
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized; it is not safe to use */
    }

    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];

    FILE *fptr = fopen(secretpath, "rb");
    fread(key, sizeof(unsigned char), 32, fptr);
    fclose(fptr);

    fptr = fopen(input, "rb");
    fread(nonce, sizeof(unsigned char), crypto_secretbox_NONCEBYTES, fptr);
    uint32_t ciphertext_length;
    fread(&ciphertext_length, sizeof(uint32_t), 1, fptr);
    unsigned char ciphertext[ciphertext_length];
    fread(ciphertext, sizeof(unsigned char), ciphertext_length, fptr);
    fclose(fptr);

    uint32_t plaintext_length = ciphertext_length - crypto_secretbox_MACBYTES;
    unsigned char plaintext[plaintext_length];

    if (crypto_secretbox_open_easy(plaintext, ciphertext, ciphertext_length, nonce, key) != 0) {
        printf("Message forged\n");
        return -1;
    }

    fptr = fopen(output, "wb");
    fwrite(plaintext, sizeof(unsigned char), plaintext_length, fptr);
    fclose(fptr);

    memset(nonce, 0, sizeof nonce);
    memset(key, 0, crypto_secretbox_KEYBYTES);
    memset(plaintext, 0, plaintext_length);
    memset(ciphertext, 0, ciphertext_length);
    return 0;
}

int create_secret_key(const char output[]) {
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized; it is not safe to use */
    }
    unsigned char key[crypto_secretbox_KEYBYTES];
    FILE *fptr = fopen(output, "wb");
    crypto_secretbox_keygen(key);
    fwrite(key, sizeof(unsigned char), 32, fptr);

    memset(key, 0, crypto_secretbox_KEYBYTES);
    fclose(fptr);
    return 0;
}

unsigned char* hash_file(const char *filename) {
    FILE *fptr = fopen(filename, "rb");
    const size_t file_length = get_file_length(fptr);
    unsigned char file_content[file_length];
    fread(file_content, sizeof(unsigned char), file_length, fptr);

    unsigned char* hash = malloc(crypto_generichash_BYTES);
    crypto_generichash(hash, sizeof hash,
                   file_content, file_length,
                   NULL, 0);

    memset(file_content, 0, file_length);
    fclose(fptr);
    return hash;
}
