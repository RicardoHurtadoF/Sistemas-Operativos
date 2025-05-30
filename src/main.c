#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#define PIPE_NAME "pipe_receptor"

void crear_pipe() {
    // Crea el pipe si no existe
    if (access(PIPE_NAME, F_OK) == -1) {
        if (mkfifo(PIPE_NAME, 0666) != 0) {
            perror("Error creando pipe");
            exit(1);
        }
    }
}

int main() {
    crear_pipe();

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        return 1;
    }

    if (pid == 0) {
        // Proceso hijo: receptor
        execl("./receptor", "receptor", PIPE_NAME, NULL);
        perror("Error ejecutando receptor");
        exit(1);
    }

    // Proceso padre: menú para llamar al solicitante
    int opcion;
    char archivo[100];

    while (1) {
        printf("\n--- Menú Principal ---\n");
        printf("1. Ejecutar solicitante en modo archivo\n");
        printf("2. Ejecutar solicitante en modo interactivo\n");
        printf("3. Salir\n");
        printf("Opción: ");
        scanf("%d", &opcion);

        if (opcion == 1) {
            printf("Nombre del archivo de solicitudes: ");
            scanf("%s", archivo);
            char *args[] = {"./solicitante", "-i", archivo, "-p", PIPE_NAME, NULL};
            if (fork() == 0) {
                execv(args[0], args);
                perror("Error ejecutando solicitante");
                exit(1);
            }
            wait(NULL); // Espera a que termine
            } else if (opcion == 2) {
              pid_t pid_receptor = fork();
              if (pid_receptor == 0) {
                  // Hijo: ejecuta receptor
                  char *args_receptor[] = {"./receptor", NULL};
                  execv(args_receptor[0], args_receptor);
                  printf("ejecutando receptor");
                  perror("Error ejecutando receptor");
                  exit(1);
              }

              // 2. Esperar un momento a que receptor esté listo
              sleep(1); // o usleep(500000) para medio segundo

              // 3. Lanzar solicitante (otro hijo)
              pid_t pid_solicitante = fork();
              if (pid_solicitante == 0) {
                  char *args_solicitante[] = {"./solicitante", "-p", PIPE_NAME, NULL};
                  execv(args_solicitante[0], args_solicitante);
                  perror("Error ejecutando solicitante");
                  exit(1);
              }

              waitpid(pid_solicitante, NULL, 0);

              waitpid(pid_receptor, NULL, 0); // espera al receptor si ya terminó
          } else if (opcion == 3) {
            printf("Saliendo...\n");
            break;
        } else {
            printf("Opción inválida.\n");
        }
    }

    return 0;
}
