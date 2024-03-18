#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>  

#define MAX_COLORS 5
#define COLOR_BOX_WIDTH 50
#define COLOR_BOX_HEIGHT 50

// Message structure for IPC
struct Message {
    long mtype;  // Message type
    int color;   // Color index
};

struct Global {
    Display *dpy;
    Window win;
    GC gc;
    int xres, yres;
} g;

int child = 0;
int msqid;  // Message Queue ID

void x11_cleanup_xwindows(void);
void x11_init_xwindows(void);
void x11_clear_window(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void render(void);
void create_child_window(void);
void *receive_messages(void *arg);  // Declare receive_messages function
void change_child_color(int color);  // Declare change_child_color function
void draw_color_boxes(void);  // Declare draw_color_boxes function

int main(int argc, char *argv[]) {
    XEvent e;
    int timer = 10000;  // Default timer value in milliseconds

    // Create message queue
    key_t key = ftok(".", 'm');
    msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    x11_init_xwindows();

    // Check if an argument was provided for timer
    if (argc > 1) {
        timer = atoi(argv[1]);
        if (timer < 0) {
            fprintf(stderr, "Timer value cannot be negative.\n");
            return 1;
        }
    }

    while (timer > 0) {
        while (XPending(g.dpy)) {
            XNextEvent(g.dpy, &e);
            check_mouse(&e);
            if (check_keys(&e)) {
                timer = 0; // Exit the loop if key pressed
                break;
            }
            render();
        }
        usleep(10000);  // Sleep for 10 milliseconds
        timer -= 10;    // Decrement timer by 10 milliseconds
    }

    // Cleanup and close message queue
    x11_cleanup_xwindows();
    msgctl(msqid, IPC_RMID, NULL);

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
    XStoreName(g.dpy, g.win, "cs3600 xwin phase-2");
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
        // Dark blue color for child window
        XSetForeground(g.dpy, g.gc, 0x0000008B);
        XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);

        XSetFont(g.dpy, g.gc, XLoadFont(g.dpy, "7x13bold"));
        XSetForeground(g.dpy, g.gc, 0x00ffffff);  // White color for text
        XDrawString(g.dpy, g.win, g.gc, 50, 50, "Child Window", strlen("Child Window"));
        XDrawString(g.dpy, g.win, g.gc, 50, 70, "Press Esc to exit", strlen("Press Esc to exit"));

    } else {
        // Yellow color for parent window
        XSetForeground(g.dpy, g.gc, 0x00ffff00);
        XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);

        XSetFont(g.dpy, g.gc, XLoadFont(g.dpy, "7x13bold"));

        XSetForeground(g.dpy, g.gc, 0x00000000);  // Black color for text
        XDrawString(g.dpy, g.win, g.gc, 50, 50, "Parent Window", strlen("Parent Window"));
        XDrawString(g.dpy, g.win, g.gc, 50, 70, "Press 'C' for child window", strlen("Press 'C' for child window"));
        XDrawString(g.dpy, g.win, g.gc, 50, 90, "Left-click mouse on colors", strlen("Left-click mouse on colors"));

        // Draw color boxes
        draw_color_boxes();
    }
    XFlush(g.dpy);
}

void draw_color_boxes(void) {
    int colors[MAX_COLORS] = {
        0x00FFB6C1, // Pastel Pink
        0x00E0FFFF, // Pastel Cyan
        0x00A52A2A, // Brown
        0x00FFA500, // Orange
        0x009ACD32  // Green
    };

    int box_x = 50;
    int box_y = 120;

    for (int i = 0; i < MAX_COLORS; ++i) {
        XSetForeground(g.dpy, g.gc, colors[i]);
        XFillRectangle(g.dpy, g.win, g.gc, box_x, box_y, COLOR_BOX_WIDTH, COLOR_BOX_HEIGHT);
        box_x += COLOR_BOX_WIDTH + 10;  // Add some spacing between boxes
    }
}

