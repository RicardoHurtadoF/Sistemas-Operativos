// solicitante.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_LINE 256

void enviar_solicitud(FILE *fp_pipe, const char *linea) {
    fprintf(fp_pipe, "%s\n", linea);
    fflush(fp_pipe);  // para enviar inmediatamente
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

void modo_archivo(const char *pipe_name, const char *archivo) {
    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1) {
        perror("Error abriendo pipe");
        exit(1);
    }

    FILE *fp_pipe = fdopen(fd, "w");
    if (!fp_pipe) {
        perror("fdopen pipe");
        close(fd);
        exit(1);
    }

    FILE *fp_archivo = fopen(archivo, "r");
    if (!fp_archivo) {
        perror("Error abriendo archivo de solicitudes");
        fclose(fp_pipe);
        exit(1);
    }

    char linea[MAX_LINE];
    char pipe_respuesta[100];

    while (fgets(linea, sizeof(linea), fp_archivo)) {
        // Eliminar salto de línea
        linea[strcspn(linea, "\n")] = 0;

        // Crear pipe respuesta único para esta solicitud
        snprintf(pipe_respuesta, sizeof(pipe_respuesta), "pipe_respuesta_%d", getpid());
        mkfifo(pipe_respuesta, 0666);

        // Agregar pipe respuesta a la línea (remplazamos o agregamos)
        // Suponemos que la línea original no incluye pipe_respuesta, así que la reconstruimos:

        // Leer tipo, nombre, isbn (3 campos)
        char tipo;
        char nombre[100];
        int isbn;
        int campos = sscanf(linea, " %c , %[^,] , %d", &tipo, nombre, &isbn);
        if (campos != 3) {
            fprintf(stderr, "Formato inválido en línea: %s\n", linea);
            unlink(pipe_respuesta);
            continue;
        }

        char linea_con_pipe[MAX_LINE];
        snprintf(linea_con_pipe, sizeof(linea_con_pipe), "%c, %s, %d, %s", tipo, nombre, isbn, pipe_respuesta);

        enviar_solicitud(fp_pipe, linea_con_pipe);
        recibir_respuesta(pipe_respuesta);

        unlink(pipe_respuesta);
    }

    fclose(fp_archivo);
    fclose(fp_pipe);
}

void modo_menu(const char *pipe_name) {
    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1) {
        perror("Error abriendo pipe");
        exit(1);
    }

    FILE *fp_pipe = fdopen(fd, "w");
    if (!fp_pipe) {
        perror("fdopen pipe");
        close(fd);
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
            enviar_solicitud(fp_pipe, linea);
            break;
        }

        printf("Nombre del libro: ");
        scanf(" %[^\n]", nombre_libro);
        printf("ISBN: ");
        scanf("%d", &isbn);

        snprintf(linea, sizeof(linea), "%c, %s, %d, %s", operacion, nombre_libro, isbn, pipe_respuesta);
        enviar_solicitud(fp_pipe, linea);
        recibir_respuesta(pipe_respuesta);
    }

    fclose(fp_pipe);
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
        modo_archivo(pipe_name, archivo);
    } else {
        modo_menu(pipe_name);
    }

    return 0;
}


