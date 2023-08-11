#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define RGB(r, g, b) (b + ( g << 8) + ( r << 16))

#define between(x, lower, upper) (((lower) <= (x)) && ((x) <= (upper)))

Atom net_wm_window_type, net_wm_window_type_dock;

typedef enum {
    RWM_FULL = (1L << 1),
    RWM_MINI = (1L << 2),
    RWM_HOVER = (1L << 3)
} client_status;

typedef struct client {
    Window window;
    Window border;
    unsigned int width;
    unsigned int status;
    unsigned int x, y, w, h;
} client;

typedef struct client_array {
    client* data;
    size_t cap;
    size_t len;
} client_array;

inline void redraw(Display* display, Pixmap p, client_array* clients, int i, bool check) ;

#define RECT_COLLIDE(r, r2) (r.x + r.width >= r2.x && r.x <= r2.x + r2.width && r.y + r.height >= r2.y && r.y <= r2.y + r2.height)

void redraw(Display* display, Pixmap p, client_array* clients, int i, bool check) {
    client c = clients->data[i];

    if (check) {
        XWindowAttributes w, wb;
        XGetWindowAttributes(display, c.window, &w);
        XGetWindowAttributes(display, c.border, &wb);

        int i;
        for (i = 0; i < clients->len; i++) {
            XWindowAttributes b;
            XGetWindowAttributes(display, clients->data[i].border, &b);

            if (RECT_COLLIDE(w, b) || RECT_COLLIDE(wb, b))
                redraw(display, p, clients, i, false);
        }

        return;
    }

    int s = XDefaultScreen(display);
    GC gc = XDefaultGC(display, s);

    XSetForeground(display, gc, RGB(27, 34, 36));
    XFillRectangle(display, c.border, gc, 0, 0, c.width, 32);

    char* name = NULL;
    XFetchName(display, c.window, &name);

    if (name != NULL) {
        size_t len = strlen(name);
        XSetForeground(display, gc, RGB(90, 97, 100));
        XDrawString(display, c.border, XDefaultGC(display, s), (c.width - 100 - (len * 2)) / 4, 20, name, len);
    }

    XFree(name);

    XCopyArea(display, p, c.border, gc, 0, 0, 60, 32, c.width - 60, 0);

    static Atom NET_WM_ICON = (Atom)NULL;
    if (NET_WM_ICON == (Atom)NULL) 
        NET_WM_ICON = XInternAtom(display, "_NET_WM_ICON", False);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    
    if (XGetWindowProperty(display, c.window, NET_WM_ICON, 0, 1024, False, XA_STRING,
                            &actual_type, &actual_format, &nitems, &bytes_after, &data) == Success) {
        
        if (actual_type == XA_CARDINAL && actual_format == 32 && data != NULL) {
            XImage *image = XCreateImage(display, DefaultVisual(display, s), 32, ZPixmap, 0,
                                        (char *)data, 32, 32, 32, 0);

            //XPutImage(display, c.border, XDefaultGC(display, s), image,
            //        0, 0, 0, 0, 5, 5);
        }
        
        XFree(data);
    }
}

void initWindow(Display* display, Window win, Pixmap p, client_array* clients) {
    XWindowAttributes attr;
    XGetWindowAttributes(display, win, &attr);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char* prop_data = NULL;

    int i;

    /* if window has no border atom */
    if (XGetWindowProperty(display, win, net_wm_window_type, 0, 1, False,
                            XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after,
                            &prop_data) == Success) 
                for (i = 0; i < nitems; i++)
                    if (actual_format == 32 && ((Atom*)prop_data)[i] == net_wm_window_type_dock)
                        return;

    if (clients->cap < clients->len) {
        clients->cap += 5;
        clients->data = realloc(clients->data, clients->cap);
    } 

    int s = XDefaultScreen(display);
    
    clients->data[clients->len].window = win;  
    clients->data[clients->len].border = XCreateSimpleWindow(display, RootWindow(display, s), attr.x, ((attr.y) ? attr.y - 32 : 0), attr.width, 32, 1,
                                                        BlackPixel(display, s), WhitePixel(display, s)); 
    
    clients->data[clients->len].width = attr.width;
    clients->data[clients->len].status = 0;

    XChangeProperty(display, clients->data[clients->len].border, net_wm_window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&net_wm_window_type_dock, 1);

    XMapWindow(display, clients->data[clients->len].border);


    XSetWindowBorder(display, win, RGB(27, 34, 36));
    XSetWindowBorder(display, clients->data[clients->len].border, RGB(27, 34, 36));

    if (attr.y == 0)
        XMoveResizeWindow(display, win, attr.x, attr.y + 32, attr.width, attr.height);

    redraw(display, p, clients, clients->len, false);
    redraw(display, p, clients, clients->len, true);

    clients->len++;
}