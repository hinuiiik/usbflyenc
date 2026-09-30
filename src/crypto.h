//
// Created by vikram on 9/30/26.
//

#ifndef USBFLYENC_CRYPTO_H
#define USBFLYENC_CRYPTO_H

int encrypt_file(char input[], char output[], char secretpath[]);
int create_secret_key(char output[]);
int decrypt_file(char input[], char output[], char secretpath[]);

#endif //USBFLYENC_CRYPTO_H
