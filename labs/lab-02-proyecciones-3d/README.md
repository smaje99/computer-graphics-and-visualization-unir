# Trabajo: Proyecciones 3D

Ventana OpenGL con cuatro vistas simultáneas del mismo cubo, cada una con una matriz de proyección distinta y sin mover el ojo de la posición por defecto `(0, 0, 0)`.

## Contenido

- `main.c`: única fuente de la práctica, como exige el enunciado.
- `Makefile`: compilación del programa y del informe.
- `captures/`: capturas usadas por el informe.
- `report/informe.tex`: memoria en LaTeX del trabajo.

## Orden de cuadrantes

- Superior izquierda: proyección ortográfica.
- Superior derecha: proyección gabinete.
- Inferior izquierda: perspectiva simétrica.
- Inferior derecha: perspectiva oblicua asimétrica.

## Compilación y ejecución

Desde esta carpeta:

```bash
make
./proyecciones_3d
```

Para compilar el informe:

```bash
make report
```

Desde la raíz del monorepo:

```bash
make lab2
make reports
```

## Notas de implementación

- No se usa `gluLookAt()` ni una cámara explícita.
- El cubo se desplaza sobre `-Z` y se rota con ángulos fijos para que las diferencias entre proyecciones sean visibles.
- Las matrices de proyección se construyen de forma explícita en el código.
