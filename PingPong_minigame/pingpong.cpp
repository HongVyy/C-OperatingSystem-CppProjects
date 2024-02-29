//adding score bar and gameover display
//g++ -o ping_pong pingpong.cpp -lX11
//./ping_pong

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <iostream>
#include <unistd.h> 
struct Ball {
    int x, y; // Position
    int xdir, ydir; // Direction
    int size;
};

// Paddle structure
struct Paddle {
    int x, y; // Position
    int width, height;
};

// Function to check collision between ball and paddle
bool checkCollision(Ball ball, Paddle paddle) {
    return ball.x >= paddle.x &&
           ball.x <= paddle.x + paddle.width &&
           ball.y >= paddle.y &&
           ball.y <= paddle.y + paddle.height;
}

int main() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Error: Unable to open X display" << std::endl;
        return -1;
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                        10, 10, 500, 300, 1,
                                        BlackPixel(display, screen),
                                        WhitePixel(display, screen));

    XSelectInput(display, window, KeyPressMask);
    XMapWindow(display, window);

    GC gc = XCreateGC(display, window, 0, nullptr);
    XSetForeground(display, gc, BlackPixel(display, screen));

    XEvent event;
    bool running = true;

    // Initialize ball
    Ball ball;
    ball.x = 250;
    ball.y = 150;
    ball.xdir = -1;
    ball.ydir = 1;
    ball.size = 10;

    // Initialize paddles
    Paddle player1, player2;
    player1.x = 10;
    player1.y = 100;
    player1.width = 10;
    player1.height = 60;

    player2.x = 480;
    player2.y = 100;
    player2.width = 10;
    player2.height = 60;

    // Game loop
    while (running) {
        while (XPending(display)) {
            XNextEvent(display, &event);
            if (event.type == KeyPress) {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                if (key == XK_w) {
                    player1.y -= 10;
                } else if (key == XK_s) {
                    player1.y += 10;
                } else if (key == XK_Up) {
                    player2.y -= 10;
                } else if (key == XK_Down) {
                    player2.y += 10;
                }
            }
        }

        // Move ball
        ball.x += ball.xdir;
        ball.y += ball.ydir;

        // Check collision with walls
        if (ball.y <= 0 || ball.y >= 300 - ball.size) {
            ball.ydir = -ball.ydir;
        }

        // Check collision with paddles
        if (checkCollision(ball, player1) || checkCollision(ball, player2)) {
            ball.xdir = -ball.xdir;
        }

        // Check for scoring
        if (ball.x <= 0 || ball.x >= 500 - ball.size) {
            ball.x = 250;
            ball.y = 150;
            ball.xdir = -ball.xdir;
        }

        // Clear the screen
        XClearWindow(display, window);

        // Draw ball
        XFillArc(display, window, gc, ball.x, ball.y, ball.size, ball.size, 0, 360 * 64);

        // Draw paddles
        XFillRectangle(display, window, gc, player1.x, player1.y, player1.width, player1.height);
        XFillRectangle(display, window, gc, player2.x, player2.y, player2.width, player2.height);

        XFlush(display);

        // Pause for a moment to control speed
        usleep(10000); // 10 ms
    }

    XCloseDisplay(display);
    return 0;
}

