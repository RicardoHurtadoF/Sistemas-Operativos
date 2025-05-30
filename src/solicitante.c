// solicitante.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_LINE 256

void enviar_solicitud(int fd, const char *linea) {
    write(fd, linea, strlen(linea));
    write(fd, "\n", 1);
}

void recibir_respuesta(const char *pipe_respuesta) {
    int fd_resp = open(pipe_respuesta, O_RDONLY);
    if (fd_resp == -1) {
        perror("Error abriendo pipe de respuesta");
        return;
    }

    char respuesta[MAX_LINE];
    ssize_t n = read(fd_resp, respuesta, sizeof(respuesta) - 1);
    if (n > 0) {
        respuesta[n] = '\0';
        printf("Respuesta del receptor: %s\n", respuesta);
    }

    close(fd_resp);
}

void modo_menu(const char *pipe_name) {
    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1) {
        perror("Error abriendo pipe");
        exit(1);
    }

    char pipe_respuesta[100];
    snprintf(pipe_respuesta, sizeof(pipe_respuesta), "pipe_respuesta_%d", getpid());
    mkfifo(pipe_respuesta, 0666);

    char operacion;
    char nombre_libro[100];
    int isbn;
    char linea[MAX_LINE];

    while (1) {
        printf("Ingrese operación (P = préstamo, R = renovación, D = devolución, Q = salir): ");
        scanf(" %c", &operacion);

        if (operacion == 'Q') {
            snprintf(linea, sizeof(linea), "Q, Salir, 0, %s", pipe_respuesta);
            enviar_solicitud(fd, linea);
            break;
        }

        printf("Nombre del libro: ");
        scanf(" %[^\n]", nombre_libro);
        printf("ISBN: ");
        scanf("%d", &isbn);

        snprintf(linea, sizeof(linea), "%c, %s, %d, %s", operacion, nombre_libro, isbn, pipe_respuesta);
        enviar_solicitud(fd, linea);
        recibir_respuesta(pipe_respuesta);
    }

    close(fd);
    unlink(pipe_respuesta); // elimina el pipe cuando termina
}

int main(int argc, char *argv[]) {
    const char *archivo = NULL;
    const char *pipe_name = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            archivo = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            pipe_name = argv[++i];
        }
    }

    if (!pipe_name) {
        fprintf(stderr, "Uso: %s [-i archivo.txt] -p pipe_name\n", argv[0]);
        return 1;
    }

    if (archivo) {
        printf(" Modo archivo no soporta respuesta aún. Usa -p sin -i para modo menú.\n");
    } else {
        modo_menu(pipe_name);
    }

    return 0;
}
