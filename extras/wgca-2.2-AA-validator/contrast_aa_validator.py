
#!/usr/bin/env python3
"""
contrast_aa_validator.py

Validador WCAG AA de contraste entre texto y fondo.

Soporta:
- #RGB, #RGBA, #RRGGBB, #RRGGBBAA
- rgb(), rgba() con sintaxis antigua o moderna
- hsl(), hsla()
- oklch()
- algunos nombres CSS comunes: white, black, red, green, blue, transparent, etc.

Uso:
  python contrast_aa_validator.py --fg "#1f2937" --bg "white"
  python contrast_aa_validator.py --fg "oklch(55% 0.18 245)" --bg "oklch(98% 0.01 245)"
  python contrast_aa_validator.py --file colores.json
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class Color:
    r: float  # 0..255
    g: float  # 0..255
    b: float  # 0..255
    a: float = 1.0  # 0..1

    def clamped(self) -> "Color":
        return Color(
            r=min(255, max(0, self.r)),
            g=min(255, max(0, self.g)),
            b=min(255, max(0, self.b)),
            a=min(1, max(0, self.a)),
        )

    def as_rgb_tuple(self) -> tuple[int, int, int]:
        c = self.clamped()
        return round(c.r), round(c.g), round(c.b)


NAMED_COLORS: dict[str, Color] = {
    "black": Color(0, 0, 0),
    "white": Color(255, 255, 255),
    "red": Color(255, 0, 0),
    "green": Color(0, 128, 0),
    "blue": Color(0, 0, 255),
    "yellow": Color(255, 255, 0),
    "cyan": Color(0, 255, 255),
    "aqua": Color(0, 255, 255),
    "magenta": Color(255, 0, 255),
    "fuchsia": Color(255, 0, 255),
    "gray": Color(128, 128, 128),
    "grey": Color(128, 128, 128),
    "transparent": Color(0, 0, 0, 0),
}


def parse_alpha(token: str | None) -> float:
    if not token:
        return 1.0

    token = token.strip()

    if token.endswith("%"):
        return float(token[:-1]) / 100

    return float(token)


def parse_rgb_channel(token: str) -> float:
    token = token.strip()

    if token.endswith("%"):
        return float(token[:-1]) * 255 / 100

    return float(token)


def parse_percent_or_number(token: str) -> float:
    token = token.strip()

    if token.endswith("%"):
        return float(token[:-1]) / 100

    return float(token)


def parse_hue(token: str) -> float:
    token = token.strip().lower()

    if token.endswith("deg"):
        return float(token[:-3])
    if token.endswith("turn"):
        return float(token[:-4]) * 360
    if token.endswith("rad"):
        return math.degrees(float(token[:-3]))
    if token.endswith("grad"):
        return float(token[:-4]) * 0.9

    return float(token)


def split_css_function_args(inside: str) -> tuple[list[str], str | None]:
    """
    Soporta:
      rgb(255, 0, 0)
      rgb(255 0 0 / 0.5)
      oklch(55% 0.18 245 / 80%)
    No evalúa calc(), var(), color-mix() ni light-dark().
    """
    normalized = inside.strip().replace(",", " ")

    if "/" in normalized:
        main, alpha = normalized.split("/", 1)
        alpha_token = alpha.strip().split()[0] if alpha.strip() else None
    else:
        main = normalized
        alpha_token = None

    parts = [p for p in main.split() if p]
    return parts, alpha_token


def parse_hex(value: str) -> Color:
    raw = value.strip().lstrip("#")

    if len(raw) == 3:
        r, g, b = [int(ch * 2, 16) for ch in raw]
        return Color(r, g, b)

    if len(raw) == 4:
        r, g, b, a = [int(ch * 2, 16) for ch in raw]
        return Color(r, g, b, a / 255)

    if len(raw) == 6:
        r = int(raw[0:2], 16)
        g = int(raw[2:4], 16)
        b = int(raw[4:6], 16)
        return Color(r, g, b)

    if len(raw) == 8:
        r = int(raw[0:2], 16)
        g = int(raw[2:4], 16)
        b = int(raw[4:6], 16)
        a = int(raw[6:8], 16) / 255
        return Color(r, g, b, a)

    raise ValueError(f"Hex inválido: {value}")


def parse_rgb_function(name: str, inside: str) -> Color:
    parts, alpha_token = split_css_function_args(inside)

    if len(parts) < 3:
        raise ValueError(f"{name} necesita 3 canales: {name}({inside})")

    alpha = parse_alpha(alpha_token)

    # rgba(255, 0, 0, 0.5), después de reemplazar comas, deja 4 tokens.
    if len(parts) >= 4 and alpha_token is None:
        alpha = parse_alpha(parts[3])

    return Color(
        parse_rgb_channel(parts[0]),
        parse_rgb_channel(parts[1]),
        parse_rgb_channel(parts[2]),
        alpha,
    ).clamped()


def hsl_to_rgb(h: float, s: float, l: float, alpha: float = 1.0) -> Color:
    h = (h % 360) / 360

    if s == 0:
        value = l * 255
        return Color(value, value, value, alpha).clamped()

    def hue_to_rgb(p: float, q: float, t: float) -> float:
        if t < 0:
            t += 1
        if t > 1:
            t -= 1
        if t < 1 / 6:
            return p + (q - p) * 6 * t
        if t < 1 / 2:
            return q
        if t < 2 / 3:
            return p + (q - p) * (2 / 3 - t) * 6
        return p

    q = l * (1 + s) if l < 0.5 else l + s - l * s
    p = 2 * l - q

    r = hue_to_rgb(p, q, h + 1 / 3)
    g = hue_to_rgb(p, q, h)
    b = hue_to_rgb(p, q, h - 1 / 3)

    return Color(r * 255, g * 255, b * 255, alpha).clamped()


def parse_hsl_function(name: str, inside: str) -> Color:
    parts, alpha_token = split_css_function_args(inside)

    if len(parts) < 3:
        raise ValueError(f"{name} necesita H, S y L: {name}({inside})")

    alpha = parse_alpha(alpha_token)

    if len(parts) >= 4 and alpha_token is None:
        alpha = parse_alpha(parts[3])

    h = parse_hue(parts[0])
    s = parse_percent_or_number(parts[1])
    l = parse_percent_or_number(parts[2])

    return hsl_to_rgb(h, s, l, alpha)


def linear_srgb_to_srgb_channel(channel: float) -> float:
    if channel <= 0.0031308:
        return 12.92 * channel

    return 1.055 * (channel ** (1 / 2.4)) - 0.055


def oklab_to_srgb(L: float, a: float, b: float, alpha: float = 1.0) -> Color:
    l_ = L + 0.3963377774 * a + 0.2158037573 * b
    m_ = L - 0.1055613458 * a - 0.0638541728 * b
    s_ = L - 0.0894841775 * a - 1.2914855480 * b

    l = l_ ** 3
    m = m_ ** 3
    s = s_ ** 3

    r_linear = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s
    g_linear = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s
    b_linear = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s

    r = linear_srgb_to_srgb_channel(r_linear) * 255
    g = linear_srgb_to_srgb_channel(g_linear) * 255
    b = linear_srgb_to_srgb_channel(b_linear) * 255

    return Color(r, g, b, alpha).clamped()


def parse_oklch_function(inside: str) -> Color:
    parts, alpha_token = split_css_function_args(inside)

    if len(parts) < 3:
        raise ValueError(f"oklch necesita L, C y h: oklch({inside})")

    L = parse_percent_or_number(parts[0])
    C = parse_percent_or_number(parts[1])
    h = math.radians(parse_hue(parts[2]))
    alpha = parse_alpha(alpha_token)

    if len(parts) >= 4 and alpha_token is None:
        alpha = parse_alpha(parts[3])

    a = C * math.cos(h)
    b = C * math.sin(h)

    return oklab_to_srgb(L, a, b, alpha)


def parse_color(value: str) -> Color:
    value = value.strip().lower()

    if value in NAMED_COLORS:
        return NAMED_COLORS[value]

    if value.startswith("#"):
        return parse_hex(value)

    match = re.fullmatch(r"([a-z-]+)\((.*)\)", value)
    if not match:
        raise ValueError(f"Color no soportado: {value}")

    name, inside = match.groups()

    if name in {"rgb", "rgba"}:
        return parse_rgb_function(name, inside)

    if name in {"hsl", "hsla"}:
        return parse_hsl_function(name, inside)

    if name == "oklch":
        return parse_oklch_function(inside)

    raise ValueError(
        f"Función CSS no soportada: {name}(). "
        "Este script no evalúa var(), calc(), color-mix(), light-dark() ni color(display-p3 ...)."
    )


def composite_over(foreground: Color, background: Color) -> Color:
    fg = foreground.clamped()
    bg = background.clamped()
    alpha = fg.a + bg.a * (1 - fg.a)

    if alpha == 0:
        return Color(255, 255, 255, 1)

    return Color(
        r=(fg.r * fg.a + bg.r * bg.a * (1 - fg.a)) / alpha,
        g=(fg.g * fg.a + bg.g * bg.a * (1 - fg.a)) / alpha,
        b=(fg.b * fg.a + bg.b * bg.a * (1 - fg.a)) / alpha,
        a=alpha,
    ).clamped()


def srgb_to_linear(channel_0_255: float) -> float:
    value = channel_0_255 / 255

    if value <= 0.04045:
        return value / 12.92

    return ((value + 0.055) / 1.055) ** 2.4


def relative_luminance(color: Color) -> float:
    c = color.clamped()

    r = srgb_to_linear(c.r)
    g = srgb_to_linear(c.g)
    b = srgb_to_linear(c.b)

    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast_ratio(foreground: Color, background: Color, page_background: Color | None = None) -> float:
    """
    Calcula contraste WCAG entre texto y fondo.

    Si hay alpha, primero se compone:
    1. fondo sobre page_background
    2. texto sobre el fondo resultante
    """
    page = page_background or Color(255, 255, 255, 1)

    effective_bg = composite_over(background, page)
    effective_fg = composite_over(foreground, effective_bg)

    l_fg = relative_luminance(effective_fg)
    l_bg = relative_luminance(effective_bg)

    lighter = max(l_fg, l_bg)
    darker = min(l_fg, l_bg)

    return (lighter + 0.05) / (darker + 0.05)


def is_large_text(font_size_px: float, font_weight: int) -> bool:
    return font_size_px >= 24 or (font_size_px >= 18.66 and font_weight >= 700)


def validate_pair(
    foreground_value: str,
    background_value: str,
    font_size_px: float = 16,
    font_weight: int = 400,
    name: str = "color-pair",
) -> dict[str, Any]:
    foreground = parse_color(foreground_value)
    background = parse_color(background_value)

    ratio = contrast_ratio(foreground, background)
    large_text = is_large_text(font_size_px, font_weight)
    required = 3.0 if large_text else 4.5

    return {
        "name": name,
        "foreground": foreground_value,
        "background": background_value,
        "foreground_rgb": foreground.as_rgb_tuple(),
        "background_rgb": background.as_rgb_tuple(),
        "font_size_px": font_size_px,
        "font_weight": font_weight,
        "large_text": large_text,
        "contrast": ratio,
        "required": required,
        "passes_AA": ratio >= required,
    }


def print_result(result: dict[str, Any]) -> None:
    status = "PASA AA" if result["passes_AA"] else "FALLA AA"

    print(f"\n[{status}] {result['name']}")
    print(f"  FG: {result['foreground']} -> rgb{result['foreground_rgb']}")
    print(f"  BG: {result['background']} -> rgb{result['background_rgb']}")
    print(f"  Texto grande: {'sí' if result['large_text'] else 'no'}")
    print(f"  Contraste: {result['contrast']:.2f}:1")
    print(f"  Requerido AA: {result['required']:.1f}:1")


def load_json_cases(path: Path) -> list[dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))

    if not isinstance(data, list):
        raise ValueError("El JSON debe ser una lista de casos.")

    return data


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validador WCAG AA de contraste entre texto y fondo."
    )

    parser.add_argument("--fg", help="Color de texto. Ej: '#111827', 'white', 'oklch(55% 0.18 245)'")
    parser.add_argument("--bg", help="Color de fondo. Ej: '#ffffff', 'rgb(255 255 255)'")
    parser.add_argument("--font-size", type=float, default=16, help="Tamaño de fuente en px. Default: 16")
    parser.add_argument("--font-weight", type=int, default=400, help="Peso de fuente. Default: 400")
    parser.add_argument("--name", default="manual", help="Nombre del caso.")
    parser.add_argument("--file", type=Path, help="JSON con casos a validar.")

    args = parser.parse_args()

    try:
        if args.file:
            cases = load_json_cases(args.file)

            has_failures = False

            for index, case in enumerate(cases, start=1):
                result = validate_pair(
                    foreground_value=case["fg"],
                    background_value=case["bg"],
                    font_size_px=float(case.get("font_size_px", 16)),
                    font_weight=int(case.get("font_weight", 400)),
                    name=case.get("name", f"caso-{index}"),
                )

                print_result(result)
                has_failures = has_failures or not result["passes_AA"]

            return 1 if has_failures else 0

        if not args.fg or not args.bg:
            parser.error("Debes pasar --fg y --bg, o usar --file colores.json")

        result = validate_pair(
            foreground_value=args.fg,
            background_value=args.bg,
            font_size_px=args.font_size,
            font_weight=args.font_weight,
            name=args.name,
        )

        print_result(result)

        return 0 if result["passes_AA"] else 1

    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
