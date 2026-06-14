/*
 * Primitivas de salida con OpenGL en C
 * Tema: lineas, circunferencias y rellenos.
 *
 * Compilacion sugerida en Linux:
 *   gcc src/primitivas_opengl.c -o primitivas_opengl -lglut -lGLU -lGL -lm
 *
 * Dependencias:
 *   sudo apt install build-essential freeglut3-dev mesa-utils
 *
 * Teclas:
 *   ESC / q : salir
 *   r       : redibujar
 *
 * Nota academica:
 * Este programa usa OpenGL clasico / immediate mode para que los algoritmos
 * sean faciles de estudiar. En produccion moderna se recomienda usar pipelines
 * con buffers, shaders y APIs como OpenGL moderno, Vulkan o WebGPU.
 */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH  1240
#define HEIGHT 820

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Color;

static Color rgb(unsigned char r, unsigned char g, unsigned char b) {
    Color c = { r, g, b };
    return c;
}

static void set_color(Color color) {
    glColor3ub(color.r, color.g, color.b);
}

/*
 * En este ejemplo, el framebuffer real lo gestiona OpenGL/GLUT.
 * Nuestras primitivas producen puntos discretos con coordenadas enteras.
 * La proyeccion ortografica se configura para que 1 unidad logica ~= 1 pixel.
 */
static void plot_pixel(int x, int y) {
    glVertex2i(x, y);
}

static void draw_text(int x, int y, const char *text, Color color) {
    set_color(color);
    glRasterPos2i(x, y);

    for (const char *p = text; *p != '\0'; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
    }
}

static void draw_text_block(int x, int y, const char *text, Color color, int line_height) {
    const char *line_start = text;

    for (const char *p = text; ; ++p) {
        if (*p != '\n' && *p != '\0') {
            continue;
        }

        char line[256];
        size_t len = (size_t)(p - line_start);

        if (len >= sizeof(line)) {
            len = sizeof(line) - 1;
        }

        for (size_t i = 0; i < len; ++i) {
            line[i] = line_start[i];
        }
        line[len] = '\0';

        draw_text(x, y, line, color);

        if (*p == '\0') {
            break;
        }

        line_start = p + 1;
        y += line_height;
    }
}

/* ============================================================
 * 1. Linea por fuerza bruta
 * ============================================================ */

static void draw_line_bruteforce(int x1, int y1, int x2, int y2, Color color) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    set_color(color);
    glBegin(GL_POINTS);

    if (dx == 0 && dy == 0) {
        plot_pixel(x1, y1);
        glEnd();
        return;
    }

    if (dx == 0) {
        int y_start = y1 < y2 ? y1 : y2;
        int y_end = y1 > y2 ? y1 : y2;

        for (int y = y_start; y <= y_end; ++y) {
            plot_pixel(x1, y);
        }

        glEnd();
        return;
    }

    if (abs(dx) >= abs(dy)) {
        float m = (float)dy / (float)dx;
        int step = dx > 0 ? 1 : -1;

        for (int x = x1; x != x2 + step; x += step) {
            int y = (int)roundf((float)y1 + m * (float)(x - x1));
            plot_pixel(x, y);
        }
    } else {
        float inv_m = (float)dx / (float)dy;
        int step = dy > 0 ? 1 : -1;

        for (int y = y1; y != y2 + step; y += step) {
            int x = (int)roundf((float)x1 + inv_m * (float)(y - y1));
            plot_pixel(x, y);
        }
    }

    glEnd();
}

/* ============================================================
 * 2. Linea con DDA
 * ============================================================ */

static void draw_line_dda(int x1, int y1, int x2, int y2, Color color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    set_color(color);
    glBegin(GL_POINTS);

    if (steps == 0) {
        plot_pixel(x1, y1);
        glEnd();
        return;
    }

    float x_inc = (float)dx / (float)steps;
    float y_inc = (float)dy / (float)steps;

    float x = (float)x1;
    float y = (float)y1;

    for (int i = 0; i <= steps; ++i) {
        plot_pixel((int)roundf(x), (int)roundf(y));
        x += x_inc;
        y += y_inc;
    }

    glEnd();
}

