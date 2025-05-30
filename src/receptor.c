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
#define MAX_LIBROS 100
#define MAX_EJEMPLARES 10

typedef struct {
    int numero;
    char estado; // 'D' o 'P'
    char fecha[20];
} Ejemplar;

typedef struct {
    char nombre[100];
    int isbn;
    int cantidad;
    Ejemplar ejemplares[MAX_EJEMPLARES];
} Libro;

typedef struct {
    char tipo;                // P, R, D
    char nombre[100];
    int isbn;
    char pipe_respuesta[100]; // Pipe para responderle al PS
} Solicitud;

Solicitud buffer[BUFFER_SIZE];
int in = 0, out = 0;
int count = 0;

Libro biblioteca[MAX_LIBROS];
int total_libros = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

int seguir_ejecutando = 1;

void cargar_base_datos(const char *archivo);
void guardar_base_datos(const char *archivo);

void* hilo_auxiliar1(void* arg) {
    (void)arg;

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

        int encontrado = 0;
        for (int i = 0; i < total_libros; i++) {
            if (strcmp(biblioteca[i].nombre, s.nombre) == 0 && biblioteca[i].isbn == s.isbn) {
                for (int j = 0; j < biblioteca[i].cantidad; j++) {
                    Ejemplar *e = &biblioteca[i].ejemplares[j];
                    
                    if (s.tipo == 'D' && e->estado == 'P') {
                        e->estado = 'D';
                        time_t t = time(NULL);
                        struct tm tm = *localtime(&t);
                        snprintf(e->fecha, sizeof(e->fecha), "%02d-%02d-%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
                        encontrado = 1;

                        int fd_resp = open(s.pipe_respuesta, O_WRONLY);
                        if (fd_resp != -1) {
                            char msg[256];
                            snprintf(msg, sizeof(msg), "✔️ Devolución registrada: %s (Ejemplar %d)\n", s.nombre, e->numero);
                            write(fd_resp, msg, strlen(msg));
                            close(fd_resp);
                        }
                        break;
                    }

                    if (s.tipo == 'R' && e->estado == 'P') {
                        time_t t = time(NULL);
                        struct tm tm = *localtime(&t);
                        snprintf(e->fecha, sizeof(e->fecha), "%02d-%02d-%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
                        encontrado = 1;

                        int fd_resp = open(s.pipe_respuesta, O_WRONLY);
                        if (fd_resp != -1) {
                            char msg[256];
                            snprintf(msg, sizeof(msg), "✔️ Renovación registrada: %s (Ejemplar %d)\n", s.nombre, e->numero);
                            write(fd_resp, msg, strlen(msg));
                            close(fd_resp);
                        }
                        break;
                    }
                }
                break;
            }
        }

        if (!encontrado) {
            int fd_resp = open(s.pipe_respuesta, O_WRONLY);
            if (fd_resp != -1) {
                char msg[256];
                snprintf(msg, sizeof(msg), "❌ %s falló: ejemplar no encontrado o no prestado\n", s.tipo == 'D' ? "Devolución" : "Renovación");
                write(fd_resp, msg, strlen(msg));
                close(fd_resp);
            }
        }
    }

    return NULL;
}


void* hilo_auxiliar2(void* arg) {
    (void)arg;
    while (1) {
        char comando;
        scanf(" %c", &comando);
        if (comando == 's') {
            printf("Terminando ejecución.\n");
            seguir_ejecutando = 0;
            pthread_cond_signal(&not_empty);
            break;
        } else if (comando == 'r') {
            printf("\n--- REPORTE DE LIBROS ---\n");
            for (int i = 0; i < total_libros; i++) {
                printf("%s, ISBN: %d, Ejemplares: %d\n", biblioteca[i].nombre, biblioteca[i].isbn, biblioteca[i].cantidad);
                for (int j = 0; j < biblioteca[i].cantidad; j++) {
                    Ejemplar e = biblioteca[i].ejemplares[j];
                    printf("  Ejemplar %d - %c (%s)\n", e.numero, e.estado, e.fecha);
                }
                printf("\n");
            }
        }
    }
    return NULL;
}

void procesar_solicitud(const char* linea) {
    Solicitud s;
    int res = sscanf(linea, " %c , %[^,] , %d , %s", &s.tipo, s.nombre, &s.isbn, s.pipe_respuesta);
    if (res != 4) {
        fprintf(stderr, "Línea mal formateada: %s\n", linea);
        return;
    }

    if (s.tipo == 'P') {
        int encontrado = 0;
        for (int i = 0; i < total_libros; i++) {
            if (strcmp(biblioteca[i].nombre, s.nombre) == 0 && biblioteca[i].isbn == s.isbn) {
                for (int j = 0; j < biblioteca[i].cantidad; j++) {
                    if (biblioteca[i].ejemplares[j].estado == 'D') {
                        biblioteca[i].ejemplares[j].estado = 'P';
                        time_t t = time(NULL);
                        struct tm tm = *localtime(&t);
                        snprintf(biblioteca[i].ejemplares[j].fecha, sizeof(biblioteca[i].ejemplares[j].fecha),
                                 "%02d-%02d-%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
                        encontrado = 1;
                        int fd_resp = open(s.pipe_respuesta, O_WRONLY);
                        if (fd_resp != -1) {
                            char respuesta[256];
                            snprintf(respuesta, sizeof(respuesta),
                                     "✔️ Préstamo exitoso: %s (Ejemplar %d)\n",
                                     s.nombre, biblioteca[i].ejemplares[j].numero);
                            write(fd_resp, respuesta, strlen(respuesta));
                            close(fd_resp);
                        }
                        break;
                    }
                }
                break;
            }
        }
        if (!encontrado) {
            int fd_resp = open(s.pipe_respuesta, O_WRONLY);
            if (fd_resp != -1) {
                char msg[] = "❌ No hay ejemplares disponibles o el libro no existe\n";
                write(fd_resp, msg, strlen(msg));
                close(fd_resp);
            }
        }

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
        seguir_ejecutando = 0;
        pthread_cond_signal(&not_empty);
    } else {
        printf("Tipo de solicitud desconocido: %c\n", s.tipo);
    }
}

void cargar_base_datos(const char *archivo) {
    FILE *f = fopen(archivo, "r");
    if (!f) {
        perror("Error abriendo bd_libros.txt");
        exit(1);
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), f)) {
        if (strlen(linea) < 5) continue;
        if (strchr(linea, ',')) {
            char nombre[100];
            int isbn, cantidad;
            sscanf(linea, " %[^,], %d, %d", nombre, &isbn, &cantidad);
            strcpy(biblioteca[total_libros].nombre, nombre);
            biblioteca[total_libros].isbn = isbn;
            biblioteca[total_libros].cantidad = cantidad;
            for (int i = 0; i < cantidad; i++) {
                fgets(linea, sizeof(linea), f);
                int num;
                char estado;
                char fecha[20];
                sscanf(linea, "%d, %c, %s", &num, &estado, fecha);
                biblioteca[total_libros].ejemplares[i].numero = num;
                biblioteca[total_libros].ejemplares[i].estado = estado;
                strcpy(biblioteca[total_libros].ejemplares[i].fecha, fecha);
            }
            total_libros++;
        }
    }
    fclose(f);
}

