/*
 * Trabajo 7: cuatro proyecciones 3D del mismo cubo en una sola ventana.
 * La escena usa OpenGL clásico sobre X11/GLX y mantiene el ojo en (0,0,0).
 */

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_WINDOW_WIDTH 1100
#define INITIAL_WINDOW_HEIGHT 820
#define PI 3.14159265358979323846
#define QUADRANT_PADDING 18
#define LABEL_STRIP_HEIGHT 34
#define FONT_GLYPH_COUNT 96

typedef struct {
    float red;
    float green;
    float blue;
} Color;

typedef struct {
    Display *display;
    Window window;
    GLXContext context;
    Atom wm_delete_message;
    GLuint font_base;
    int width;
    int height;
    bool running;
} App;

typedef struct {
    int frame_x;
    int frame_y;
    int frame_width;
    int frame_height;
    int viewport_x;
    int viewport_y;
    int viewport_width;
    int viewport_height;
    Color background;
    const char *label;
} ViewportRegion;

static int maximum_int(int first, int second) {
    return first > second ? first : second;
}

static void set_identity(double matrix[16]) {
    int i;

    for (i = 0; i < 16; ++i) {
        matrix[i] = 0.0;
    }

    matrix[0] = 1.0;
    matrix[5] = 1.0;
    matrix[10] = 1.0;
    matrix[15] = 1.0;
}

static void multiply_matrix(const double left[16], const double right[16], double out[16]) {
    int row;
    int column;
    double product[16];

    for (column = 0; column < 4; ++column) {
        for (row = 0; row < 4; ++row) {
            product[column * 4 + row] =
                left[0 * 4 + row] * right[column * 4 + 0] +
                left[1 * 4 + row] * right[column * 4 + 1] +
                left[2 * 4 + row] * right[column * 4 + 2] +
                left[3 * 4 + row] * right[column * 4 + 3];
        }
    }

    for (row = 0; row < 16; ++row) {
        out[row] = product[row];
    }
}

/* Orthographic projection matrix: parallel projection, no vanishing points. */
static void build_orthographic_matrix(
    double left,
    double right,
    double bottom,
    double top,
    double near_plane,
    double far_plane,
    double matrix[16]
) {
    set_identity(matrix);
    matrix[0] = 2.0 / (right - left);
    matrix[5] = 2.0 / (top - bottom);
    matrix[10] = -2.0 / (far_plane - near_plane);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(far_plane + near_plane) / (far_plane - near_plane);
}

/* Perspective frustum matrix used for the symmetric and oblique perspectives. */
static void build_frustum_matrix(
    double left,
    double right,
    double bottom,
    double top,
    double near_plane,
    double far_plane,
    double matrix[16]
) {
    int i;

    for (i = 0; i < 16; ++i) {
        matrix[i] = 0.0;
    }

    matrix[0] = (2.0 * near_plane) / (right - left);
    matrix[5] = (2.0 * near_plane) / (top - bottom);
    matrix[8] = (right + left) / (right - left);
    matrix[9] = (top + bottom) / (top - bottom);
    matrix[10] = -(far_plane + near_plane) / (far_plane - near_plane);
    matrix[11] = -1.0;
    matrix[14] = -(2.0 * far_plane * near_plane) / (far_plane - near_plane);
}

/*
 * Cabinet projection matrix:
 * x' = x + k * z * cos(alpha)
 * y' = y + k * z * sin(alpha)
 * with k = 0.5 and alpha = 45 degrees.
 */
static void build_cabinet_matrix(double aspect_ratio, double matrix[16]) {
    const double half_height = 3.6;
    const double half_width = half_height * aspect_ratio;
    const double angle = 45.0 * PI / 180.0;
    const double reduction = 0.5;
    double orthographic[16];
    double shear[16];

    build_orthographic_matrix(-half_width, half_width, -half_height, half_height, 1.0, 18.0, orthographic);
    set_identity(shear);

    /* The cube lives at z < 0, so a negative shear keeps depth receding up-right on screen. */
    shear[8] = -reduction * cos(angle);
    shear[9] = -reduction * sin(angle);

    multiply_matrix(orthographic, shear, matrix);
}

