# WGCA 2.2 AA Validator

Validador autocontenido de contraste WCAG AA entre color de texto y color de fondo.

No tiene dependencias externas y funciona con Python 3.10 o superior. El script principal es `contrast_aa_validator.py` y también se incluye una demo HTML (`index.html`) y un ejemplo de entrada por lotes (`colors.json`).

## Qué soporta

- `#hex` en formato corto y largo, con o sin canal alfa.
- `rgb()` y `rgba()`, con sintaxis antigua y moderna.
- `hsl()` y `hsla()`.
- `oklch()`.
- Nombres CSS comunes como `white`, `black`, `red`, `green`, `blue`, `transparent`, `gray`, `grey`, `yellow`, `cyan`, `aqua`, `magenta` y `fuchsia`.

## Uso básico

```bash
python contrast_aa_validator.py --fg "#111827" --bg "white"
```

Con `oklch()`:

```bash
python contrast_aa_validator.py \
  --fg "oklch(55% 0.18 245)" \
  --bg "oklch(98% 0.01 245)"
```

Indicando tamaño y peso de fuente:

```bash
python contrast_aa_validator.py \
  --fg "white" \
  --bg "oklch(55% 0.18 245)" \
  --font-size 16 \
  --font-weight 700
```

## Validación por lotes

Puedes pasar un JSON con varios casos:

```json
[
  {
    "name": "Botón primario",
    "fg": "white",
    "bg": "oklch(55% 0.18 245)",
    "font_size_px": 16,
    "font_weight": 700
  }
]
```

Ejecución:

```bash
python contrast_aa_validator.py --file colors.json
```

## Criterios AA

El script calcula el color efectivo en RGB, obtiene la luminancia relativa y evalúa la relación de contraste. Usa estos umbrales:

- Texto normal: mínimo `4.5:1`.
- Texto grande o negrita grande: mínimo `3.0:1`.

## Códigos de salida

- `0`: todos los casos pasan AA.
- `1`: al menos un caso falla AA.
- `2`: error de entrada o color no soportado.

## Integración

Esto permite usar el validador en CI/CD para revisar tokens de diseño, estilos de componentes o cualquier paleta antes de aceptar cambios.
