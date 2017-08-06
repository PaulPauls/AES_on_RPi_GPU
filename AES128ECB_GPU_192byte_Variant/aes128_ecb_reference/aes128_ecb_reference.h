#ifndef _AES128_ECB_H_
#define _AES128_ECB_H_

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t



int aes128_ecb_encrypt(uint8_t *input, size_t inputSize, const uint8_t *key, size_t keySize);

int aes128_ecb_decrypt(uint8_t *input, size_t inputSize, const uint8_t *key, size_t keySize);



#endif

