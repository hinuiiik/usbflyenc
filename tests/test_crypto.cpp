//
// Created by vikram on 9/30/26.
//
#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include "../include/crypto.h"

#define PLAINTEXT_CONTENTS "This is a testing file for encryption and decryption. \n It contains some sample text to verify the functionality of the crypto library."
#define KEYFILE_NAME "test_secret.key"
#define PLAINTEXT_FILE_NAME "test_file.txt"
#define CIPHER_FILE_NAME "test_encrypted.enc"
#define DECRYPTED_FILE_NAME "test_decrypted.txt"
#define SECOND_KEYFILE_NAME "test_secret_2.key"

using namespace std;

const string data_dir = getenv("TEST_DATA_DIR");

class CryptoTests : public ::testing::Test {
protected:
    void SetUp() override {
        ofstream testFile(PLAINTEXT_FILE_NAME);
        if (testFile.is_open()) {
            testFile << PLAINTEXT_CONTENTS;
            testFile.close();
        }
    }

    void TearDown() override {
        error_code ec;
        filesystem::remove(PLAINTEXT_FILE_NAME, ec);
        filesystem::remove(KEYFILE_NAME, ec);
        filesystem::remove(CIPHER_FILE_NAME, ec);
        filesystem::remove(DECRYPTED_FILE_NAME, ec);
        filesystem::remove(SECOND_KEYFILE_NAME, ec);
    }
};

// TODO: make more in line with AAA

TEST_F(CryptoTests, CreateSecretKey) {
    const char *secret_path = KEYFILE_NAME;
    int result = create_secret_key((char *)secret_path);
    EXPECT_EQ(result, 0);
}

TEST_F(CryptoTests, EncryptFile) {
    const char *secret_path = KEYFILE_NAME;
    const char *input_file = PLAINTEXT_FILE_NAME;
    const char *cipher_file = CIPHER_FILE_NAME;
    ASSERT_EQ(create_secret_key((char *)secret_path), 0);
    ASSERT_EQ(encrypt_file((char *)input_file, (char *)cipher_file, (char *)secret_path), 0);
}

TEST_F(CryptoTests, DecryptFile) {
    string secret_path = (data_dir + "/" + KEYFILE_NAME);
    string cipher_file = (data_dir + "/" + CIPHER_FILE_NAME);
    string decrypted_file = DECRYPTED_FILE_NAME;

    ASSERT_EQ(decrypt_file(cipher_file.c_str(), decrypted_file.c_str(), secret_path.c_str()), 0);

    ifstream decryptedFile(decrypted_file);
    string decryptedContents((istreambuf_iterator<char>(decryptedFile)), istreambuf_iterator<char>());
    decryptedFile.close();

    EXPECT_EQ(decryptedContents, PLAINTEXT_CONTENTS);
}

TEST_F(CryptoTests, EncryptAndDecryptFile) {
    const char *secret_path = KEYFILE_NAME;
    const char *input_file = PLAINTEXT_FILE_NAME;
    const char *cipher_file = CIPHER_FILE_NAME;
    const char *decrypted_file = DECRYPTED_FILE_NAME;

    ASSERT_EQ(create_secret_key((char *)secret_path), 0);
    ASSERT_EQ(encrypt_file((char *)input_file, (char *)cipher_file, (char *)secret_path), 0);
    ASSERT_EQ(decrypt_file((char *)cipher_file, (char *)decrypted_file, (char *)secret_path), 0);

    ifstream decryptedFile(decrypted_file);
    string decryptedContents((istreambuf_iterator<char>(decryptedFile)), istreambuf_iterator<char>());
    decryptedFile.close();

    EXPECT_EQ(decryptedContents, PLAINTEXT_CONTENTS);
}

TEST_F(CryptoTests, DecryptWithWrongKey) {
    const char *secret_path = KEYFILE_NAME;
    const char *input_file = PLAINTEXT_FILE_NAME;
    const char *cipher_file = CIPHER_FILE_NAME;
    const char *decrypted_file = DECRYPTED_FILE_NAME;

    ASSERT_EQ(create_secret_key((char *)secret_path), 0);
    ASSERT_EQ(encrypt_file((char *)input_file, (char *)cipher_file, (char *)secret_path), 0);

    const char *wrong_key_path = SECOND_KEYFILE_NAME;
    ASSERT_EQ(create_secret_key((char *)wrong_key_path), 0);

    int result = decrypt_file((char *)cipher_file, (char *)decrypted_file, (char *)wrong_key_path);
    EXPECT_EQ(result, -1);
}