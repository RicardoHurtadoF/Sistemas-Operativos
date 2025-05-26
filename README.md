# Sistemas-Operativos

## Proyecto: Sistema de Préstamo de Libros

**Curso:** Sistemas Operativos  
**Integrantes:**  
- Andrés Mauricio Raba  
- Ricardo Andrés Hurtado  

---

## 📌 Descripción

Este proyecto consiste en la implementación de un sistema de préstamo de libros utilizando conceptos de programación de sistemas en lenguaje C. Se hace uso de procesos, hilos (`pthreads`), comunicación entre procesos con `pipes`, sincronización con semáforos y el manejo de archivos como base de datos.

Objetivos principales

a. Resolver un problema utilizando procesos e hilos de la biblioteca POSIX.

b. Emplear mecanismos de sincronización de procesos y comunicación de procesos usando pipes.

c. Utilizar de forma correcta llamadas al sistema relacionadas con hilos y procesos.
## 🧩 Componentes del Proyecto

### 1. `solicitante.c`
- Simula el comportamiento de un usuario que realiza operaciones:
  - Préstamo (`P`)
  - Renovación (`R`)
  - Devolución (`D`)
  - Salida (`Q`)
- Puede funcionar leyendo peticiones desde un archivo (`-i archivo.txt`) o a través de un menú interactivo.
- Envía las solicitudes al receptor usando un pipe nominal.

### 2. `receptor.c`
- Recibe las solicitudes enviadas por uno o varios solicitantes.
- Procesa los préstamos directamente.
- Encola renovaciones y devoluciones para que sean procesadas por un hilo auxiliar.
- Tiene un segundo hilo que permite recibir comandos de consola:
  - `r`: imprimir reporte
  - `s`: terminar ejecución

### 3. Archivos auxiliares
- `libros.txt`: Base de datos de libros inicial.
- `input_solicitudes.txt`: Archivo de solicitudes de prueba.
- `Makefile`: Facilita la compilación del proyecto.
- `salida.txt`: Archivo donde se guarda el estado final de la base de datos tras la ejecución.

---

## 🧪 Compilación y Ejecución

### Compilar
