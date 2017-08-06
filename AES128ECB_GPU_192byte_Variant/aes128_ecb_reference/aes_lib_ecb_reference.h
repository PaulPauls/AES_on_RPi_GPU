#ifndef _AES_LIB_ECB_H_
#define _AES_LIB_ECB_H_

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t



void add_round_key(uint8_t *state, const uint8_t *expandedKey, int round);

void sub_bytes(uint8_t *state);

void inv_sub_bytes(uint8_t *state);

void shift_row(uint8_t *state);

void inv_shift_row(uint8_t *state);

void mix_columns(uint8_t *state);

void inv_mix_columns(uint8_t *state);



#endif