void guardar_base_datos(const char *archivo) {
    FILE *f = fopen(archivo, "w");
    if (!f) {
        perror("Error escribiendo salida.txt");
        return;
    }

    for (int i = 0; i < total_libros; i++) {
        fprintf(f, "%s, %d, %d\n", biblioteca[i].nombre, biblioteca[i].isbn, biblioteca[i].cantidad);
        for (int j = 0; j < biblioteca[i].cantidad; j++) {
            Ejemplar e = biblioteca[i].ejemplares[j];
            fprintf(f, "%d, %c, %s\n", e.numero, e.estado, e.fecha);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s -p pipe_name -f bd_libros.txt\n", argv[0]);
        exit(1);
    }

    const char *pipe_name = NULL;
    const char *bd_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            pipe_name = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            bd_file = argv[++i];
        }
    }

    if (!pipe_name || !bd_file) {
        fprintf(stderr, "Faltan argumentos obligatorios.\n");
        exit(1);
    }

    cargar_base_datos(bd_file);
    mkfifo(pipe_name, 0666);
    int fd = open(pipe_name, O_RDONLY);
    if (fd == -1) {
        perror("Error abriendo pipe receptor");
        exit(1);
    }

    FILE *fp = fdopen(fd, "r");
    if (!fp) {
        perror("Error en fdopen");
        close(fd);
        unlink(pipe_name);
        exit(1);
    }

    pthread_t hilo1, hilo2;
    pthread_create(&hilo1, NULL, hilo_auxiliar1, NULL);
    pthread_create(&hilo2, NULL, hilo_auxiliar2, NULL);

    char linea[MAX_LINE];
    while (seguir_ejecutando && fgets(linea, sizeof(linea), fp)) {
        linea[strcspn(linea, "\n")] = 0;
        procesar_solicitud(linea);
    }

    fclose(fp);
    unlink(pipe_name);
    pthread_join(hilo1, NULL);
    pthread_join(hilo2, NULL);
    guardar_base_datos("salida.txt");
    return 0;
}
