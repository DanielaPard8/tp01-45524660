#define _POSIX_C_SOURCE 200809L

#include "protocolo.h"
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

// lee del socket hasta encontrar un \n o que se corte la conexion
// devuelve: 0 = linea en out, 1 = el otro cerro, -1 = error 
int leer_linea(int fd, char *out, int max) {
    int i = 0;
    char c;
    while (i < max - 1) {
        ssize_t n = recv(fd, &c, 1, 0);    // leo de a 1 byte
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return 1;
        if (c == '\n') break;
        out[i++] = c;
    }
    out[i] = '\0';
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "uso: %s <host> <puerto>\n", argv[0]);
        return 1;
    }

    // sin esto, si el server cerro y mandamos algo, el proceso muere
    signal(SIGPIPE, SIG_IGN);

    // resuelvo el host
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // ipv4 o ipv6
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return 1;
    }

    // pruebo conectarme con cada direccion que devolvio
    int fd = -1;
    for (p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        fprintf(stderr, "no me pude conectar a %s:%s\n", argv[1], argv[2]);
        return 1;
    }

    char linea[MAX_LINEA + 2];
    char resp[MAX_LINEA + 1];

    // el prompt va a stderr para no ensuciar la salida que se evalua
    fprintf(stderr, "> ");    
    while (fgets(linea, sizeof(linea), stdin)) {
        int len = strlen(linea);

        // me aseguro que termine en \n (el protocolo lo pide)
        if (linea[len - 1] != '\n') {
            if (len < (int)sizeof(linea) - 1) {
                linea[len++] = '\n';
                linea[len] = '\0';
            } else {
                // si es demasiado larga tiro el resto
                int c;
                while ((c = getchar()) != '\n' && c != EOF) {}
                fprintf(stderr, "linea muy larga, la ignoro\n");
                fprintf(stderr, "> ");
                continue;
            }
        }

        if (enviar_todo(fd, linea, len) < 0) {
            perror("send");
            break;
        }

        int r = leer_linea(fd, resp, sizeof(resp));
        if (r == 1) {
            fprintf(stderr, "el server cerro la conexion\n");
            break;
        }
        if (r < 0) {
            perror("recv");
            break;
        }

        printf("%s\n", resp);
        fflush(stdout); // me aseguro que se imprima ya

        if (strcmp(resp, "OK BYE") == 0) break;
        fprintf(stderr, "> ");
    }

    close(fd);
    return 0;
}