static void configure_viewports(const App *app, ViewportRegion viewports[4]) {
    const int half_width = app->width / 2;
    const int half_height = app->height / 2;
    int i;

    viewports[0] = (ViewportRegion) {
        0,
        half_height,
        half_width,
        app->height - half_height,
        0,
        0,
        0,
        0,
        {0.95f, 0.96f, 0.98f},
        "Ortogonal"
    };
    viewports[1] = (ViewportRegion) {
        half_width,
        half_height,
        app->width - half_width,
        app->height - half_height,
        0,
        0,
        0,
        0,
        {0.96f, 0.95f, 0.98f},
        "Gabinete"
    };
    viewports[2] = (ViewportRegion) {
        0,
        0,
        half_width,
        half_height,
        0,
        0,
        0,
        0,
        {0.95f, 0.98f, 0.97f},
        "Perspectiva simetrica"
    };
    viewports[3] = (ViewportRegion) {
        half_width,
        0,
        app->width - half_width,
        half_height,
        0,
        0,
        0,
        0,
        {0.98f, 0.96f, 0.94f},
        "Perspectiva oblicua"
    };

    for (i = 0; i < 4; ++i) {
        viewports[i].viewport_x = viewports[i].frame_x + QUADRANT_PADDING;
        viewports[i].viewport_y = viewports[i].frame_y + QUADRANT_PADDING;
        viewports[i].viewport_width = maximum_int(80, viewports[i].frame_width - 2 * QUADRANT_PADDING);
        viewports[i].viewport_height = maximum_int(
            80,
            viewports[i].frame_height - LABEL_STRIP_HEIGHT - 2 * QUADRANT_PADDING
        );
    }
}

static int create_overlay_font(App *app) {
    XFontStruct *font_info = XLoadQueryFont(app->display, "fixed");

    if (font_info == NULL) {
        return 0;
    }

    app->font_base = glGenLists(FONT_GLYPH_COUNT);
    if (app->font_base == 0U) {
        XFreeFont(app->display, font_info);
        return 0;
    }

    glXUseXFont(font_info->fid, 32, FONT_GLYPH_COUNT, app->font_base);
    XFreeFont(app->display, font_info);
    return 1;
}

static void draw_overlay_text(const App *app, int x, int y, const char *text) {
    if (app->font_base == 0U || text == NULL) {
        return;
    }

    glColor3f(0.12f, 0.14f, 0.18f);
    glRasterPos2i(x, y);
    glListBase(app->font_base - 32U);
    glCallLists((GLsizei) strlen(text), GL_UNSIGNED_BYTE, (const GLubyte *) text);
}

static void drawLabel(const App *app, int x, int y, const char *text) {
    draw_overlay_text(app, x, y, text);
}

