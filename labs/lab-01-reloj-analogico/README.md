# Laboratorio 1: Reloj analógico

Práctica histórica conservada dentro del monorepo. Implementa un reloj analógico estático en C con OpenGL clásico sobre X11/GLX.

## Contenido

- `solution.c`: implementación original del reloj.
- `Makefile`: compilación del ejecutable y del informe.
- `report/informe.tex`: informe de la práctica.
- `captures/reloj.png`: captura incluida en el informe.

## Compilación y ejecución

Desde esta carpeta:

```bash
make
./reloj_analogico
```

Para compilar el informe:

```bash
make report
```

Desde la raíz del monorepo también puedes usar:

```bash
make lab1
make reports
```

## Notas

- El reloj toma la hora una sola vez al iniciar, por lo que su representación permanece estática.
- La implementación sigue usando X11/GLX para mantener la compatibilidad con el entorno usado en la asignatura.
