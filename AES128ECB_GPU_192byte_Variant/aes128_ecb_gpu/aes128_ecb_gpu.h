#ifndef _AES128_ECB_GPU_H_
#define _AES128_ECB_GPU_H_

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t


int32_t aes128_ecb_gpu_encrypt(uint8_t *text, const size_t textByteCount, const uint8_t *key);

int32_t aes128_ecb_gpu_decrypt(uint8_t *text, const size_t textByteCount, const uint8_t *key);


#endif

