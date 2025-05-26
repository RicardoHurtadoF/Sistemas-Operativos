# Makefile para el proyecto Sistema de Préstamo de Libros

# Compilador
CC = gcc

# Flags de compilación (puedes agregar -Wall para mostrar advertencias)
CFLAGS = -pthread

# Archivos fuente
SRC = solicitante.c receptor.c

# Archivos objeto (opcional si quieres usar .o)
OBJ_SOL = solicitante
OBJ_REC = receptor

# Regla por defecto: compilar ambos ejecutables
all: $(OBJ_SOL) $(OBJ_REC)

# Compilar solicitante
$(OBJ_SOL): solicitante.c
	$(CC) $(CFLAGS) solicitante.c -o solicitante

# Compilar receptor
$(OBJ_REC): receptor.c
	$(CC) $(CFLAGS) receptor.c -o receptor

# Limpiar archivos generados
clean:
	rm -f $(OBJ_SOL) $(OBJ_REC) *.o pipe_receptor salida.txt