/* ============================================================
 * 3. Linea con Bresenham generalizado
 * ============================================================ */

static void draw_line_bresenham(int x1, int y1, int x2, int y2, Color color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int error = dx - dy;

    set_color(color);
    glBegin(GL_POINTS);

    while (1) {
        plot_pixel(x1, y1);

        if (x1 == x2 && y1 == y2) {
            break;
        }

        int double_error = 2 * error;

        if (double_error > -dy) {
            error -= dy;
            x1 += sx;
        }

        if (double_error < dx) {
            error += dx;
            y1 += sy;
        }
    }

    glEnd();
}

/* ============================================================
 * 4. Rectangulo relleno por doble bucle
 * ============================================================ */

static void fill_rect_pixels(int x1, int y1, int x2, int y2, Color color) {
    if (x1 > x2) {
        int tmp = x1; x1 = x2; x2 = tmp;
    }

    if (y1 > y2) {
        int tmp = y1; y1 = y2; y2 = tmp;
    }

    set_color(color);
    glBegin(GL_POINTS);

    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            plot_pixel(x, y);
        }
    }

    glEnd();
}

/* ============================================================
 * 5. Circunferencia con Bresenham
 * ============================================================ */

static void plot_circle_octants(int xc, int yc, int x, int y) {
    plot_pixel(xc + x, yc + y);
    plot_pixel(xc - x, yc + y);
    plot_pixel(xc + x, yc - y);
    plot_pixel(xc - x, yc - y);

    plot_pixel(xc + y, yc + x);
    plot_pixel(xc - y, yc + x);
    plot_pixel(xc + y, yc - x);
    plot_pixel(xc - y, yc - x);
}

