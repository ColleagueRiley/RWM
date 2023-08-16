/* X11 */
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* standard libraries */
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
    RWM_HOVER = (1L << 3),
    RWM_HIDE = (1L << 4)
} client_status;

typedef struct client {
    Window window;
    Window border;
    Pixmap icon;

    unsigned int width;
    unsigned int status;
    unsigned int x, y, w, h;
} client;

typedef struct client_array {
    client* data;
    size_t cap;
    size_t len;
} client_array;


/* 
    basic syntax to add a new font
    font_name("font name")
*/

#define font_name(x)  "-*-" x "-medium-r-normal-*-12-*-*-*-*-*-iso8859-*"
char* font_names[] = {    
    font_name("dejavu sans"),
    font_name("liberation sans"),
    font_name("droid sans"),
    font_name("roboto"),
    font_name("ubuntu"),
    font_name("noto sans")
};

inline void redraw(Display* display, Pixmap p, client c) ;

#define RECT_COLLIDE(r, r2) (r.x + r.width >= r2.x && r.x <= r2.x + r2.width && r.y + r.height >= r2.y && r.y <= r2.y + r2.height)

void redraw(Display* display, Pixmap p, client c) {
    if (c.border == None)
        return;

    if (RWM_HOVER & c.status)
        XClearArea(display, c.border, c.width - 70, 0, 70, 32, false);

    int s = XDefaultScreen(display);
    GC gc = XDefaultGC(display, s);

    XSetForeground(display, gc, RGB(27, 34, 36));
    XFillRectangle(display, c.border, gc, 0, 0, c.width, 32);

    XWindowAttributes attr;
    XGetWindowAttributes(display, c.window, &attr);

    char* name = NULL;

    XFetchName(display, c.window, &name);

    if (name != NULL) {
        size_t len = strlen(name);
        XSetForeground(display, gc, RGB(90, 97, 100));
        XDrawString(display, c.border, XDefaultGC(display, s), (c.width - 200 - (len * 2)) / 4 , 20, name, len);

        XFree(name);
    }

    XCopyArea(display, p, c.border, gc, 0, 0, 70, 32, c.width - 70, 0);
}

void initWindow(Display* display, Window win, Pixmap p, client_array* clients) {
    int i;

    for (i = 0; i < clients->len; i++)
        if (clients->data[i].window == win || 
            clients->data[i].border == win
        )
            return;

    XWindowAttributes attr;
    XGetWindowAttributes(display, win, &attr);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char* prop_data = NULL;

    if (clients->cap < (clients->len + 1)) {
        clients->cap += 5;
        
        client* nclients = malloc(sizeof(client) * clients->cap);
        memcpy(nclients, clients->data, sizeof(client) * (clients->len + 1));

        free(clients->data);
        clients->data = nclients;
    } 

    int s = XDefaultScreen(display);
    
    /* check if the window wants a border or not */
    bool border = true; 

    if (attr.override_redirect)
        border = false;

    /* if window has no border atom */
    if (XGetWindowProperty(display, win, net_wm_window_type, 0, 1, False,
                            XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after,
                            &prop_data) == Success) 
                for (i = 0; i < nitems; i++)
                    if (actual_format == 32 && ((Atom*)prop_data)[i] == net_wm_window_type_dock)
                        border = false;

    static Atom atom = None;
    if (atom == None)
        atom = XInternAtom(display, "_MOTIF_WM_HINTS", False);

    unsigned char *prop;

    if (XGetWindowProperty(display, win, atom, 0, 5, False, AnyPropertyType,
                        &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success) {
        
        if (nitems == 5 && (!((long *)prop)[0] & (1L << 1)))
            border = false;

        XFree(prop);
    }

    clients->data[clients->len].window = win;
    clients->data[clients->len].border = None;  
printf("h\n");
    if (border) {
        clients->data[clients->len].border = XCreateSimpleWindow(display, RootWindow(display, s), attr.x, ((attr.y) ? attr.y - 32 : 0), attr.width, 32, 1,
                                                            BlackPixel(display, s), WhitePixel(display, s)); 
        

        XSelectInput(display, clients->data[clients->len].border, LeaveWindowMask|VisibilityChangeMask|ExposureMask|PointerMotionMask); 

        clients->data[clients->len].width = attr.width;
        clients->data[clients->len].status = 0;

        XChangeProperty(display, clients->data[clients->len].border, net_wm_window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&net_wm_window_type_dock, 1);

        XMapWindow(display, clients->data[clients->len].border);

        XSetWindowBorder(display, clients->data[clients->len].border, RGB(27, 34, 36));

        if (attr.y < 32)
            XMoveResizeWindow(display, win, attr.x, attr.y + 32, attr.width, attr.height);
    }

    clients->data[clients->len].icon = None;

    XWMHints* h = XAllocWMHints();
    h->flags = IconMaskHint;
    h->icon_pixmap = clients->data[clients->len].icon;

    XSizeHints* sh = XAllocSizeHints();
    sh->flags = PSize | PPosition;
    sh->min_width = 32;
    sh->min_height = 32;
    
    XSetWMHints(display, win, h);
    XSetWMSizeHints(display, win, sh, XA_WM_NORMAL_HINTS);

    XFree(sh);
    XFree(h);

    XSync(display, 0);

    clients->len++;
}