static void begin_viewport_3d(const ViewportRegion *region, Color background) {
    glScissor(region->frame_x, region->frame_y, region->frame_width, region->frame_height);
    glClearColor(background.red, background.green, background.blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(region->viewport_x, region->viewport_y, region->viewport_width, region->viewport_height);
    glScissor(region->viewport_x, region->viewport_y, region->viewport_width, region->viewport_height);
}

static void draw_axes(double length) {
    glLineWidth(1.8f);
    glBegin(GL_LINES);

    glColor3f(0.84f, 0.18f, 0.16f);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(length, 0.0, 0.0);

    glColor3f(0.16f, 0.60f, 0.26f);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(0.0, length, 0.0);

    glColor3f(0.12f, 0.38f, 0.78f);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(0.0, 0.0, length);

    glEnd();
}

static void draw_cube(void) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    glBegin(GL_QUADS);

    glColor3f(0.89f, 0.27f, 0.24f);
    glVertex3d(-1.0, -1.0, 1.0);
    glVertex3d(1.0, -1.0, 1.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(-1.0, 1.0, 1.0);

    glColor3f(0.21f, 0.66f, 0.87f);
    glVertex3d(-1.0, -1.0, -1.0);
    glVertex3d(-1.0, 1.0, -1.0);
    glVertex3d(1.0, 1.0, -1.0);
    glVertex3d(1.0, -1.0, -1.0);

    glColor3f(0.96f, 0.74f, 0.18f);
    glVertex3d(-1.0, 1.0, -1.0);
    glVertex3d(-1.0, 1.0, 1.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(1.0, 1.0, -1.0);

    glColor3f(0.31f, 0.76f, 0.44f);
    glVertex3d(-1.0, -1.0, -1.0);
    glVertex3d(1.0, -1.0, -1.0);
    glVertex3d(1.0, -1.0, 1.0);
    glVertex3d(-1.0, -1.0, 1.0);

    glColor3f(0.74f, 0.44f, 0.84f);
    glVertex3d(1.0, -1.0, -1.0);
    glVertex3d(1.0, 1.0, -1.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(1.0, -1.0, 1.0);

    glColor3f(0.98f, 0.55f, 0.19f);
    glVertex3d(-1.0, -1.0, -1.0);
    glVertex3d(-1.0, -1.0, 1.0);
    glVertex3d(-1.0, 1.0, 1.0);
    glVertex3d(-1.0, 1.0, -1.0);

    glEnd();

    glDisable(GL_POLYGON_OFFSET_FILL);

    glColor3f(0.05f, 0.08f, 0.12f);
    glLineWidth(2.3f);
    glBegin(GL_LINES);

    glVertex3d(-1.0, -1.0, -1.0);
    glVertex3d(1.0, -1.0, -1.0);
    glVertex3d(1.0, -1.0, -1.0);
    glVertex3d(1.0, 1.0, -1.0);
    glVertex3d(1.0, 1.0, -1.0);
    glVertex3d(-1.0, 1.0, -1.0);
    glVertex3d(-1.0, 1.0, -1.0);
    glVertex3d(-1.0, -1.0, -1.0);

    glVertex3d(-1.0, -1.0, 1.0);
    glVertex3d(1.0, -1.0, 1.0);
    glVertex3d(1.0, -1.0, 1.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(-1.0, 1.0, 1.0);
    glVertex3d(-1.0, 1.0, 1.0);
    glVertex3d(-1.0, -1.0, 1.0);

    glVertex3d(-1.0, -1.0, -1.0);
    glVertex3d(-1.0, -1.0, 1.0);
    glVertex3d(1.0, -1.0, -1.0);
    glVertex3d(1.0, -1.0, 1.0);
    glVertex3d(1.0, 1.0, -1.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(-1.0, 1.0, -1.0);
    glVertex3d(-1.0, 1.0, 1.0);

    glEnd();
}

static void draw_scene(void) {
    draw_axes(1.8);
    draw_cube();
}

static void drawOrthogonalView(const ViewportRegion *region) {
    double projection[16];
    const double aspect_ratio = (double) region->viewport_width / (double) region->viewport_height;
    const double half_height = 3.3;
    const double half_width = half_height * aspect_ratio;

    begin_viewport_3d(region, region->background);

    build_orthographic_matrix(-half_width, half_width, -half_height, half_height, 1.0, 18.0, projection);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Isometric-like pose: still parallel projection, but several faces are visible. */
    glTranslated(0.0, 0.0, -8.4);
    glRotated(35.264, 1.0, 0.0, 0.0);
    glRotated(-45.0, 0.0, 1.0, 0.0);

    draw_scene();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawCabinetView(const ViewportRegion *region) {
    double projection[16];
    const double aspect_ratio = (double) region->viewport_width / (double) region->viewport_height;

    begin_viewport_3d(region, region->background);
    build_cabinet_matrix(aspect_ratio, projection);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /*
     * Frontal pose: the front face stays in true magnitude and the depth recedes
     * diagonally only because of the cabinet shear, not because of perspective.
     */
    glTranslated(-2.15, -2.15, -6.0);

    draw_scene();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawSymmetricPerspectiveView(const ViewportRegion *region) {
    double projection[16];
    const double aspect_ratio = (double) region->viewport_width / (double) region->viewport_height;
    const double top = 2.2;
    const double right = top * aspect_ratio;

    begin_viewport_3d(region, region->background);
    build_frustum_matrix(-right, right, -top, top, 3.0, 24.0, projection);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Centered frustum: balanced perspective and symmetric vanishing behavior. */
    glTranslated(0.0, 0.15, -7.6);
    glRotated(24.0, 1.0, 0.0, 0.0);
    glRotated(-32.0, 0.0, 1.0, 0.0);

    draw_scene();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawObliquePerspectiveView(const ViewportRegion *region) {
    double projection[16];
    const double aspect_ratio = (double) region->viewport_width / (double) region->viewport_height;
    const double top = 2.2;
    const double right = top * aspect_ratio;

    begin_viewport_3d(region, region->background);

    /*
     * Off-axis perspective: same eye at the origin, but an asymmetric frustum
     * shifts the visible volume and changes the perspective result.
     */
    build_frustum_matrix(-1.65 * right, 0.35 * right, -1.05 * top, 0.95 * top, 3.0, 24.0, projection);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Same cube placement as the symmetric case so the projection matrix is the main difference. */
    glTranslated(0.0, 0.15, -7.6);
    glRotated(24.0, 1.0, 0.0, 0.0);
    glRotated(-32.0, 0.0, 1.0, 0.0);

    draw_scene();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void draw_overlay(const App *app, const ViewportRegion viewports[4]) {
    int i;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    glViewport(0, 0, app->width, app->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double) app->width, 0.0, (double) app->height, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.92f, 0.93f, 0.95f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2d(app->width / 2.0, 0.0);
    glVertex2d(app->width / 2.0, app->height);
    glVertex2d(0.0, app->height / 2.0);
    glVertex2d(app->width, app->height / 2.0);
    glEnd();

    for (i = 0; i < 4; ++i) {
        drawLabel(app, viewports[i].frame_x + 12, viewports[i].frame_y + viewports[i].frame_height - 22, viewports[i].label);
    }

    glEnable(GL_DEPTH_TEST);
}

static void render(const App *app) {
    ViewportRegion viewports[4];

    configure_viewports(app, viewports);
    glEnable(GL_SCISSOR_TEST);

    drawOrthogonalView(&viewports[0]);
    drawCabinetView(&viewports[1]);
    drawSymmetricPerspectiveView(&viewports[2]);
    drawObliquePerspectiveView(&viewports[3]);

    draw_overlay(app, viewports);
    glXSwapBuffers(app->display, app->window);
}

static int create_window(App *app) {
    static int visual_attributes[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };
    XVisualInfo *visual_info;
    Colormap colormap;
    XSetWindowAttributes attributes;
    XEvent event;

    app->display = XOpenDisplay(NULL);
    if (app->display == NULL) {
        fprintf(stderr, "No se pudo abrir la pantalla X11.\n");
        return 0;
    }

    visual_info = glXChooseVisual(app->display, DefaultScreen(app->display), visual_attributes);
    if (visual_info == NULL) {
        fprintf(stderr, "No se encontro un visual GLX compatible.\n");
        XCloseDisplay(app->display);
        app->display = NULL;
        return 0;
    }

    app->context = glXCreateContext(app->display, visual_info, NULL, True);
    colormap = XCreateColormap(
        app->display,
        RootWindow(app->display, visual_info->screen),
        visual_info->visual,
        AllocNone
    );

    attributes.colormap = colormap;
    attributes.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

    app->window = XCreateWindow(
        app->display,
        RootWindow(app->display, visual_info->screen),
        0,
        0,
        INITIAL_WINDOW_WIDTH,
        INITIAL_WINDOW_HEIGHT,
        0,
        visual_info->depth,
        InputOutput,
        visual_info->visual,
        CWColormap | CWEventMask,
        &attributes
    );

    XStoreName(app->display, app->window, "Trabajo - Proyecciones 3D");
    app->wm_delete_message = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->window, &app->wm_delete_message, 1);
    XMapWindow(app->display, app->window);
    glXMakeCurrent(app->display, app->window, app->context);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_CULL_FACE);
    create_overlay_font(app);

    app->width = INITIAL_WINDOW_WIDTH;
    app->height = INITIAL_WINDOW_HEIGHT;
    app->running = true;

    while (XPending(app->display) > 0) {
        XNextEvent(app->display, &event);
    }

    XFree(visual_info);
    return 1;
}

static void destroy_window(App *app) {
    if (app->display == NULL) {
        return;
    }

    if (app->font_base != 0U) {
        glDeleteLists(app->font_base, FONT_GLYPH_COUNT);
    }

    glXMakeCurrent(app->display, None, NULL);

    if (app->context != NULL) {
        glXDestroyContext(app->display, app->context);
    }

    XDestroyWindow(app->display, app->window);
    XCloseDisplay(app->display);
}

static void handle_event(App *app, const XEvent *event) {
    switch (event->type) {
        case ClientMessage:
            if ((Atom) event->xclient.data.l[0] == app->wm_delete_message) {
                app->running = false;
            }
            break;
        case ConfigureNotify:
            if (event->xconfigure.width > 0 && event->xconfigure.height > 0) {
                app->width = event->xconfigure.width;
                app->height = event->xconfigure.height;
            }
            break;
        case KeyPress:
            app->running = false;
            break;
        default:
            break;
    }
}

int main(void) {
    App app = {0};

    if (!create_window(&app)) {
        return EXIT_FAILURE;
    }

    while (app.running) {
        while (XPending(app.display) > 0) {
            XEvent event;
            XNextEvent(app.display, &event);
            handle_event(&app, &event);
        }

        render(&app);
    }

    destroy_window(&app);
    return EXIT_SUCCESS;
}
