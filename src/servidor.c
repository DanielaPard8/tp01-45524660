#define _POSIX_C_SOURCE 200809L

#include "protocolo.h"
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

// atiende un cliente hasta que se desconecte o mande QUIT
// lee del socket hasta encontrar un \n o que se corte la conexion
void atender(int fd) {
    char buf[TAM_BUF];
    char linea[MAX_LINEA + 1];
    char resp[TAM_RESP];
    int largo = 0;        // bytes que tengo acumulados en linea
    int descartando = 0;  // si me pase del limite tiro todo hasta el \n
    int contador = 0;     // comandos validos de ESTA conexion

    ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buf[i];

            if (descartando) {
                if (c == '\n') descartando = 0;
                continue;
            }

            if (c == '\n') {
                linea[largo] = '\0';
                int cerrar = procesar_comando(linea, resp, &contador);
                enviar_todo(fd, resp, strlen(resp));
                largo = 0;
                if (cerrar) return;
                continue;
            }

            // el protocolo no acepta nulos
            if (c == '\0') continue;

            if (largo >= MAX_LINEA) {
                const char *err = "ERROR LINE_TOO_LONG\n";
                enviar_todo(fd, err, strlen(err));
                descartando = 1;
                largo = 0;
                continue;
            }

            linea[largo++] = c;
        }
    }

    if (n < 0 && errno != EINTR) perror("recv");
    // si n == 0 el cliente cerro sin QUIT y salimos igual
}

///////////////////// MAIN ///////////////////////

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <puerto>\n", argv[0]);
        return 1;
    }

    // valido el puerto
    char *fin;
    long puerto = strtol(argv[1], &fin, 10);
    if (*fin != '\0' || puerto < 1024 || puerto > 65535) {
        fprintf(stderr, "puerto invalido\n");
        return 1;
    }

    // getaddrinfo con AI_PASSIVE y nodo NULL me da 0.0.0.0
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPV4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // todas las interfaces

    int rc = getaddrinfo(NULL, argv[1], &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return 1;
    }

    int fd_escucha = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd_escucha < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    // uso SO_REUSEADDR para no tener que esperar a que se libere el puerto
    int uno = 1;
    if (setsockopt(fd_escucha, SOL_SOCKET, SO_REUSEADDR, &uno, sizeof(uno)) < 0) {
        perror("setsockopt");
        freeaddrinfo(res);
        return 1;
    }

    if (bind(fd_escucha, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        close(fd_escucha);
        freeaddrinfo(res);
        return 1;
    }

    freeaddrinfo(res);

// sin esto, si el cliente cerro y mandamos algo, el proceso muere
    if (listen(fd_escucha, 5) < 0) {
        perror("listen");
        close(fd_escucha);
        return 1;
    }

    fprintf(stderr, "servidor escuchando en el puerto %s\n", argv[1]);

    // acepto un cliente por vez 
    while (1) {
        struct sockaddr_storage dir;
        socklen_t tam = sizeof(dir);
        int fd_cli = accept(fd_escucha, (struct sockaddr *)&dir, &tam);
        if (fd_cli < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        atender(fd_cli);
        close(fd_cli);   // solo cierro al cliente, el de escucha sigue
    }

    close(fd_escucha);
    return 0;
}