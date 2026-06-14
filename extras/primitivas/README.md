# Primitivas de salida con OpenGL en C

Proyecto educativo para estudiar primitivas de salida en informática gráfica:

- Línea por fuerza bruta.
- Línea con DDA.
- Línea con Bresenham.
- Rectángulos rellenos.
- Circunferencias con Bresenham.
- Círculos rellenos con segmentos horizontales.
- Doble buffer con GLUT/OpenGL.

## Dependencias en Linux

```bash
sudo apt update
sudo apt install build-essential freeglut3-dev mesa-utils
```

## Compilación rápida

```bash
gcc primitivas_opengl.c -o primitivas_opengl -lglut -lGLU -lGL -lm
./primitivas_opengl
```

## Controles

- `q` o `ESC`: salir.
- `r`: redibujar.

## Nota

El código usa OpenGL clásico / immediate mode para fines académicos. La intención es observar los algoritmos de rasterización de forma clara, no construir un motor gráfico moderno.
