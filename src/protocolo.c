#include "protocolo.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h> 

// repite send hasta mandar todo o que explote
int enviar_todo(int fd, const char *datos, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(fd, datos + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;   // se interrumpio, reintento
            return -1;
        }
        if (n == 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

// arma la rta segun el comando recibido
// la linea viene sin \n, la respuesta debe salir con el \n
int procesar_comando(const char *linea, char *resp, int *contador) {

    // linea vacia
    if (linea[0] == '\0') {
        snprintf(resp, TAM_RESP, "ERROR EMPTY_COMMAND\n");
        return 0;
    }

    if (strcmp(linea, "PING") == 0) {
        (*contador)++;
        snprintf(resp, TAM_RESP, "OK PONG\n");
        return 0;
    }

    if (strcmp(linea, "TIME") == 0) {
        (*contador)++;
        time_t ahora = time(NULL);
        struct tm *t = localtime(&ahora);
        char fecha[32];
        strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", t);
        snprintf(resp, TAM_RESP, "OK TIME %s\n", fecha);
        return 0;
    }

    if (strcmp(linea, "COUNT") == 0) {
        (*contador)++;
        snprintf(resp, TAM_RESP, "OK COUNT %d\n", *contador);
        return 0;
    }

    if (strcmp(linea, "QUIT") == 0) {
        (*contador)++;
        snprintf(resp, TAM_RESP, "OK BYE\n");
        return 1;   // esto le dice al server que cierre
    }

    // ECHO <texto>
    if (strncmp(linea, "ECHO", 4) == 0) {
        // ECHO solo
        if (linea[4] == '\0') {
            snprintf(resp, TAM_RESP, "ERROR INVALID_ARGUMENT\n");
            return 0;
        }
        // ECHO pegado a otra cosa, tipo "ECHOalgo"
        if (linea[4] != ' ') {
            snprintf(resp, TAM_RESP, "ERROR UNKNOWN_COMMAND\n");
            return 0;
        }
        // ECHO con espacio pero sin texto
        if (linea[5] == '\0') {
            snprintf(resp, TAM_RESP, "ERROR INVALID_ARGUMENT\n");
            return 0;
        }
        (*contador)++;
        snprintf(resp, TAM_RESP, "OK %s\n", linea + 5);
        return 0;
    }

    // no matcheo con nada
    snprintf(resp, TAM_RESP, "ERROR UNKNOWN_COMMAND\n");
    return 0;
}