# Extra: Cálculo Vectorial, Figura 1

Adaptación OpenGL de la Figura 1 del laboratorio de Cálculo Vectorial. El programa dibuja únicamente la superficie cuádrica principal como un hiperboloide de una hoja.

## Ecuación representada

\[
\frac{x^2}{9} + \frac{(y-3)^2}{4} - \frac{(z+1)^2}{9} = 1
\]

La parametrización usada en `main.c` es:

\[
x = 3 \cosh(v) \cos(u), \quad
y = 3 + 2 \cosh(v) \sin(u), \quad
z = -1 + 3 \sinh(v)
\]

## Compilación y ejecución

Desde esta carpeta:

```bash
make
./figura_1
```

Desde la raíz del monorepo:

```bash
make vectorial
```

## Notas

- La superficie se dibuja como malla alámbrica para mantenerla ligera y legible.
- Se incluyen ejes y una orientación fija de escena.
- No se representan las trazas de las Figuras 2 a 7 ni el resto de problemas del laboratorio original.
