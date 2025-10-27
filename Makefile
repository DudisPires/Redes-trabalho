CC = gcc
CFLAGS = -Wall

all: meu_navegador meu_servidor

meu_navegador: cliente_http.c
	$(CC) $(CFLAGS) cliente_http.c -o meu_navegador

meu_servidor: servidor_http.c
	$(CC) $(CFLAGS) servidor_http.c -o meu_servidor

clean:
	rm -f meu_navegador meu_servidor
