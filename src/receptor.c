// receptor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_LINE 256
#define BUFFER_SIZE 10

typedef struct {
    char tipo;                // P, R, D
    char nombre[100];
    int isbn;
    char pipe_respuesta[100]; // Pipe para responderle al PS
} Solicitud;

Solicitud buffer[BUFFER_SIZE];
int in = 0, out = 0;
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

int seguir_ejecutando = 1;

// Hilo que procesa R y D
void* hilo_auxiliar1(void* arg) {
    while (seguir_ejecutando) {
        pthread_mutex_lock(&mutex);
        while (count == 0 && seguir_ejecutando) {
            pthread_cond_wait(&not_empty, &mutex);
        }
        if (!seguir_ejecutando) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        Solicitud s = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);

        printf("Hilo 1 procesando: %c, %s, %d\n", s.tipo, s.nombre, s.isbn);
    }
    return NULL;
}

void* hilo_auxiliar2(void* arg) {
    while (1) {
        char comando;
        scanf(" %c", &comando);
        if (comando == 's') {
            printf("Terminando ejecución.\n");
            seguir_ejecutando = 0;
            pthread_cond_signal(&not_empty);
            break;
        } else if (comando == 'r') {
            printf("Generar reporte aquí.\n");
        }
    }
    return NULL;
}

void procesar_solicitud(const char* linea) {
    Solicitud s;
    sscanf(linea, " %c , %[^,] , %d , %s", &s.tipo, s.nombre, &s.isbn, s.pipe_respuesta);

    if (s.tipo == 'P') {
        // Procesar préstamo aquí (verificar disponibilidad, responder)
        printf("Procesar préstamo: %s (%d)\n", s.nombre, s.isbn);
    } else if (s.tipo == 'R' || s.tipo == 'D') {
        pthread_mutex_lock(&mutex);
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&not_full, &mutex);
        }
        buffer[in] = s;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
    } else if (s.tipo == 'Q') {
        printf("Solicitante terminó.\n");
    }
}

int main() {
    const char* pipe_name = "pipe_receptor";
    mkfifo(pipe_name, 0666);

    int fd = open(pipe_name, O_RDONLY);
    if (fd == -1) {
        perror("Error abriendo pipe receptor");
        exit(1);
    }

    pthread_t hilo1, hilo2;
    pthread_create(&hilo1, NULL, hilo_auxiliar1, NULL);
    pthread_create(&hilo2, NULL, hilo_auxiliar2, NULL);

    char linea[MAX_LINE];
    while (seguir_ejecutando && read(fd, linea, sizeof(linea)) > 0) {
        procesar_solicitud(linea);
    }

    close(fd);
    unlink(pipe_name);

    pthread_join(hilo1, NULL);
    pthread_join(hilo2, NULL);

    return 0;
}
