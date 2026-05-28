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

#define INITIAL_WINDOW_WIDTH 1100
#define INITIAL_WINDOW_HEIGHT 820
#define PI 3.14159265358979323846

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
    int width;
    int height;
    bool running;
} App;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    Color background;
    const double *projection;
} ViewportRegion;

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

static void build_cabinet_matrix(
    double left,
    double right,
    double bottom,
    double top,
    double near_plane,
    double far_plane,
    double angle_degrees,
    double receding_scale,
    double matrix[16]
) {
    const double angle = angle_degrees * PI / 180.0;
    double orthographic[16];
    double shear[16];

    build_orthographic_matrix(left, right, bottom, top, near_plane, far_plane, orthographic);
    set_identity(shear);
    shear[8] = receding_scale * cos(angle);
    shear[9] = receding_scale * sin(angle);
    multiply_matrix(orthographic, shear, matrix);
}

static void build_projection_set(double orthographic[16], double cabinet[16], double symmetric[16], double oblique[16]) {
    build_orthographic_matrix(-3.6, 3.6, -3.0, 3.0, 1.0, 18.0, orthographic);
    build_cabinet_matrix(-4.0, 4.0, -3.0, 3.0, 1.0, 18.0, 45.0, 0.5, cabinet);
    build_frustum_matrix(-2.2, 2.2, -1.8, 1.8, 3.0, 24.0, symmetric);
    build_frustum_matrix(-2.7, 1.5, -1.4, 2.2, 3.0, 24.0, oblique);
}

static void draw_axes(double length) {
    glLineWidth(2.0f);
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

static void draw_cube_faces(void) {
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
}

static void draw_cube_edges(void) {
    glColor3f(0.05f, 0.08f, 0.12f);
    glLineWidth(2.5f);
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

static void draw_scene_geometry(void) {
    draw_axes(2.3);
    draw_cube_faces();
    draw_cube_edges();
}

static void draw_overlay_dividers(const App *app) {
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

    glEnable(GL_DEPTH_TEST);
}

static void render_viewport(const ViewportRegion *region) {
    glViewport(region->x, region->y, region->width, region->height);
    glScissor(region->x, region->y, region->width, region->height);
    glClearColor(region->background.red, region->background.green, region->background.blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(region->projection);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(0.0, -0.1, -8.0);
    glRotated(26.0, 1.0, 0.0, 0.0);
    glRotated(-34.0, 0.0, 1.0, 0.0);
    glRotated(12.0, 0.0, 0.0, 1.0);

    draw_scene_geometry();
}

static void render(const App *app, const double orthographic[16], const double cabinet[16], const double symmetric[16], const double oblique[16]) {
    const int half_width = app->width / 2;
    const int half_height = app->height / 2;
    const ViewportRegion viewports[4] = {
        {0, half_height, half_width, app->height - half_height, {0.95f, 0.96f, 0.98f}, orthographic},
        {half_width, half_height, app->width - half_width, app->height - half_height, {0.96f, 0.95f, 0.98f}, cabinet},
        {0, 0, half_width, half_height, {0.95f, 0.98f, 0.97f}, symmetric},
        {half_width, 0, app->width - half_width, half_height, {0.98f, 0.96f, 0.94f}, oblique}
    };
    int i;

    glEnable(GL_SCISSOR_TEST);

    for (i = 0; i < 4; ++i) {
        render_viewport(&viewports[i]);
    }

    draw_overlay_dividers(app);
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

    XStoreName(app->display, app->window, "Trabajo 7 - Proyecciones 3D");
    app->wm_delete_message = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->window, &app->wm_delete_message, 1);
    XMapWindow(app->display, app->window);
    glXMakeCurrent(app->display, app->window, app->context);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_CULL_FACE);

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
    double orthographic[16];
    double cabinet[16];
    double symmetric[16];
    double oblique[16];

    build_projection_set(orthographic, cabinet, symmetric, oblique);

    if (!create_window(&app)) {
        return EXIT_FAILURE;
    }

    while (app.running) {
        while (XPending(app.display) > 0) {
            XEvent event;
            XNextEvent(app.display, &event);
            handle_event(&app, &event);
        }

        render(&app, orthographic, cabinet, symmetric, oblique);
    }

    destroy_window(&app);
    return EXIT_SUCCESS;
}
