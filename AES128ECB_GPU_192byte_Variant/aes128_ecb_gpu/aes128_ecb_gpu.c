#include <stdio.h>    // printf(), fprintf()
#include <stddef.h>   // offsetof
#include <inttypes.h> // uint8_t, uint32_t
#include <string.h>   // memcpy(), memset()
#include <openssl/aes.h> // AES_KEY, aes_set_encrypt_key
#include <stdlib.h>   // EXIT_FAILURE, size_t

#include "mailbox.h"  // mbox_open(), qpu_enable(), mem_alloc(), mem_lock(),
                      // mapmem(), ETC ############################# ADD HERE



#define QPU_ENC_CODEFILE "./aes128_ecb_gpu/aes128_ecb_gpu_encrypt.bin"
#define QPU_DEC_CODEFILE "./aes128_ecb_gpu/aes128_ecb_gpu_decrypt.bin"
#define NUM_QPUS        (12)
#define NUM_UNIFORMS    (3)
#define NUM_MESSAGES    (2)
#define GPU_MEM_FLAG    (0xc) // cached
#define GPU_MEM_MAP     (0x0) // cached

#define QPU_ENC_CODE_BYTECOUNT (5304)
#define QPU_DEC_CODE_BYTECOUNT (12680)
#define SBOX_WORDSIZE   (256) // 256 words for 1024 byte SBOX
#define KEY_WORDSIZE    (44)  // 44 words for 176 byte Key
#define TEXT_WORDSIZE   (4)   // 48 words for 192 byte total, 16 byte for ea QPU

//#define DEBUG



int32_t aes128_ecb_gpu_encrypt(uint8_t *text, const size_t textBytecount, const uint8_t *key);

int32_t aes128_ecb_gpu_decrypt(uint8_t *text, const size_t textBytecount, const uint8_t *key);

void load_qpu_code(const char *qpuCodeFile, uint32_t *qpuCode, uint32_t bytecount);



struct EncGPUMemoryMap {
  uint32_t qpuCode[QPU_ENC_CODE_BYTECOUNT / sizeof(uint32_t)];
  uint32_t data[ SBOX_WORDSIZE                // accessed via TMU
               + KEY_WORDSIZE                 // accessed via TMU
               + TEXT_WORDSIZE * NUM_QPUS ];  // accessed via VPM, divided
  uint32_t uniforms[NUM_QPUS * NUM_UNIFORMS];
  uint32_t messages[NUM_QPUS * NUM_MESSAGES];
};

struct DecGPUMemoryMap {
  uint32_t qpuCode[QPU_DEC_CODE_BYTECOUNT / sizeof(uint32_t)];
  uint32_t data[ SBOX_WORDSIZE                // accessed via TMU
               + KEY_WORDSIZE                 // accessed via TMU
               + TEXT_WORDSIZE * NUM_QPUS ];  // accessed via VPM, divided
  uint32_t uniforms[NUM_QPUS * NUM_UNIFORMS];
  uint32_t messages[NUM_QPUS * NUM_MESSAGES];
};



/* Encrypts 'text' with 'key' according to AES128 in ECB mode on the RPi GPU.
 * On success, the plaintext in 'text' is replaced with the corresponding
 * ciphertext and the function returns 1.
 * On failure, the function exits or returns -1.
 */
