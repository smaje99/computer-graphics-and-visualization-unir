# Informática Gráfica y Visualización

Monorepo de la asignatura de UNIR organizado por prácticas y ampliaciones técnicas. Todos los proyectos compilan en Linux con `gcc`, OpenGL clásico, X11/GLX y `make`.

## Estructura

- `labs/lab-01-reloj-analogico/`: práctica histórica del reloj analógico estático.
- `labs/lab-02-proyecciones-3d/`: Trabajo con cuatro proyecciones simultáneas de un mismo cubo.
- `extras/calculo-vectorial-figura-1/`: rescate independiente de la Figura 1 del laboratorio de Cálculo Vectorial.
- `extras/primitivas/`: demo educativa de primitivas de salida con OpenGL clásico y GLUT.
- `extras/wgca-2.2-AA-validator/`: validador independiente de contraste WCAG AA y demo HTML asociada.

## Requisitos

- `gcc`
- `make`
- `libgl1-mesa-dev`
- `libx11-dev`
- `pdflatex` para compilar los informes

En Debian o Ubuntu:

```bash
sudo apt install build-essential libgl1-mesa-dev libx11-dev texlive-latex-base texlive-fonts-recommended
```

## Targets raíz

Compilar todos los ejecutables:

```bash
make
```

Compilar proyectos concretos:

```bash
make lab1
make lab2
make vectorial
```

Compilar informes:

```bash
make reports
```

Limpiar binarios y artefactos de LaTeX:

```bash
make clean
```

## Notas

- El trabajo principal entregable está en `labs/lab-02-proyecciones-3d/` y concentra toda la implementación en `main.c`, como exige el enunciado.
- El informe del trabajo se mantiene en LaTeX y se genera como PDF final del repositorio, aunque la nota original del enunciado pedía formatos editables.
- El extra de cálculo vectorial queda aislado para no contaminar la entrega del trabajo.
- El extra de primitivas requiere `freeglut3-dev` además de las bibliotecas habituales de OpenGL.
- `extras/wgca-2.2-AA-validator/` es una utilidad autocontenida en Python para revisar contraste entre texto y fondo antes de aceptar tokens o componentes de diseño. No está conectada a los targets raíz de `make`; se ejecuta de forma directa con `python`.
