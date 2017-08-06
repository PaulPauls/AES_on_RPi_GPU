#ifndef AES256_ECB_H
#define AES256_ECB_H

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t



int aes256_ecb_encrypt(uint8_t *data, size_t dataSize, const uint8_t *key, size_t keySize);

int aes256_ecb_decrypt(uint8_t *data, size_t dataSize, const uint8_t *key, size_t keySize);



#endif

