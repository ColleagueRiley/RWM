/*
* Copyright (C) 2023 ColleagueRiley
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
*
*/

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define RGB(r, g, b) (b + ( g << 8) + ( r << 16))

#define between(x, lower, upper) (((lower) <= (x)) && ((x) <= (upper)))

Atom net_wm_window_type, net_wm_window_type_dock;
Display* display;

typedef struct client {
    Window window;
    Window border;
    unsigned int width;
    bool mini, full;
    unsigned int x, y, w, h;
} client;

client* clients;
size_t clients_cap;
size_t clients_len;

Pixmap p;
inline void redraw();

void redraw() {
    client* c;
    
    int s = XDefaultScreen(display);
    GC gc = XDefaultGC(display, s);

    for (c = clients; (c - clients) < clients_len; c++) {
        XSetForeground(display, gc, RGB(27, 34, 36));
        XFillRectangle(display, c->border, gc, 0, 0, c->width, 32);
        
        XCopyArea(display, p, c->border, gc, 0, 0, 60, 32, c->width - 60, 0);

        char* name = NULL;
        XFetchName(display, c->window, &name);

        if (name != NULL) {
            size_t len = strlen(name);
            XSetForeground(display, gc, RGB(90, 97, 100));
            XDrawString(display, c->border, XDefaultGC(display, s), ((c->width - 100) + len) / 6, 20, name, len);
        }

        XFree(name);

        static Atom NET_WM_ICON = (Atom)NULL;
        if (NET_WM_ICON == (Atom)NULL) 
            NET_WM_ICON = XInternAtom(display, "_NET_WM_ICON", False);

        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char *data = NULL;
        
        if (XGetWindowProperty(display, c->window, NET_WM_ICON, 0, 1024, False, XA_STRING,
                                &actual_type, &actual_format, &nitems, &bytes_after, &data) == Success) {
            
            if (actual_type == XA_CARDINAL && actual_format == 32 && data != NULL) {
                XImage *image = XCreateImage(display, DefaultVisual(display, s), 32, ZPixmap, 0,
                                            (char *)data, 32, 32, 32, 0);

                XPutImage(display, c->border, XDefaultGC(display, s), image,
                        0, 0, 0, 0, 5, 5);
            }
            
            XFree(data);
        }
 
    }
}

void initWindow(Window win) {
    XWindowAttributes attr;
    XGetWindowAttributes(display, win, &attr);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char* prop_data = NULL;

    Atom window_type = XInternAtom((Display *)display, "_NET_WM_WINDOW_TYPE", False);

    int i;

    /* if window has no border atom */
    if (XGetWindowProperty(display, win, net_wm_window_type, 0, 1, False,
                            XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after,
                            &prop_data) == Success) 
                for (i = 0; i < nitems; i++)
                    if (actual_format == 32 && ((Atom*)prop_data)[i] == net_wm_window_type_dock)
                        return;

    clients_len++;

    if (clients_cap < clients_len) {
        clients_cap++;
        clients = realloc(clients, sizeof(client) * (clients_len + 5));
    } 

    int s = s;
    
    clients[clients_len - 1].window = win;
    clients[clients_len - 1].border = XCreateSimpleWindow(display, RootWindow(display, s), attr.x, attr.y, attr.width, 32, 1,
                                                        BlackPixel(display, s), WhitePixel(display, s));

    clients[clients_len - 1].width = attr.width;
    clients[clients_len - 1].full = false;
    clients[clients_len - 1].mini = false;

    XChangeProperty(display, clients[clients_len - 1].border, net_wm_window_type, ((Atom)4), 32, PropModeReplace, (unsigned char *)&net_wm_window_type_dock, 1);

    XMapWindow(display, clients[clients_len - 1].border);

    XSetWindowBorderWidth(display, clients[clients_len - 1].border, 0);

    XSetWindowBorder(display, win, RGB(27, 34, 36));
    XMoveResizeWindow(display, win, attr.x, attr.y + 32, attr.width, attr.height);

    redraw(clients[i]);
}