static void draw_circle_bresenham(int xc, int yc, int r, Color color) {
    int x = 0;
    int y = r;
    int p = 3 - 2 * r;

    set_color(color);
    glBegin(GL_POINTS);

    while (x <= y) {
        plot_circle_octants(xc, yc, x, y);

        if (p < 0) {
            p = p + 4 * x + 6;
        } else {
            p = p + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }

    glEnd();
}

/* ============================================================
 * 6. Circulo relleno con segmentos horizontales
 * ============================================================ */

static void draw_horizontal_segment_inside_begin(int x1, int x2, int y) {
    if (x1 > x2) {
        int tmp = x1; x1 = x2; x2 = tmp;
    }

    for (int x = x1; x <= x2; ++x) {
        plot_pixel(x, y);
    }
}

static void fill_circle_pixels(int xc, int yc, int r, Color color) {
    int x = 0;
    int y = r;
    int p = 3 - 2 * r;

    set_color(color);
    glBegin(GL_POINTS);

    while (x <= y) {
        draw_horizontal_segment_inside_begin(xc - x, xc + x, yc + y);
        draw_horizontal_segment_inside_begin(xc - x, xc + x, yc - y);
        draw_horizontal_segment_inside_begin(xc - y, xc + y, yc + x);
        draw_horizontal_segment_inside_begin(xc - y, xc + y, yc - x);

        if (p < 0) {
            p = p + 4 * x + 6;
        } else {
            p = p + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }

    glEnd();
}

/* ============================================================
 * Escena de demostracion
 * ============================================================ */

static void draw_panel_border(int x1, int y1, int x2, int y2, Color color) {
    draw_line_bresenham(x1, y1, x2, y1, color);
    draw_line_bresenham(x2, y1, x2, y2, color);
    draw_line_bresenham(x2, y2, x1, y2, color);
    draw_line_bresenham(x1, y2, x1, y1, color);
}

static void display(void) {
    Color black  = rgb(30, 30, 30);
    Color red    = rgb(220, 50, 50);
    Color blue   = rgb(40, 100, 230);
    Color green  = rgb(40, 160, 90);
    Color purple = rgb(140, 80, 220);
    Color orange = rgb(240, 150, 40);
    Color gray   = rgb(235, 235, 235);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glPointSize(1.0f);

    draw_text(25, 30, "Primitivas de salida con OpenGL en C", black);
    draw_text(25, 52, "Fuerza bruta, DDA, Bresenham, circunferencias y rellenos", black);

    fill_rect_pixels(25, 75, 385, 260, rgb(245, 248, 255));
    draw_panel_border(25, 75, 385, 260, black);
    draw_text_block(40, 96, "Lineas:\nfuerza bruta / DDA / Bresenham", black, 16);
    draw_line_bruteforce(50, 145, 360, 220, red);
    draw_line_dda(50, 220, 360, 145, blue);
    draw_line_bresenham(50, 182, 360, 182, black);
    draw_text_block(40, 230, "Rojo=bruta  Azul=DDA\nNegro=Bresenham", black, 16);

    fill_rect_pixels(415, 75, 790, 260, rgb(248, 245, 255));
    draw_panel_border(415, 75, 790, 260, black);
    draw_text(430, 96, "Bresenham generalizado", black);
    draw_line_bresenham(445, 220, 775, 110, purple);
    draw_line_bresenham(445, 110, 775, 220, purple);
    draw_line_bresenham(610, 95, 610, 235, green);
    draw_line_bresenham(430, 165, 780, 165, green);
    draw_text_block(430, 230, "Pendientes positivas,\nnegativas y verticales", black, 16);

    fill_rect_pixels(820, 75, 1215, 260, rgb(245, 255, 248));
    draw_panel_border(820, 75, 1215, 260, black);
    draw_text(835, 96, "Circunferencia con Bresenham", black);
    draw_circle_bresenham(1015, 170, 72, green);
    draw_text_block(835, 230, "Se calcula un octante\ny se replica", black, 16);

    fill_rect_pixels(25, 290, 385, 760, rgb(250, 250, 250));
    draw_panel_border(25, 290, 385, 760, black);
    draw_text(40, 310, "Relleno de rectangulos", black);
    fill_rect_pixels(85, 355, 325, 620, gray);
    draw_panel_border(85, 355, 325, 620, black);
    draw_line_bresenham(85, 355, 325, 620, red);
    draw_line_bresenham(85, 620, 325, 355, red);
    draw_text_block(40, 700, "Doble bucle:\nO(ancho * alto)", black, 16);

    fill_rect_pixels(415, 290, 790, 760, rgb(255, 250, 242));
    draw_panel_border(415, 290, 790, 760, black);
    draw_text(430, 310, "Circulo relleno", black);
    fill_circle_pixels(605, 500, 95, rgb(255, 225, 175));
    draw_circle_bresenham(605, 500, 95, orange);
    draw_text_block(430, 700, "Rellena segmentos\nhorizontales", black, 16);

    fill_rect_pixels(820, 290, 1215, 760, rgb(245, 248, 255));
    draw_panel_border(820, 290, 1215, 760, black);
    draw_text_block(835, 310, "Ejemplo aplicado:\noverlay debug 2D", black, 16);
    fill_rect_pixels(900, 390, 1095, 565, rgb(230, 240, 255));
    draw_panel_border(900, 390, 1095, 565, blue);
    draw_circle_bresenham(998, 477, 82, red);
    draw_line_bresenham(850, 477, 1185, 477, green);
    draw_line_bresenham(998, 325, 998, 630, green);
    draw_text_block(835, 700, "Bounding box,\nradio de colision y ejes", black, 16);

    glutSwapBuffers();
}

static void reshape(int width, int height) {
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (double)WIDTH, (double)HEIGHT, 0.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    if (key == 27 || key == 'q' || key == 'Q') {
        exit(EXIT_SUCCESS);
    }

    if (key == 'r' || key == 'R') {
        glutPostRedisplay();
    }
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);

    /* GLUT_DOUBLE activa doble buffer: dibujamos y presentamos con glutSwapBuffers(). */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Primitivas de salida - OpenGL C");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    return EXIT_SUCCESS;
}