int32_t aes128_ecb_gpu_encrypt(uint8_t *text, const size_t textBytecount, const uint8_t *key)
{
  uint32_t localQPUCode[QPU_ENC_CODE_BYTECOUNT / sizeof(uint32_t)];
  AES_KEY  expandedKey;

  uint32_t sbox[] = {0x00000063, 0x0000007C, 0x00000077, 0x0000007B,
                     0x000000F2, 0x0000006B, 0x0000006F, 0x000000C5,
                     0x00000030, 0x00000001, 0x00000067, 0x0000002B,
                     0x000000FE, 0x000000D7, 0x000000AB, 0x00000076,
                     0x000000CA, 0x00000082, 0x000000C9, 0x0000007D,
                     0x000000FA, 0x00000059, 0x00000047, 0x000000F0,
                     0x000000AD, 0x000000D4, 0x000000A2, 0x000000AF,
                     0x0000009C, 0x000000A4, 0x00000072, 0x000000C0,
                     0x000000B7, 0x000000FD, 0x00000093, 0x00000026,
                     0x00000036, 0x0000003F, 0x000000F7, 0x000000CC,
                     0x00000034, 0x000000A5, 0x000000E5, 0x000000F1,
                     0x00000071, 0x000000D8, 0x00000031, 0x00000015,
                     0x00000004, 0x000000C7, 0x00000023, 0x000000C3,
                     0x00000018, 0x00000096, 0x00000005, 0x0000009A,
                     0x00000007, 0x00000012, 0x00000080, 0x000000E2,
                     0x000000EB, 0x00000027, 0x000000B2, 0x00000075,
                     0x00000009, 0x00000083, 0x0000002C, 0x0000001A,
                     0x0000001B, 0x0000006E, 0x0000005A, 0x000000A0,
                     0x00000052, 0x0000003B, 0x000000D6, 0x000000B3,
                     0x00000029, 0x000000E3, 0x0000002F, 0x00000084,
                     0x00000053, 0x000000D1, 0x00000000, 0x000000ED,
                     0x00000020, 0x000000FC, 0x000000B1, 0x0000005B,
                     0x0000006A, 0x000000CB, 0x000000BE, 0x00000039,
                     0x0000004A, 0x0000004C, 0x00000058, 0x000000CF,
                     0x000000D0, 0x000000EF, 0x000000AA, 0x000000FB,
                     0x00000043, 0x0000004D, 0x00000033, 0x00000085,
                     0x00000045, 0x000000F9, 0x00000002, 0x0000007F,
                     0x00000050, 0x0000003C, 0x0000009F, 0x000000A8,
                     0x00000051, 0x000000A3, 0x00000040, 0x0000008F,
                     0x00000092, 0x0000009D, 0x00000038, 0x000000F5,
                     0x000000BC, 0x000000B6, 0x000000DA, 0x00000021,
                     0x00000010, 0x000000FF, 0x000000F3, 0x000000D2,
                     0x000000CD, 0x0000000C, 0x00000013, 0x000000EC,
                     0x0000005F, 0x00000097, 0x00000044, 0x00000017,
                     0x000000C4, 0x000000A7, 0x0000007E, 0x0000003D,
                     0x00000064, 0x0000005D, 0x00000019, 0x00000073,
                     0x00000060, 0x00000081, 0x0000004F, 0x000000DC,
                     0x00000022, 0x0000002A, 0x00000090, 0x00000088,
                     0x00000046, 0x000000EE, 0x000000B8, 0x00000014,
                     0x000000DE, 0x0000005E, 0x0000000B, 0x000000DB,
                     0x000000E0, 0x00000032, 0x0000003A, 0x0000000A,
                     0x00000049, 0x00000006, 0x00000024, 0x0000005C,
                     0x000000C2, 0x000000D3, 0x000000AC, 0x00000062,
                     0x00000091, 0x00000095, 0x000000E4, 0x00000079,
                     0x000000E7, 0x000000C8, 0x00000037, 0x0000006D,
                     0x0000008D, 0x000000D5, 0x0000004E, 0x000000A9,
                     0x0000006C, 0x00000056, 0x000000F4, 0x000000EA,
                     0x00000065, 0x0000007A, 0x000000AE, 0x00000008,
                     0x000000BA, 0x00000078, 0x00000025, 0x0000002E,
                     0x0000001C, 0x000000A6, 0x000000B4, 0x000000C6,
                     0x000000E8, 0x000000DD, 0x00000074, 0x0000001F,
                     0x0000004B, 0x000000BD, 0x0000008B, 0x0000008A,
                     0x00000070, 0x0000003E, 0x000000B5, 0x00000066,
                     0x00000048, 0x00000003, 0x000000F6, 0x0000000E,
                     0x00000061, 0x00000035, 0x00000057, 0x000000B9,
                     0x00000086, 0x000000C1, 0x0000001D, 0x0000009E,
                     0x000000E1, 0x000000F8, 0x00000098, 0x00000011,
                     0x00000069, 0x000000D9, 0x0000008E, 0x00000094,
                     0x0000009B, 0x0000001E, 0x00000087, 0x000000E9,
                     0x000000CE, 0x00000055, 0x00000028, 0x000000DF,
                     0x0000008C, 0x000000A1, 0x00000089, 0x0000000D,
                     0x000000BF, 0x000000E6, 0x00000042, 0x00000068,
                     0x00000041, 0x00000099, 0x0000002D, 0x0000000F,
                     0x000000B0, 0x00000054, 0x000000BB, 0x00000016};


  // Key Expansion on CPU because it is not parallelizable
  AES_set_encrypt_key(key, 128, &expandedKey);


  // Load the QPU code
  load_qpu_code(QPU_ENC_CODEFILE, localQPUCode, QPU_ENC_CODE_BYTECOUNT);


  // Open mailbox for GPU communication and enable GPU
  int32_t mailbox = mbox_open();

  if (qpu_enable(mailbox, 1)) {
    fprintf(stderr, "ERROR. Unable to enable GPU. Executed as superuser?\n");
    return -1;
  }

  uint32_t totalBytecount =
    QPU_ENC_CODE_BYTECOUNT +   // Bytecount of QPU Code
    (SBOX_WORDSIZE + KEY_WORDSIZE + TEXT_WORDSIZE * NUM_QPUS) * sizeof(uint32_t) + // Bytecount of data
    NUM_UNIFORMS * sizeof(uint32_t) +   // Bytecount of Uniforms
    NUM_MESSAGES * sizeof(uint32_t);    // Bytecount of Messages

  // Allocate and lock memory on GPU, then create a mirror memory on CPU
  uint32_t gpu_MemoryHandle = mem_alloc(mailbox, totalBytecount, 4096, GPU_MEM_FLAG);
  uint32_t gpu_MemoryBase = mem_lock(mailbox, gpu_MemoryHandle);
  void*    cpu_MemoryBase = mapmem(gpu_MemoryBase + GPU_MEM_MAP, totalBytecount);

  struct EncGPUMemoryMap *cpu_map = (struct EncGPUMemoryMap*) cpu_MemoryBase;

  // Optionally memset.
  // memset(cpu_map, 0x0, sizeof(struct EncGPUMemoryMap));
  unsigned vc_code = gpu_MemoryBase + offsetof(struct EncGPUMemoryMap, qpuCode);
  unsigned vc_data = gpu_MemoryBase + offsetof(struct EncGPUMemoryMap, data);
  unsigned vc_uni  = gpu_MemoryBase + offsetof(struct EncGPUMemoryMap, uniforms);
  unsigned vc_msg  = gpu_MemoryBase + offsetof(struct EncGPUMemoryMap, messages);

  memcpy(cpu_map->qpuCode, localQPUCode, QPU_ENC_CODE_BYTECOUNT);
  memcpy(cpu_map->data, sbox, SBOX_WORDSIZE*sizeof(uint32_t));
  memcpy(cpu_map->data+SBOX_WORDSIZE, expandedKey.rd_key, KEY_WORDSIZE*sizeof(uint32_t));
  memcpy(cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, text, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);

  for (int i=0; i < NUM_QPUS; i++) {
    cpu_map->uniforms[i*NUM_UNIFORMS+0] = vc_data;         // data (address of sbox)
    cpu_map->uniforms[i*NUM_UNIFORMS+1] = vc_data + SBOX_WORDSIZE*sizeof(uint32_t);         // address of key
    cpu_map->uniforms[i*NUM_UNIFORMS+2] = vc_data + SBOX_WORDSIZE*sizeof(uint32_t) + KEY_WORDSIZE*sizeof(uint32_t) + TEXT_WORDSIZE*sizeof(uint32_t)*i; // address of text

    cpu_map->messages[i*NUM_MESSAGES+0] = vc_uni + NUM_UNIFORMS*sizeof(uint32_t)*i;
    cpu_map->messages[i*NUM_MESSAGES+1] = vc_code;
  }


  // Start execution
  execute_qpu(mailbox, NUM_QPUS, vc_msg, 1, 10000);

  // Fetch the results
  memcpy(text, cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);


  // Begin transmit and fetch of further blocks
  for (int block = 1; block < textBytecount/192; block++) {
    qpu_enable(mailbox, 0);
    qpu_enable(mailbox, 1);

    memcpy(cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, text+block*TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);

    execute_qpu(mailbox, NUM_QPUS, vc_msg, 1, 10000);

    memcpy(text+block*TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS, cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);
  }


  // Free memory and close pipes
  unmapmem(cpu_MemoryBase, totalBytecount);
  mem_unlock(mailbox, gpu_MemoryHandle);
  mem_free(mailbox, gpu_MemoryHandle);
  qpu_enable(mailbox, 0);
  mbox_close(mailbox);


  return 1;
}



