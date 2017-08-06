.POSIX:
.PHONY: clean

CC = gcc

CFLAGS = -Wall -Werror -std=c11 -lcrypto


INC = -I/opt/vc/src/hello_pi/hello_fft \
	-I./aes128_ecb_reference/ \
	-I./aes128_ecb_gpu/ \
	-I./aes_ecb_rijndael/ \
	-I./aes128_ecb_tiny/


C_SOURCE = aes128_ecb_cpu_gpu_test.c \
	./aes128_ecb_reference/aes128_ecb_reference.c \
	./aes128_ecb_reference/aes_lib_ecb_reference.c \
	./aes128_ecb_gpu/aes128_ecb_gpu.c \
	./aes128_ecb_gpu/aes128_ecb_gpu_prePipe.c \
	./aes_ecb_rijndael/rijndael.c \
	./aes128_ecb_tiny/aes.c \
	/opt/vc/src/hello_pi/hello_fft/mailbox.c


aes128_ecb_cpu_qpu: $(C_SOURCE) ./aes128_ecb_gpu/aes128_ecb_gpu_encrypt.qasm
	$(CC) $(CFLAGS) $(C_SOURCE) $(INC); \
	./aes128_ecb_gpu/my_gpu_assembler ./aes128_ecb_gpu/aes128_ecb_gpu_encrypt.qasm -o ./aes128_ecb_gpu/aes128_ecb_gpu_encrypt.bin; \
	./aes128_ecb_gpu/my_gpu_assembler ./aes128_ecb_gpu/aes128_ecb_gpu_decrypt.qasm -o ./aes128_ecb_gpu/aes128_ecb_gpu_decrypt.bin


clean:
	rm -f a.out ./aes128_ecb_gpu/aes128_ecb_gpu_encrypt.bin ./aes128_ecb_gpu/aes128_ecb_gpu_decrypt.bin

