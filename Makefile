# Makefile para el proyecto Sistema de Préstamo de Libros
# Definimos el compilador a usar (gcc)
CC = gcc

# Opciones de compilación:
# -Wall: activa la mayoría de las advertencias
# -Wextra: advertencias adicionales
# -pedantic: avisos sobre estándares estrictos de C
# -std=c11: usamos estándar C11
# -g: añade información para depuración (debugging)
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -D_POSIX_C_SOURCE=200809L

# Flags para el linker (enlazador).
# En receptor usamos pthread, por eso incluimos -lpthread.
LDFLAGS = -lpthread

# Nombres de los ejecutables que vamos a generar
RECEPTOR = receptor
SOLICITANTE = solicitante

# Archivos fuente de cada programa
SRC_RECEPTOR = receptor.c
SRC_SOLICITANTE = solicitante.c

# PHONY indica que estas "tareas" no corresponden a archivos reales
.PHONY: all clean run-receptor run-solicitante

# Regla por defecto, compila ambos programas
all: $(RECEPTOR) $(SOLICITANTE)

# Cómo compilar receptor:
# $@ representa el objetivo (receptor)
# $^ representa todos los prerequisitos (receptor.c)
$(RECEPTOR): $(SRC_RECEPTOR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Cómo compilar solicitante (sin pthread)
$(SOLICITANTE): $(SRC_SOLICITANTE)
	$(CC) $(CFLAGS) -o $@ $^

# Limpia los ejecutables, archivos objeto y pipes temporales
clean:
	rm -f $(RECEPTOR) $(SOLICITANTE) *.o pipe_receptor pipe_respuesta_*

# Atajo para ejecutar el receptor directamente con ./receptor
run-receptor: $(RECEPTOR)
	./$(RECEPTOR) -p pipe_receptor -f Libros.txt

# Atajo para ejecutar el solicitante en modo menú
run-solicitante: $(SOLICITANTE)
	@test -p pipe_receptor || (echo "Error: pipe_receptor no existe. Ejecuta primero receptor." && exit 1)
	./$(SOLICITANTE) -p pipe_receptor

# Regla para probar el sistema completo usando archivos predefinidos
run-test: $(RECEPTOR) $(SOLICITANTE)
	./$(RECEPTOR) -p pipe_receptor -f libros.txt & \
	sleep 1 && \
	./$(SOLICITANTE) -i input_solicitudes.txt -p pipe_receptor