/* Decrypts 'text' with 'key' according to AES128 in ECB mode on the RPi GPU.
 * On success, the ciphertext in 'text' is replaced with the corresponding
 * plaintext and the function returns 1.
 * On failure, the function exits or returns -1.
 */
int32_t aes128_ecb_gpu_decrypt(uint8_t *text, const size_t textBytecount, const uint8_t *key)
{
  uint32_t localQPUCode[QPU_DEC_CODE_BYTECOUNT / sizeof(uint32_t)];
  AES_KEY  expandedKey;

  uint32_t sbox[] = {0x00000052, 0x00000009, 0x0000006A, 0x000000D5,
                     0x00000030, 0x00000036, 0x000000A5, 0x00000038,
                     0x000000BF, 0x00000040, 0x000000A3, 0x0000009E,
                     0x00000081, 0x000000F3, 0x000000D7, 0x000000FB,
                     0x0000007C, 0x000000E3, 0x00000039, 0x00000082,
                     0x0000009B, 0x0000002F, 0x000000FF, 0x00000087,
                     0x00000034, 0x0000008E, 0x00000043, 0x00000044,
                     0x000000C4, 0x000000DE, 0x000000E9, 0x000000CB,
                     0x00000054, 0x0000007B, 0x00000094, 0x00000032,
                     0x000000A6, 0x000000C2, 0x00000023, 0x0000003D,
                     0x000000EE, 0x0000004C, 0x00000095, 0x0000000B,
                     0x00000042, 0x000000FA, 0x000000C3, 0x0000004E,
                     0x00000008, 0x0000002E, 0x000000A1, 0x00000066,
                     0x00000028, 0x000000D9, 0x00000024, 0x000000B2,
                     0x00000076, 0x0000005B, 0x000000A2, 0x00000049,
                     0x0000006D, 0x0000008B, 0x000000D1, 0x00000025,
                     0x00000072, 0x000000F8, 0x000000F6, 0x00000064,
                     0x00000086, 0x00000068, 0x00000098, 0x00000016,
                     0x000000D4, 0x000000A4, 0x0000005C, 0x000000CC,
                     0x0000005D, 0x00000065, 0x000000B6, 0x00000092,
                     0x0000006C, 0x00000070, 0x00000048, 0x00000050,
                     0x000000FD, 0x000000ED, 0x000000B9, 0x000000DA,
                     0x0000005E, 0x00000015, 0x00000046, 0x00000057,
                     0x000000A7, 0x0000008D, 0x0000009D, 0x00000084,
                     0x00000090, 0x000000D8, 0x000000AB, 0x00000000,
                     0x0000008C, 0x000000BC, 0x000000D3, 0x0000000A,
                     0x000000F7, 0x000000E4, 0x00000058, 0x00000005,
                     0x000000B8, 0x000000B3, 0x00000045, 0x00000006,
                     0x000000D0, 0x0000002C, 0x0000001E, 0x0000008F,
                     0x000000CA, 0x0000003F, 0x0000000F, 0x00000002,
                     0x000000C1, 0x000000AF, 0x000000BD, 0x00000003,
                     0x00000001, 0x00000013, 0x0000008A, 0x0000006B,
                     0x0000003A, 0x00000091, 0x00000011, 0x00000041,
                     0x0000004F, 0x00000067, 0x000000DC, 0x000000EA,
                     0x00000097, 0x000000F2, 0x000000CF, 0x000000CE,
                     0x000000F0, 0x000000B4, 0x000000E6, 0x00000073,
                     0x00000096, 0x000000AC, 0x00000074, 0x00000022,
                     0x000000E7, 0x000000AD, 0x00000035, 0x00000085,
                     0x000000E2, 0x000000F9, 0x00000037, 0x000000E8,
                     0x0000001C, 0x00000075, 0x000000DF, 0x0000006E,
                     0x00000047, 0x000000F1, 0x0000001A, 0x00000071,
                     0x0000001D, 0x00000029, 0x000000C5, 0x00000089,
                     0x0000006F, 0x000000B7, 0x00000062, 0x0000000E,
                     0x000000AA, 0x00000018, 0x000000BE, 0x0000001B,
                     0x000000FC, 0x00000056, 0x0000003E, 0x0000004B,
                     0x000000C6, 0x000000D2, 0x00000079, 0x00000020,
                     0x0000009A, 0x000000DB, 0x000000C0, 0x000000FE,
                     0x00000078, 0x000000CD, 0x0000005A, 0x000000F4,
                     0x0000001F, 0x000000DD, 0x000000A8, 0x00000033,
                     0x00000088, 0x00000007, 0x000000C7, 0x00000031,
                     0x000000B1, 0x00000012, 0x00000010, 0x00000059,
                     0x00000027, 0x00000080, 0x000000EC, 0x0000005F,
                     0x00000060, 0x00000051, 0x0000007F, 0x000000A9,
                     0x00000019, 0x000000B5, 0x0000004A, 0x0000000D,
                     0x0000002D, 0x000000E5, 0x0000007A, 0x0000009F,
                     0x00000093, 0x000000C9, 0x0000009C, 0x000000EF,
                     0x000000A0, 0x000000E0, 0x0000003B, 0x0000004D,
                     0x000000AE, 0x0000002A, 0x000000F5, 0x000000B0,
                     0x000000C8, 0x000000EB, 0x000000BB, 0x0000003C,
                     0x00000083, 0x00000053, 0x00000099, 0x00000061,
                     0x00000017, 0x0000002B, 0x00000004, 0x0000007E,
                     0x000000BA, 0x00000077, 0x000000D6, 0x00000026,
                     0x000000E1, 0x00000069, 0x00000014, 0x00000063,
                     0x00000055, 0x00000021, 0x0000000C, 0x0000007D};


  // Key Expansion on CPU because it is not parallelizable
  AES_set_encrypt_key(key, 128, &expandedKey);


  // Load the QPU code
  load_qpu_code(QPU_DEC_CODEFILE, localQPUCode, QPU_DEC_CODE_BYTECOUNT);


  // Open mailbox for GPU communication and enable GPU
  int32_t mailbox = mbox_open();

  if (qpu_enable(mailbox, 1)) {
    fprintf(stderr, "ERROR. Unable to enable GPU. Executed as superuser?\n");
    return -1;
  }

  uint32_t totalBytecount =
    QPU_DEC_CODE_BYTECOUNT +   // Bytecount of QPU Code
    (SBOX_WORDSIZE + KEY_WORDSIZE + TEXT_WORDSIZE * NUM_QPUS) * sizeof(uint32_t) + // Bytecount of data
    NUM_UNIFORMS * sizeof(uint32_t) +   // Bytecount of Uniforms
    NUM_MESSAGES * sizeof(uint32_t);    // Bytecount of Messages

  // Allocate and lock memory on GPU, then create a mirror memory on CPU
  uint32_t gpu_MemoryHandle = mem_alloc(mailbox, totalBytecount, 4096, GPU_MEM_FLAG);
  uint32_t gpu_MemoryBase = mem_lock(mailbox, gpu_MemoryHandle);
  void*    cpu_MemoryBase = mapmem(gpu_MemoryBase + GPU_MEM_MAP, totalBytecount);

  struct DecGPUMemoryMap *cpu_map = (struct DecGPUMemoryMap*) cpu_MemoryBase;

  // Optionally memset.
  // memset(cpu_map, 0x0, sizeof(struct DecGPUMemoryMap));
  unsigned vc_code = gpu_MemoryBase + offsetof(struct DecGPUMemoryMap, qpuCode);
  unsigned vc_data = gpu_MemoryBase + offsetof(struct DecGPUMemoryMap, data);
  unsigned vc_uni  = gpu_MemoryBase + offsetof(struct DecGPUMemoryMap, uniforms);
  unsigned vc_msg  = gpu_MemoryBase + offsetof(struct DecGPUMemoryMap, messages);

  memcpy(cpu_map->qpuCode, localQPUCode, QPU_DEC_CODE_BYTECOUNT);
  memcpy(cpu_map->data, sbox, SBOX_WORDSIZE*sizeof(uint32_t));
  memcpy(cpu_map->data+SBOX_WORDSIZE, expandedKey.rd_key, KEY_WORDSIZE*sizeof(uint32_t));
  memcpy(cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, text, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);

  for (int i=0; i < NUM_QPUS; i++) {
    cpu_map->uniforms[i*NUM_UNIFORMS+0] = vc_data;         // data (address of sbox)
    cpu_map->uniforms[i*NUM_UNIFORMS+1] = vc_data + SBOX_WORDSIZE*sizeof(uint32_t);         // address of key
    cpu_map->uniforms[i*NUM_UNIFORMS+2] = vc_data + SBOX_WORDSIZE*sizeof(uint32_t) + KEY_WORDSIZE*sizeof(uint32_t) + TEXT_WORDSIZE*sizeof(uint32_t)*i; // address of text

    cpu_map->messages[i*NUM_MESSAGES+0] = vc_uni + NUM_UNIFORMS*sizeof(uint32_t)*i;
    cpu_map->messages[i*NUM_MESSAGES+1] = vc_code;
  }


  // Start execution
  execute_qpu(mailbox, NUM_QPUS, vc_msg, 1, 10000);

  // Fetch the results
  memcpy(text, cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);


  // Begin transmit and fetch of further blocks
  for (int block = 1; block < textBytecount/192; block++) {
    qpu_enable(mailbox, 0);
    qpu_enable(mailbox, 1);

    memcpy(cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, text+block*TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);

    execute_qpu(mailbox, NUM_QPUS, vc_msg, 1, 10000);

    memcpy(text+block*TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS, cpu_map->data+SBOX_WORDSIZE+KEY_WORDSIZE, TEXT_WORDSIZE*sizeof(uint32_t)*NUM_QPUS);
  }


  // Free memory and close pipes
  unmapmem(cpu_MemoryBase, totalBytecount);
  mem_unlock(mailbox, gpu_MemoryHandle);
  mem_free(mailbox, gpu_MemoryHandle);
  qpu_enable(mailbox, 0);
  mbox_close(mailbox);


  return 1;
}



/* Reads the content of 'qpuCodeFile' into 'qpuCode'.
 */
void load_qpu_code(const char *qpuCodeFile, uint32_t *qpuCode, uint32_t bytecount)
{
  FILE *file_in = fopen(qpuCodeFile, "r");
  if (!file_in) {
    fprintf(stderr, "ERROR. Failed to open %s.\n", qpuCodeFile);
    exit(EXIT_FAILURE);
  }

  fread(qpuCode, sizeof(uint32_t), bytecount / sizeof(uint32_t) , file_in);
  fclose(file_in);
}