int main(char** argv, int argc) {
    clients = malloc(sizeof(client) * 10);
    clients_cap = 10;
    clients_len = 0;
 
    display = XOpenDisplay(0);    

    if (display == NULL) {
        printf("RWM : Failed to load X11 display. Please ensure an X11 server is running.\n");
        return 1;
    }

    int i;
    for (i = 0; i < 2; i++) /* grab button (1 and 3)*/
        XGrabButton(display, 1 + (2 * i), 0, DefaultRootWindow(display), True, 
                            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    XSelectInput(display, XDefaultRootWindow(display), ExposureMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|KeyReleaseMask|PointerMotionMask|SubstructureNotifyMask); 

    net_wm_window_type = XInternAtom((Display *)display, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_dock = XInternAtom((Display *)display, "_NET_WM_WINDOW_TYPE_DOCK", False);

    Atom WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);

    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    start.subwindow = None;

    int s = XDefaultScreen(display);

    /* load + st font */
    XFontStruct *font_info = XLoadQueryFont(display, "fixed");
    XSetFont(display, XDefaultGC(display, s), font_info->fid);

    Window* wins;
    Window w, w2;
    unsigned int size;

    XQueryTree(display, XDefaultRootWindow(display), &w, &w2, &wins, &size);

    /* draw basic title bar button onto pixmap */
    p = XCreatePixmap(display, XDefaultRootWindow(display), 60, 32, XDefaultDepth(display, s));

    GC gc = XDefaultGC(display, s);

    XSetForeground(display, gc, RGB(27, 34, 36));
    XFillRectangle(display, p, gc, 0, 0, 60, 32);

    XSetForeground(display, gc, RGB(90, 97, 100));

    /* minimize */
    XDrawRectangle(display, p, gc, 0, 32 / 2, 16, 1);

    /* fullscreen */
    XDrawRectangle(display, p, gc, 21, 8, 16, 16);
    XDrawRectangle(display, p, gc, 22, 8 + 1, 8, 8);

    /* draw x button*/
    XSetForeground(display, gc, RGB(202, 86, 92));
    XFillArc(display, p, gc, 42, 8, 16, 16, 0, 360 * 64);

    XSetForeground(display, gc, RGB(27, 34, 36));
    XPoint points[4] = {
            42, 8,              42 + 16, 8 + 16,
            42 + 16, 8,         42, 8 + 16
    };

    XDrawLines(display, p, gc, points, 4, 0);

    for (i = 0; i < size; i++)
        initWindow(wins[i]);

    while (1) {
        XNextEvent(display, &ev);
        switch (ev.type) {
            case ButtonPress:
                if(ev.xbutton.subwindow == None)
                    break;

                XGetWindowAttributes(display, ev.xbutton.subwindow, &attr);
                start = ev.xbutton;

                for (i = 0; i < clients_len; i++) {
                    if (clients[i].window != start.subwindow && 
                        clients[i].border != start.subwindow
                    )
                        continue;
                
                    XRaiseWindow(display, clients[i].border);
                    XRaiseWindow(display, clients[i].window);

                    redraw();
                    break;
                }
    
                break;
            case MotionNotify:
                if (start.subwindow == None)
                    break;

                int xdiff = ev.xbutton.x_root - start.x_root;
                int ydiff = ev.xbutton.y_root - start.y_root;

                for (i = 0; i < clients_len; i++) {
                    if (clients[i].border != start.subwindow)
                        continue;

                    XMoveResizeWindow(display, clients[i].border,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0),
                        MAX(1, attr.width + (start.button == 3 ? xdiff : 0)),
                        32);

                    XWindowAttributes a;
                    XGetWindowAttributes(display, clients[i].window, &a);
                    XMoveResizeWindow(display, clients[i].window,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0) + 32,
                        a.width, a.height
                    );
    
                    redraw(clients[i]);

                    break;
                }

                break;
            case ButtonRelease:
                for (i = 0; i < clients_len; i++) {
                    if (clients[i].border != start.subwindow) 
                        continue;

                    XWindowAttributes a;
                    XGetWindowAttributes(display, clients[i].border, &a);
                    
                    int x = 60 + ev.xbutton.x_root - a.x - a.width;
                    
                    if (between(x, 0, 20)) {
                        XUnmapWindow(display, clients[i].border);
                        XUnmapWindow(display, clients[i].window);
                        clients[i].mini = true;
                    }
                    else if (between(x, 21, 41)) {
                        if (clients[i].full == false) {
                            XGetWindowAttributes(display, clients[i].window, &a);
                            clients[i].x = a.x;
                            clients[i].y = a.y;
                            clients[i].w = a.width;
                            clients[i].h = a.height;
                            clients[i].full = true;

                            XMoveResizeWindow(display, clients[i].window, 0, 32, DisplayWidth(display, s), DisplayHeight(display, s) - 32);
                            XMoveResizeWindow(display, clients[i].border, 0, 0, DisplayWidth(display, s), 32);
                            clients[i].width = DisplayWidth(display, s);
                            
                            redraw();
                            break;
                        }

                        XMoveResizeWindow(display, clients[i].window, clients[i].x, clients[i].y, clients[i].w, clients[i].h);
                        XMoveResizeWindow(display, clients[i].border, clients[i].x, 32, clients[i].w, clients[i].h);
                        clients[i].width = clients[i].w;
                        clients[i].full = false;

                        redraw();
                    }
                    else if (between(x, 42, 60)) {
                        XEvent event;
                        event.xclient.type = ClientMessage;
                        event.xclient.window = clients[i].window;
                        event.xclient.message_type = WM_PROTOCOLS;
                        event.xclient.format = 32;
                        event.xclient.data.l[0] = WM_DELETE_WINDOW;
                        event.xclient.data.l[1] = CurrentTime;
                        XSendEvent(display, clients[i].window, False, NoEventMask, &event);
                    }
                    break; 
                }

                start.subwindow = None;
                break;
            case CreateNotify:
                initWindow(ev.xcreatewindow.window);
                break;
            case DestroyNotify:
                for (i = 0; i < clients_len; i++) {
                    if (clients[i].window != ev.xdestroywindow.window)
                        continue;

                    XDestroyWindow(display, clients[i].border);
                
                    /* 
                    clients +_ 
                    */
                    clients_cap--;
                    client* nclients = malloc(sizeof(client) * clients_cap);
                    memcpy(nclients, clients, (sizeof(client) * (i + 1)));
                    
                    if (clients_cap - 1 > i)
                        memcpy(nclients + i, clients + (i + 1), (sizeof(client) * (clients_cap - i)));

                    free(clients);
                    clients = nclients;

                    clients_len--;
                    break;
                }
                break;
		    case ClientMessage:
			    if (ev.xclient.data.l[0] == (long int)XInternAtom(display, "WM_DELETE_WINDOW", 1))
                    goto quit;
                break;

            default:

                XFlush(display);
                ev.type = 0;
                break;
        }  
    }

    quit:

    for (i = 0; i < clients_len; i++) 
        XDestroyWindow(display, clients[i].border);
    
    free(clients);

    XCloseDisplay(display);
}