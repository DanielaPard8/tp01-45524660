#ifndef PROTOCOLO_H
#define PROTOCOLO_H // evita que las definiciones se dupliquen

#include <stddef.h>

// tamaño maximo de linea y de respuesta
#define MAX_LINEA 1024
#define TAM_BUF   4096
#define TAM_RESP  (MAX_LINEA + 64)

// manda todos los bytes; devuelve 0 si salio todo
int enviar_todo(int fd, const char *datos, size_t len); 

// procesa una linea (sin \n), arma la respuesta y actualiza el contador
// devuelve 1 si hay QUIT
int procesar_comando(const char *linea, char *resp, int *contador); 

#endif
