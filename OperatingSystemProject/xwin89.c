#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>

struct Global {
    Display *dpy;
    Window win;
    GC gc;
    int xres, yres;
} g;

int child = 0;

void x11_cleanup_xwindows(void);
void x11_init_xwindows(void);
void x11_clear_window(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void render(void);
void create_child_window(void);

int main(int argc, char *argv[], char *envp[]) {
    XEvent e;
    int done = 0;
    x11_init_xwindows();
    int timer = 0;
    if (argc > 1) {
        timer = atoi(argv[1]);
        if (timer < 0) {
            fprintf(stderr, "Timer value cannot be negative.\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: %s <timer>\n", argv[0]);
        fprintf(stderr, "Running with default timer value: 0\n");
    }

    while (!done) {
        /* Check the event queue */
        while (XPending(g.dpy)) {
            XNextEvent(g.dpy, &e);
            check_mouse(&e);
            done = check_keys(&e);
            render();
        }
        usleep(timer * 1000);  // Convert timer to microseconds
    }
    x11_cleanup_xwindows();
    return 0;
}
void x11_cleanup_xwindows(void) {
    XDestroyWindow(g.dpy, g.win);
    XCloseDisplay(g.dpy);
}

void x11_init_xwindows(void) {
    int scr;
    if (!(g.dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "ERROR: could not open display!\n");
        exit(EXIT_FAILURE);
    }
    scr = DefaultScreen(g.dpy);
    g.xres = 400;
    g.yres = 200;
    g.win = XCreateSimpleWindow(g.dpy, RootWindow(g.dpy, scr), 1, 1,
                            g.xres, g.yres, 0, 0x00ffffff, 0x00ffa500);
    XStoreName(g.dpy, g.win, "cs3600 xwin sample");
    g.gc = XCreateGC(g.dpy, g.win, 0, NULL);
    XMapWindow(g.dpy, g.win);
    XSelectInput(g.dpy, g.win, ExposureMask | StructureNotifyMask |
                                PointerMotionMask | ButtonPressMask |
                                ButtonReleaseMask | KeyPressMask);
    child = 0;
}

void render(void) {
    XSetForeground(g.dpy, g.gc, 0x00ffff00);  // Yellow color
    XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);
    if (child) {
        XSetForeground(g.dpy, g.gc, 0x000000ff);  // Blue color for child window
        XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);
        XSetForeground(g.dpy, g.gc, 0x00ffffff);  // White color for text
        XDrawString(g.dpy, g.win, g.gc, 50, 50, "Child window", strlen("Child window"));
    } else {
        // Parent window
        XSetForeground(g.dpy, g.gc, 0x00000000);  // Black color for text
        XDrawString(g.dpy, g.win, g.gc, 50, 50, "Press C to turn child window", strlen("Press C to turn child window"));
    }
}
void check_mouse(XEvent *e) {
    int mx = e->xbutton.x;
    int my = e->xbutton.y;

    if (e->type != MotionNotify)
        return;
    if (child) {
        if (mx >= 0 && mx < g.xres && my >= 0 && my < g.yres) {
            printf("c");
            fflush(stdout);
        }
    } else {
        if (mx >= 0 && mx < g.xres && my >= 0 && my < g.yres) {
            printf("m");
            fflush(stdout);
        }
    }
}

int check_keys(XEvent *e) {
    int key;
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;
    key = XLookupKeysym(&e->xkey, 0);
    if (e->type == KeyPress) {
        switch (key) {
            case XK_c:
                create_child_window();
                break;
            case XK_Escape:
                return 1;
        }
    }
    return 0;
}

void create_child_window(void) {
    if (!child) {
        pid_t cpid = fork();
        if (cpid == 0) {
            // Child process
            if (!(g.dpy = XOpenDisplay(NULL))) {
                fprintf(stderr, "ERROR: could not open display for child!\n");
                exit(EXIT_FAILURE);
            }
            int scr = DefaultScreen(g.dpy);
            g.win = XCreateSimpleWindow(g.dpy, RootWindow(g.dpy, scr), 1, 1,
                                        g.xres, g.yres, 0, 0x000000ff, 0x00ffffff);
            XStoreName(g.dpy, g.win, "Child Window");
            g.gc = XCreateGC(g.dpy, g.win, 0, NULL);
            XMapWindow(g.dpy, g.win);
            XSelectInput(g.dpy, g.win, ExposureMask | StructureNotifyMask);
            child = 1;
            render();
            while (1) {
                XEvent e;
                XNextEvent(g.dpy, &e);
                if (e.type == Expose) {
                    render();
                }
            }
            exit(0);
        }
    }
}
