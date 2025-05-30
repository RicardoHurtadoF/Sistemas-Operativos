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

void modo_archivo(const char *archivo_nombre, const char *pipe_name) {
    FILE *archivo = fopen(archivo_nombre, "r");
    if (!archivo) {
        perror("Error abriendo archivo de solicitudes");
        exit(1);
    }

    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1) {
        perror("Error abriendo pipe");
        fclose(archivo);
        exit(1);
    }

    char linea_raw[MAX_LINE];
    char linea_final[MAX_LINE];
    char tipo;
    char nombre[100];
    int isbn;

    char pipe_respuesta[100];
    snprintf(pipe_respuesta, sizeof(pipe_respuesta), "pipe_respuesta_%d", getpid());
    mkfifo(pipe_respuesta, 0666);

    while (fgets(linea_raw, sizeof(linea_raw), archivo)) {
        // Eliminar salto de línea
        linea_raw[strcspn(linea_raw, "\n")] = 0;

        // Validar formato básico
        int ok = sscanf(linea_raw, " %c , %[^,] , %d", &tipo, nombre, &isbn);
        if (ok != 3) {
            fprintf(stderr, "Línea inválida: %s\n", linea_raw);
            continue;
        }

        snprintf(linea_final, sizeof(linea_final), "%c, %s, %d, %s", tipo, nombre, isbn, pipe_respuesta);
        enviar_solicitud(fd, linea_final);
        recibir_respuesta(pipe_respuesta);
        sleep(1); // opcional: evitar que colapsen respuestas
    }

    close(fd);
    fclose(archivo);
    unlink(pipe_respuesta);
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
    unlink(pipe_respuesta);
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

