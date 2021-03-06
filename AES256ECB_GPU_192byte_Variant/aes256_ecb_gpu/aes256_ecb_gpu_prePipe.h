#ifndef _AES256_ECB_GPU_PREPIPE_H_
#define _AES256_ECB_GPU_PREPIPE_H_

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t


double aes256_ecb_gpu_encrypt_prePipe(uint8_t *text, const size_t textByteCount, const uint8_t *key);

double aes256_ecb_gpu_decrypt_prePipe(uint8_t *text, const size_t textByteCount, const uint8_t *key);


#endif

