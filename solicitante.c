// solicitante.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LINE 256

void enviar_solicitud(int fd, const char *linea) {
    write(fd, linea, strlen(linea));
    write(fd, "\n", 1); // enviar salto de línea
}

void modo_archivo(const char *nombre_archivo, const char *pipe_name) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (!archivo) {
        perror("Error abriendo archivo de solicitudes");
        exit(1);
    }

    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1) {
        perror("Error abriendo pipe");
        exit(1);
    }

    char linea[MAX_LINE];
    while (fgets(linea, sizeof(linea), archivo)) {
        enviar_solicitud(fd, linea);
        printf("Enviada solicitud: %s", linea);
        sleep(1); // para simular comportamiento real
    }

    fclose(archivo);
    close(fd);
}

void modo_menu(const char *pipe_name) {
    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1) {
        perror("Error abriendo pipe");
        exit(1);
    }

    char operacion;
    char nombre_libro[100];
    int isbn;
    char linea[MAX_LINE];

    while (1) {
        printf("Ingrese operación (P = préstamo, R = renovación, D = devolución, Q = salir): ");
        scanf(" %c", &operacion);

        if (operacion == 'Q') {
            snprintf(linea, sizeof(linea), "Q, Salir, 0");
            enviar_solicitud(fd, linea);
            break;
        }

        printf("Nombre del libro: ");
        scanf(" %[^\n]", nombre_libro);
        printf("ISBN: ");
        scanf("%d", &isbn);

        snprintf(linea, sizeof(linea), "%c, %s, %d", operacion, nombre_libro, isbn);
        enviar_solicitud(fd, linea);
    }

    close(fd);
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
        modo_archivo(archivo, pipe_name);
    } else {
        modo_menu(pipe_name);
    }

    return 0;
}