void check_mouse(XEvent *e) {
    static int event_count = 0;
    static int savex = 0;
    static int savey = 0;
    int mx = e->xbutton.x;
    int my = e->xbutton.y;
    if (e->type != ButtonPress && e->type != ButtonRelease && e->type != MotionNotify)
        return;
    if (e->type == ButtonPress) {
        if (e->xbutton.button == 1) {
            // Check if mouse click is on color box
            int box_x = 50;
            int box_y = 120;
            for (int i = 0; i < MAX_COLORS; ++i) {
                if (mx >= box_x && mx <= box_x + COLOR_BOX_WIDTH &&
                    my >= box_y && my <= box_y + COLOR_BOX_HEIGHT) {
                    // Send message to change color
                    struct Message msg;
                    msg.mtype = 1;  // Message type
                    msg.color = i;  // Color index
                    msgsnd(msqid, &msg, sizeof(msg.color), 0);
                    break;
                }
                box_x += COLOR_BOX_WIDTH + 10;  // Move to next box
            }
        }
        if (e->xbutton.button == 3) {
            // Send message to close child window on right-click
            struct Message msg;
            msg.mtype = 1;  // Message type
            msg.color = -1; // Special color to indicate close
            msgsnd(msqid, &msg, sizeof(msg.color), 0);
        }
    }
    if (e->type == MotionNotify) {
        if (child) {
            event_count++;
            if (event_count >= 10) {
                printf("c");
                fflush(stdout);
                event_count = 0;
            }
        } else {
            if (savex != mx || savey != my) {
                savex = mx;
                savey = my;
                event_count++;
                if (event_count >= 10) {
                    printf("m");
                    fflush(stdout);
                }
            }
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
            XSelectInput(g.dpy, g.win, ExposureMask | StructureNotifyMask | PointerMotionMask | KeyPressMask);
            child = 1;
            render();

            // Thread to receive messages from parent
            pthread_t tid;
            pthread_create(&tid, NULL, receive_messages, NULL);

            while (1) {
                XEvent e;
                XNextEvent(g.dpy, &e);
                switch (e.type) {
                    case Expose:
                        render();
                        break;
                    case KeyPress:
                        if (XLookupKeysym(&e.xkey, 0) == XK_Escape) {
                            // Exit child window if 'Esc' is pressed
                            exit(0);
                        }
                        break;
                    case MotionNotify:
                        // Print 'c' to console for mouse movement
                        printf("c");
                        fflush(stdout);
                        break;
                }
            }
            exit(0);
        }
    }
}

void *receive_messages(void *arg) {
    while (1) {
        struct Message msg;
        if (msgrcv(msqid, &msg, sizeof(msg.color), 1, 0) == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        if (msg.color == -1) {
            // Close child window
            exit(0);
        } else {
            // Change color based on received message
            change_child_color(msg.color);
        }
    }
    return NULL;
}

void change_child_color(int color) {
    // Colors array for child window
    int colors[MAX_COLORS] = {
        0x00FFB6C1, // Pastel Pink
        0x00E0FFFF, // Pastel Cyan
        0x00A52A2A, // Brown
        0x00FFA500, // Orange
        0x009ACD32  // Green
    };

    if (color < 0 || color >= MAX_COLORS) {
        fprintf(stderr, "Invalid color index.\n");
        return;
    }

    // Change child window color
    XSetForeground(g.dpy, g.gc, colors[color]);
    XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);
    XSetFont(g.dpy, g.gc, XLoadFont(g.dpy, "7x13bold"));
    XSetForeground(g.dpy, g.gc, 0x00ffffff);  // White color for text
    XDrawString(g.dpy, g.win, g.gc, 50, 50, "Child Window", strlen("Child Window"));
    XDrawString(g.dpy, g.win, g.gc, 50, 70, "Press Esc to exit", strlen("Press Esc to exit"));
    XFlush(g.dpy);
}
