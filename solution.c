/*
 * Static analog clock rendered with classic OpenGL over X11/GLX.
 * The program captures the local time once at startup and uses it
 * to compute the clock hands and hour marks geometrically.
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
#include <time.h>

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 800
#define CIRCLE_SEGMENTS 180
#define PI 3.14159265358979323846

typedef struct {
    int hour;
    int minute;
    int second;
} ClockTime;

/* Stores the X11 window and GLX context state used by the application. */
typedef struct {
    Display *display;
    Window window;
    GLXContext context;
    Atom wm_delete_message;
    int width;
    int height;
    bool running;
} App;

/* Convert degrees into radians before calling trigonometric functions. */
static double degrees_to_radians(double degrees) {
    return degrees * PI / 180.0;
}

static void set_color(float red, float green, float blue) {
    glColor3f(red, green, blue);
}

/* Keep the orthographic projection proportional after window resizes. */
static void configure_projection(const App *app) {
    const double aspect_ratio = (double) app->width / (double) app->height;

    glViewport(0, 0, app->width, app->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (aspect_ratio >= 1.0) {
        glOrtho(-aspect_ratio, aspect_ratio, -1.0, 1.0, -1.0, 1.0);
    } else {
        glOrtho(-1.0, 1.0, -1.0 / aspect_ratio, 1.0 / aspect_ratio, -1.0, 1.0);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* Capture the local time once because the laboratory requires a static clock. */
static ClockTime capture_clock_time(void) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    ClockTime captured = {0, 0, 0};

    if (local_time == NULL) {
        return captured;
    }

    captured.hour = local_time->tm_hour;
    captured.minute = local_time->tm_min;
    captured.second = local_time->tm_sec;

    return captured;
}

/* Approximate a circumference with uniformly distributed line segments. */
static void draw_circle(double radius) {
    int i;

    glBegin(GL_LINE_LOOP);
    for (i = 0; i < CIRCLE_SEGMENTS; ++i) {
        const double angle = (2.0 * PI * i) / (double) CIRCLE_SEGMENTS;
        glVertex2d(radius * cos(angle), radius * sin(angle));
    }
    glEnd();
}

/* Draw the twelve radial hour marks, emphasizing quarter positions. */
static void draw_hour_marks(void) {
    int hour_mark;

    for (hour_mark = 0; hour_mark < 12; ++hour_mark) {
        const double angle = degrees_to_radians(90.0 - hour_mark * 30.0);
        const bool major_mark = (hour_mark % 3) == 0;
        const double outer_radius = 0.88;
        const double inner_radius = major_mark ? 0.72 : 0.78;
        const double start_x = inner_radius * cos(angle);
        const double start_y = inner_radius * sin(angle);
        const double end_x = outer_radius * cos(angle);
        const double end_y = outer_radius * sin(angle);

        glLineWidth(major_mark ? 4.0f : 2.0f);
        glBegin(GL_LINES);
        glVertex2d(start_x, start_y);
        glVertex2d(end_x, end_y);
        glEnd();
    }
}

/* Draw a hand from the origin using its angle and length. */
static void draw_hand(double angle_degrees, double length, double line_width) {
    const double angle = degrees_to_radians(angle_degrees);
    const double end_x = length * cos(angle);
    const double end_y = length * sin(angle);

    glLineWidth((GLfloat) line_width);
    glBegin(GL_LINES);
    glVertex2d(0.0, 0.0);
    glVertex2d(end_x, end_y);
    glEnd();
}

/* Cover the joint of the three hands with a small filled circle. */
static void draw_center_pin(void) {
    const double radius = 0.03;
    int i;

    glBegin(GL_POLYGON);
    for (i = 0; i < CIRCLE_SEGMENTS; ++i) {
        const double angle = (2.0 * PI * i) / (double) CIRCLE_SEGMENTS;
        glVertex2d(radius * cos(angle), radius * sin(angle));
    }
    glEnd();
}

/* Compute clock-hand angles from the captured time and render the full clock. */
static void draw_clock(const ClockTime *clock_time) {
    const double second_angle = 90.0 - clock_time->second * 6.0;
    const double minute_value = (double) clock_time->minute + clock_time->second / 60.0;
    const double minute_angle = 90.0 - minute_value * 6.0;
    const double hour_value = (clock_time->hour % 12) + minute_value / 60.0;
    const double hour_angle = 90.0 - hour_value * 30.0;

    set_color(0.08f, 0.10f, 0.16f);
    glLineWidth(5.0f);
    draw_circle(0.9);

    set_color(0.15f, 0.18f, 0.28f);
    glLineWidth(1.5f);
    draw_circle(0.82);

    set_color(0.08f, 0.10f, 0.16f);
    draw_hour_marks();

    set_color(0.10f, 0.12f, 0.18f);
    draw_hand(hour_angle, 0.45, 6.0);

    set_color(0.22f, 0.28f, 0.40f);
    draw_hand(minute_angle, 0.65, 4.0);

    set_color(0.78f, 0.16f, 0.12f);
    draw_hand(second_angle, 0.75, 2.0);

    set_color(0.08f, 0.10f, 0.16f);
    draw_center_pin();
}

/* Render one static frame and swap the front/back buffers. */
static void render(const App *app, const ClockTime *clock_time) {
    (void) app;

    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    draw_clock(clock_time);

    glXSwapBuffers(app->display, app->window);
}

/* Create the X11 window, attach a GLX context and configure basic OpenGL state. */
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
        fprintf(stderr, "Could not open the X11 display.\n");
        return 0;
    }

    visual_info = glXChooseVisual(app->display, DefaultScreen(app->display), visual_attributes);
    if (visual_info == NULL) {
        fprintf(stderr, "Could not find a compatible GLX visual.\n");
        XCloseDisplay(app->display);
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

    XStoreName(app->display, app->window, "OpenGL Analog Clock");
    app->wm_delete_message = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->window, &app->wm_delete_message, 1);
    XMapWindow(app->display, app->window);
    glXMakeCurrent(app->display, app->window, app->context);

    glClearColor(0.96f, 0.95f, 0.92f, 1.0f);
    glDisable(GL_DEPTH_TEST);

    app->width = INITIAL_WINDOW_WIDTH;
    app->height = INITIAL_WINDOW_HEIGHT;
    app->running = true;

    configure_projection(app);

    while (XPending(app->display) > 0) {
        XNextEvent(app->display, &event);
    }

    XFree(visual_info);
    return 1;
}

/* Release the GLX context and close the X11 window. */
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

/* React to close, resize and keyboard events from the X11 event queue. */
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
                configure_projection(app);
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
    const ClockTime captured_time = capture_clock_time();

    if (!create_window(&app)) {
        return EXIT_FAILURE;
    }

    while (app.running) {
        while (XPending(app.display) > 0) {
            XEvent event;
            XNextEvent(app.display, &event);
            handle_event(&app, &event);
        }

        render(&app, &captured_time);
    }

    destroy_window(&app);
    return EXIT_SUCCESS;
}
