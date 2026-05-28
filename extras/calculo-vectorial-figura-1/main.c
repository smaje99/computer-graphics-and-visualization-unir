/*
 * Rescate de la Figura 1 del laboratorio de Calculo Vectorial.
 * Se representa la superficie cuadrica como una malla alambrica.
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

#define INITIAL_WINDOW_WIDTH 960
#define INITIAL_WINDOW_HEIGHT 760
#define PI 3.14159265358979323846
#define U_STEPS 72
#define V_STEPS 28
#define V_LIMIT 1.1

typedef struct {
    Display *display;
    Window window;
    GLXContext context;
    Atom wm_delete_message;
    int width;
    int height;
    bool running;
} App;

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

static void sample_hyperboloid(double u, double v, double *x, double *y, double *z) {
    const double cosh_v = cosh(v);

    *x = 3.0 * cosh_v * cos(u);
    *y = 3.0 + 2.0 * cosh_v * sin(u);
    *z = -1.0 + 3.0 * sinh(v);
}

static void draw_axes(double length) {
    glLineWidth(2.0f);
    glBegin(GL_LINES);

    glColor3f(0.82f, 0.20f, 0.16f);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(length, 0.0, 0.0);

    glColor3f(0.16f, 0.62f, 0.28f);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(0.0, length, 0.0);

    glColor3f(0.14f, 0.38f, 0.80f);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(0.0, 0.0, length);

    glEnd();
}

static void draw_hyperboloid_mesh(void) {
    int v_index;
    int u_index;

    glColor3f(0.10f, 0.25f, 0.55f);
    glLineWidth(1.6f);

    for (v_index = 0; v_index <= V_STEPS; ++v_index) {
        const double v = -V_LIMIT + (2.0 * V_LIMIT * v_index) / (double) V_STEPS;

        glBegin(GL_LINE_STRIP);
        for (u_index = 0; u_index <= U_STEPS; ++u_index) {
            const double u = (2.0 * PI * u_index) / (double) U_STEPS;
            double x;
            double y;
            double z;

            sample_hyperboloid(u, v, &x, &y, &z);
            glVertex3d(x, y, z);
        }
        glEnd();
    }

    for (u_index = 0; u_index < U_STEPS; ++u_index) {
        const double u = (2.0 * PI * u_index) / (double) U_STEPS;

        glBegin(GL_LINE_STRIP);
        for (v_index = 0; v_index <= V_STEPS; ++v_index) {
            const double v = -V_LIMIT + (2.0 * V_LIMIT * v_index) / (double) V_STEPS;
            double x;
            double y;
            double z;

            sample_hyperboloid(u, v, &x, &y, &z);
            glVertex3d(x, y, z);
        }
        glEnd();
    }
}

static void render(const App *app, const double projection[16]) {
    glViewport(0, 0, app->width, app->height);
    glClearColor(0.95f, 0.97f, 0.99f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(0.0, -3.0, -18.0);
    glRotated(22.0, 1.0, 0.0, 0.0);
    glRotated(-42.0, 0.0, 1.0, 0.0);

    draw_axes(8.0);
    draw_hyperboloid_mesh();

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

    XStoreName(app->display, app->window, "Calculo Vectorial - Figura 1");
    app->wm_delete_message = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->window, &app->wm_delete_message, 1);
    XMapWindow(app->display, app->window);
    glXMakeCurrent(app->display, app->window, app->context);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);

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
    double projection[16];

    build_frustum_matrix(-1.8, 1.8, -1.4, 1.4, 3.0, 40.0, projection);

    if (!create_window(&app)) {
        return EXIT_FAILURE;
    }

    while (app.running) {
        while (XPending(app.display) > 0) {
            XEvent event;
            XNextEvent(app.display, &event);
            handle_event(&app, &event);
        }

        render(&app, projection);
    }

    destroy_window(&app);
    return EXIT_SUCCESS;
}
