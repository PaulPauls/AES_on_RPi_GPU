#define _XOPEN_SOURCE 700
#define __STDC_FORMAT_MACROS

#include <stdio.h> // printf(), fprintf()
#include <inttypes.h> // uint8_t, uint32_t
#include <string.h> // memcpy()
#include <stdlib.h> // srand()
#include <time.h> // clock_gettime(), timespec, CLOCK_REALTIME, time()
#include <openssl/aes.h>
#include "aes128_ecb_reference.h" // aes128_ecb_encrypt()
#include "aes128_ecb_gpu.h" // aes128_ecb_gpu_encrypt()
#include "aes128_ecb_gpu_prePipe.h"
#include "rijndael.h"
#include "aes.h"



#define NUM_RUNS  (100)
#define NUM_BYTES (1536)
#define CSV_FILE  "timing_results.csv"

#define ENCRYPT_ELSE_DECRYPT (1)
#define PRELOAD_PIPE         (1)



void print_hex(const uint8_t *input, size_t size);

int compare_hex(const uint8_t *buffer1, const uint8_t *buffer2, size_t size);

void write_to_csv(FILE *fp, uint64_t time, int newline);



int main(int argc, char **argv)
{
  uint64_t timeOpenSSLTotal = 0;
  uint64_t timeRijndaelTotal = 0;
  uint64_t timeGPUTotal = 0;
  uint64_t timeReferenceTotal = 0;
  uint64_t timeTinyAESTotal = 0;

  uint64_t timeOpenSSLAvg = 0;
  uint64_t timeRijndaelAvg = 0;
  uint64_t timeGPUAvg = 0;
  uint64_t timeReferenceAvg = 0;
  uint64_t timeTinyAESAvg = 0;

  int resultsMatch = 1;

  FILE *csvFP = fopen(CSV_FILE, "w");

  srand(time(NULL));



  for (int nor = 0; nor < NUM_RUNS; nor++) {
    if (nor % 10 == 0) {
      printf("Round %d\n", nor);
    }

    uint8_t key[16];
    uint8_t data1[NUM_BYTES];

    // Fill 'key' with random data
    for (int i = 0; i < (sizeof(key) / sizeof(key[0])); i++) {
      key[i] = (uint8_t) rand();
    }

    // Fill 'data' with random data
    for (int i = 0; i < (sizeof(data1) / sizeof(data1[0])); i++) {
      data1[i] = (uint8_t) rand();
    }

    // Copies of data1 as all Implementations apply AES in memory
    uint8_t data2[sizeof(data1) / sizeof(data1[0])];
    memcpy(data2, data1, (sizeof(data1) / sizeof(data1[0])));
    uint8_t data3_in[sizeof(data1) / sizeof(data1[0])];
    memcpy(data3_in, data1, (sizeof(data1) / sizeof(data1[0])));
    uint8_t data3_out[sizeof(data1) / sizeof(data1[0])];
    memcpy(data3_out, data1, (sizeof(data1) / sizeof(data1[0])));
    uint8_t data4_in[sizeof(data1) / sizeof(data1[0])];
    memcpy(data4_in, data1, (sizeof(data1) / sizeof(data1[0])));
    uint8_t data4_out[sizeof(data1) / sizeof(data1[0])];
    memcpy(data4_out, data1, (sizeof(data1) / sizeof(data1[0])));
    uint8_t data5_in[sizeof(data1) / sizeof(data1[0])];
    memcpy(data5_in, data1, (sizeof(data1) / sizeof(data1[0])));
    uint8_t data5_out[sizeof(data1) / sizeof(data1[0])];
    memcpy(data5_out, data1, (sizeof(data1) / sizeof(data1[0])));


    // Timing structs and variables
    struct timespec start, stop;
    double timeDiff;


    if (ENCRYPT_ELSE_DECRYPT) {
      // CPU Execution of AES OpenSSL
      clock_gettime(CLOCK_REALTIME, &start);
      AES_KEY enc_key;
      AES_set_encrypt_key(key, sizeof(key)*8, &enc_key);
      for (int i = 0; i < (sizeof(data1) / sizeof(data1[0])); i = i+16) {
        AES_ecb_encrypt(data3_in + i, data3_out + i, &enc_key, AES_ENCRYPT);
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeOpenSSLTotal = timeOpenSSLTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 0);



      // CPU Execution of AES Rijndael
      clock_gettime(CLOCK_REALTIME, &start);
      unsigned long expandedKey[44];
      rijndaelSetupEncrypt(expandedKey, key, 128);
      for (int i = 0; i < (sizeof(data1) / sizeof(data1[0])); i = i+16) {
        rijndaelEncrypt(expandedKey, 10, data4_in + i, data4_out + i);
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeRijndaelTotal = timeRijndaelTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 0);



      if (PRELOAD_PIPE) {
        // GPU Execution of AES with pre loaded Pipe
        timeDiff = aes128_ecb_gpu_encrypt_prePipe(data1, sizeof(data1), key);
        timeGPUTotal = timeGPUTotal + (uint64_t)timeDiff;
        write_to_csv(csvFP, (uint64_t) timeDiff, 0);
      } else {
        // GPU Execution of AES without pre loaded Pipe
        clock_gettime(CLOCK_REALTIME, &start);
        aes128_ecb_gpu_encrypt(data1, sizeof(data1), key);
        clock_gettime(CLOCK_REALTIME, &stop);
        timeDiff = (stop.tv_sec - start.tv_sec)
                 + (stop.tv_nsec - start.tv_nsec) / 1E3;
        timeGPUTotal = timeGPUTotal + (uint64_t)timeDiff;
        write_to_csv(csvFP, (uint64_t) timeDiff, 0);
      }



      // CPU Execution of AES Tiny
      clock_gettime(CLOCK_REALTIME, &start);
      for (int i = 0; i < (sizeof(data1) / sizeof(data1[0])); i = i+16) {
        AES128_ECB_encrypt(data5_in + i, key, data5_out + i);
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeTinyAESTotal = timeTinyAESTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 0);



      // CPU Execution of AES Reference
      clock_gettime(CLOCK_REALTIME, &start);
      if (aes128_ecb_encrypt(data2, sizeof(data2), key, sizeof(key)) < 0) {
        fprintf(stderr, "Error when calling aes128_ecb_encrypt(...)\n");
        return -1;
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeReferenceTotal = timeReferenceTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 1);



      // Test if OpenSSL and GPU results match
      resultsMatch = resultsMatch && compare_hex(data1, data3_out, (sizeof(data1) / sizeof(data1[0])));

    } else {

      // CPU Execution of AES OpenSSL
      clock_gettime(CLOCK_REALTIME, &start);
      AES_KEY dec_key;
      AES_set_decrypt_key(key, sizeof(key)*8, &dec_key);
      for (int i = 0; i < (sizeof(data1) / sizeof(data1[0])); i = i+16) {
        AES_ecb_encrypt(data3_in + i, data3_out + i, &dec_key, AES_DECRYPT);
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeOpenSSLTotal = timeOpenSSLTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 0);



      // CPU Execution of AES Rijndael
      clock_gettime(CLOCK_REALTIME, &start);
      unsigned long expandedKey[44];
      rijndaelSetupDecrypt(expandedKey, key, 128);
      for (int i = 0; i < (sizeof(data1) / sizeof(data1[0])); i = i+16) {
        rijndaelDecrypt(expandedKey, 10, data4_in + i, data4_out + i);
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeRijndaelTotal = timeRijndaelTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 0);



      if (PRELOAD_PIPE) {
        // GPU Execution of AES with pre loaded Pipe
        timeDiff = aes128_ecb_gpu_decrypt_prePipe(data1, sizeof(data1), key);
        timeGPUTotal = timeGPUTotal + (uint64_t)timeDiff;
        write_to_csv(csvFP, (uint64_t) timeDiff, 0);
      } else {
        // GPU Execution of AES without pre loaded Pipe
        clock_gettime(CLOCK_REALTIME, &start);
        aes128_ecb_gpu_decrypt(data1, sizeof(data1), key);
        clock_gettime(CLOCK_REALTIME, &stop);
        timeDiff = (stop.tv_sec - start.tv_sec)
                 + (stop.tv_nsec - start.tv_nsec) / 1E3;
        timeGPUTotal = timeGPUTotal + (uint64_t)timeDiff;
        write_to_csv(csvFP, (uint64_t) timeDiff, 0);
      }



      // CPU Execution of AES Tiny
      clock_gettime(CLOCK_REALTIME, &start);
      for (int i = 0; i < (sizeof(data1) / sizeof(data1[0])); i = i+16) {
        AES128_ECB_decrypt(data5_in + i, key, data5_out + i);
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeTinyAESTotal = timeTinyAESTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 0);



      // CPU Execution of AES Reference
      clock_gettime(CLOCK_REALTIME, &start);
      if (aes128_ecb_decrypt(data2, sizeof(data2), key, sizeof(key)) < 0) {
        fprintf(stderr, "Error when calling aes128_ecb_encrypt(...)\n");
        return -1;
      }
      clock_gettime(CLOCK_REALTIME, &stop);
      timeDiff = (stop.tv_sec - start.tv_sec)
               + (stop.tv_nsec - start.tv_nsec) / 1E3;
      timeReferenceTotal = timeReferenceTotal + (uint64_t)timeDiff;
      write_to_csv(csvFP, (uint64_t) timeDiff, 1);



      // Test if OpenSSL and GPU results match
      resultsMatch = resultsMatch && compare_hex(data1, data3_out, (sizeof(data1) / sizeof(data1[0])));
    }
  }

  fclose(csvFP);

  if (ENCRYPT_ELSE_DECRYPT) {
    printf("\n\nEncryption\n\n");
  } else {
    printf("\n\nDecryption\n\n");
  }

  if (PRELOAD_PIPE) {
    printf("GPU pipe acquired before: Yes\n");
  } else {
    printf("GPU pipe acquired before: No\n");
  }
  printf("Number of Runs: %d\n", NUM_RUNS);
  printf("Bytes encrypted: %d\n\n", NUM_BYTES);


  printf("Total time for OpenSSL: %"PRIu64"ns\n", timeOpenSSLTotal);
  printf("Total time for Rijndael: %"PRIu64"ns\n", timeRijndaelTotal);
  printf("Total time for GPU: %"PRIu64"ns\n", timeGPUTotal);
  printf("Total time for tinyAES: %"PRIu64"ns\n", timeTinyAESTotal);
  printf("Total time for Reference: %"PRIu64"ns\n\n", timeReferenceTotal);


  timeOpenSSLAvg = (uint64_t) timeOpenSSLTotal / NUM_RUNS;
  timeRijndaelAvg = (uint64_t) timeRijndaelTotal / NUM_RUNS;
  timeGPUAvg = (uint64_t) timeGPUTotal / NUM_RUNS;
  timeReferenceAvg = (uint64_t) timeReferenceTotal / NUM_RUNS;
  timeTinyAESAvg = (uint64_t) timeTinyAESTotal / NUM_RUNS;


  printf("Average time for OpenSSL: %"PRIu64"ns\n", timeOpenSSLAvg);
  printf("Average time for Rijndael: %"PRIu64"ns\n", timeRijndaelAvg);
  printf("Average time for GPU: %"PRIu64"ns\n", timeGPUAvg);
  printf("Average time for tinyAES: %"PRIu64"ns\n", timeTinyAESAvg);
  printf("Average time for Reference: %"PRIu64"ns\n\n", timeReferenceAvg);


  if (resultsMatch) {
    printf("OpenSSL and GPU result match: yes\n");
  } else {
    printf("OpenSSL and GPU result match: NO!\n");
  }


  return 1;
}



/* Prints each byte of the input array as a hex encoded number.
 * Line break at each 16th number.
 */
void print_hex(const uint8_t *input, size_t size)
{
  for (int i = 0; i < size; i++) {
    printf("%.2x", input[i]);

    if ((i+1) % 16 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}



/* Compares each byte of both buffers and returns 1 if they match. 0 otherwise
 */
int compare_hex(const uint8_t *buffer1, const uint8_t *buffer2, size_t size)
{
  for (int i = 0; i < size; i++) {
    if(buffer1[i] != buffer2[i]) {
      return 0;
    }
  }

  return 1;
}



/* Writes 'time' value to csv formatted file. Adds ';' instead of ',' if
 * 'newline is set.
 */
void write_to_csv(FILE *fp, uint64_t time, int newline)
{
  if (newline) {
    fprintf(fp, "%"PRIu64";\n", time);
  } else {
    fprintf(fp, "%"PRIu64", ", time);
  }
}

