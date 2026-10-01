//
// Created by vikram on 9/30/26.
//

#ifndef USBFLYENC_CRYPTO_H
#define USBFLYENC_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

int encrypt_file(const char input[], const char output[], const char secretpath[]);
int create_secret_key(const char output[]);
int decrypt_file(const char input[], const char output[], const char secretpath[]);
unsigned char* hash_file(const char *filename);

#ifdef __cplusplus
}
#endif

#endif //USBFLYENC_CRYPTO_H
