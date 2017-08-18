.POSIX:
.PHONY: clean

CC = gcc

CFLAGS = -Wall -Werror -std=c11 -lcrypto

INC = -I/opt/vc/src/hello_pi/hello_fft \

C_SOURCE = vpm_test.c \
	/opt/vc/src/hello_pi/hello_fft/mailbox.c

vpm_test: $(C_SOURCE) ./vpm_test.qasm
	$(CC) $(CFLAGS) $(C_SOURCE) $(INC); \
	./my_gpu_assembler ./vpm_test.qasm -o ./vpm_test.qbin

clean:
	rm -f a.out ./vpm_test.qbin
