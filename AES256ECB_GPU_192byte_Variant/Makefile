.POSIX:
.PHONY: clean

CC = gcc

CFLAGS = -Wall -Werror -std=c11 -lcrypto


INC = -I/opt/vc/src/hello_pi/hello_fft \
	-I./aes256_ecb_reference/ \
	-I./aes256_ecb_gpu/ \
	-I./aes_ecb_rijndael/ \


C_SOURCE = aes256_ecb_cpu_gpu_test.c \
	./aes256_ecb_reference/aes256_ecb_reference.c \
	./aes256_ecb_reference/aes_lib_ecb_reference.c \
	./aes256_ecb_gpu/aes256_ecb_gpu.c \
	./aes256_ecb_gpu/aes256_ecb_gpu_prePipe.c \
	./aes_ecb_rijndael/rijndael.c \
	/opt/vc/src/hello_pi/hello_fft/mailbox.c


aes256_ecb_cpu_qpu: $(C_SOURCE) ./aes256_ecb_gpu/aes256_ecb_gpu_encrypt.qasm
	$(CC) $(CFLAGS) $(C_SOURCE) $(INC); \
	./aes256_ecb_gpu/my_gpu_assembler ./aes256_ecb_gpu/aes256_ecb_gpu_encrypt.qasm -o ./aes256_ecb_gpu/aes256_ecb_gpu_encrypt.bin; \
	./aes256_ecb_gpu/my_gpu_assembler ./aes256_ecb_gpu/aes256_ecb_gpu_decrypt.qasm -o ./aes256_ecb_gpu/aes256_ecb_gpu_decrypt.bin


clean:
	rm -f a.out ./aes256_ecb_gpu/aes256_ecb_gpu_encrypt.bin ./aes256_ecb_gpu/aes256_ecb_gpu_decrypt.bin

