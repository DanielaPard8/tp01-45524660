
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude

all: cliente servidor

cliente: src/cliente.c src/protocolo.c include/protocolo.h
	$(CC) $(CFLAGS) -o cliente src/cliente.c src/protocolo.c

servidor: src/servidor.c src/protocolo.c include/protocolo.h
	$(CC) $(CFLAGS) -o servidor src/servidor.c src/protocolo.c

clean:
	rm -f cliente servidor

test: all
	./tests/test_tp01.sh

.PHONY: all clean test
