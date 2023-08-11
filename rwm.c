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

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

int main(int argc, char** argv) {
    if (argc > 1) {
        if (*(int*)argv[1] == *(int*)"end\0") {
            FILE* f = fopen("/tmp/RWM_END", "w+");
            
            if (f)
                fclose(f);
            return 0;
        }
    } 

	struct stat tmp;

    client_array clients; 
    clients.data = malloc(sizeof(client) * 10);
    clients.cap = 10;
    clients.len = 0;
 
    Display* display = XOpenDisplay(0);    

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
    Pixmap p = XCreatePixmap(display, XDefaultRootWindow(display), 60, 32, XDefaultDepth(display, s));

    GC gc = XDefaultGC(display, s);

    XSetForeground(display, gc, RGB(27, 34, 36));
    XFillRectangle(display, p, gc, 0, 0, 61, 32);

    XSetForeground(display, gc, RGB(90, 97, 100));

    /* minimize */
    XDrawRectangle(display, p, gc, 7, 32 / 2, 6, 1);

    /* fullscreen */
    XDrawRectangle(display, p, gc, 22, (32 / 2) - 5, 10, 10);

    /* draw x button*/
    XDrawLine(display, p, gc, 41, 11,                 41 + 10,     11 + 10);
    XDrawLine(display, p, gc, 41 + 10,  11,           41, 11 + 10);

    for (i = 0; i < size; i++)
        initWindow(display, wins[i], p, &clients);

    while (1) {
        XNextEvent(display, &ev);
        switch (ev.type) {
            case ButtonPress:
                if(ev.xbutton.subwindow == None)
                    break;

                XGetWindowAttributes(display, ev.xbutton.subwindow, &attr);
                start = ev.xbutton;

                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].window != start.subwindow && 
                        clients.data[i].border != start.subwindow
                    )
                        continue;
                
                    XRaiseWindow(display, clients.data[i].border);
                    XRaiseWindow(display, clients.data[i].window);

                    redraw(display, p, &clients, i, true);
                    break;
                }
    
                break;
            case MotionNotify:
                if (start.subwindow == None) {
                    for (i = 0; i < clients.len; i++) {
                        if (clients.data[i].border != ev.xbutton.subwindow)
                            continue;

                        XWindowAttributes a;
                        XGetWindowAttributes(display, clients.data[i].border, &a);
                        
                        int x = 60 + ev.xbutton.x_root - a.x - a.width;
                        
                        XSetForeground(display, gc,  RGB(37, 44, 46));
                        if (between(x, 7, 13)) {
                            XFillRectangle(display, clients.data[i].border, gc, a.width - 60 + 7 - 4, 0, 16, 32);
                            clients.data->status |= RWM_HOVER;

                            XSetForeground(display, gc, RGB(90, 97, 100));
                            XDrawRectangle(display, clients.data[i].border, gc, (a.width - 60 + 7), 32 / 2, 6, 1);
                        }
                        else if (between(x, 22, 32)) {
                            XFillRectangle(display, clients.data[i].border, gc, a.width - 60 + 21 - 1, 0, 16, 32);
                            clients.data->status |= RWM_HOVER;
                            
                            XSetForeground(display, gc, RGB(90, 97, 100));
                            XDrawRectangle(display, clients.data[i].border, gc, (a.width - 60 + 22), (32 / 2) - 5, 10, 10);
                        }
                        else if (between(x, 41, 51)) {
                            XSetForeground(display, gc, RGB(202, 86, 92));
                            XFillRectangle(display, clients.data[i].border, gc, a.width - 60 + 41 - 2, 0, 16, 32);

                            XSetForeground(display, gc, RGB(90, 97, 100));
                            XDrawLine(display, clients.data[i].border, gc, (a.width - 60 + 41), 11,                 (a.width - 60 + 41) + 10,     11 + 10);
                            XDrawLine(display, clients.data[i].border, gc, (a.width - 60 + 41) + 10,  11,           (a.width - 60 + 41), 11 + 10);
                            clients.data->status |= RWM_HOVER;
                        } 
                        else if (RWM_HOVER & clients.data->status) {
                            clients.data->status |= !RWM_HOVER;
                            XClearWindow(display, clients.data[i].border);
                            redraw(display, p, &clients, i, false);
                        }

                        break;
                    }
                    break;
                }

                int xdiff = ev.xbutton.x_root - start.x_root;
                int ydiff = ev.xbutton.y_root - start.y_root;

                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].border != start.subwindow)
                        continue;

                    XMoveResizeWindow(display, clients.data[i].border,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0),
                        MAX(1, attr.width + (start.button == 3 ? xdiff : 0)),
                        32);

                    XWindowAttributes a;
                    XGetWindowAttributes(display, clients.data[i].window, &a);
                    XMoveResizeWindow(display, clients.data[i].window,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0) + 32,
                        a.width, a.height
                    );

                    redraw(display, p, &clients, i, true);
                    break;
                }

                break;
            case ButtonRelease:
                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].border != start.subwindow) 
                        continue;

                    XWindowAttributes a;
                    XGetWindowAttributes(display, clients.data[i].border, &a);
                    
                    int x = 60 + ev.xbutton.x_root - a.x - a.width;
                    
                    if (between(x, 7, 13)) {
                        XUnmapWindow(display, clients.data[i].border);
                        XUnmapWindow(display, clients.data[i].window);
                        clients.data->status |= RWM_MINI;
                    }
                    else if (between(x, 22, 32)) {
                        if (RWM_FULL & clients.data[i].status) {
                            XMoveResizeWindow(display, clients.data[i].window, clients.data[i].x, clients.data[i].y, clients.data[i].w, clients.data[i].h);
                            XMoveResizeWindow(display, clients.data[i].border, clients.data[i].x, clients.data[i].y - 32, clients.data[i].w, clients.data[i].h);
                            clients.data[i].width = clients.data[i].w;
                            clients.data->status ^= RWM_FULL;

                            redraw(display, p, &clients, i, true);
                            break;
                        }
  
                        XGetWindowAttributes(display, clients.data[i].window, &a);
                        clients.data[i].x = a.x;
                        clients.data[i].y = a.y;
                        clients.data[i].w = a.width;
                        clients.data[i].h = a.height;
                        clients.data->status |= RWM_FULL;

                        XMoveResizeWindow(display, clients.data[i].window, 0, 32, DisplayWidth(display, s), DisplayHeight(display, s) - 32);
                        XMoveResizeWindow(display, clients.data[i].border, 0, 0, DisplayWidth(display, s), 32);
                        clients.data[i].width = DisplayWidth(display, s);
                        
                        redraw(display, p, &clients, i, true);
                        break;
                    }
                    else if (between(x, 41, 51)) {
                        XEvent event;
                        event.xclient.type = ClientMessage;
                        event.xclient.window = clients.data[i].window;
                        event.xclient.message_type = WM_PROTOCOLS;
                        event.xclient.format = 32;
                        event.xclient.data.l[0] = WM_DELETE_WINDOW;
                        event.xclient.data.l[1] = CurrentTime;
                        XSendEvent(display, clients.data[i].window, False, NoEventMask, &event);
                    }
                    break; 
                }

                start.subwindow = None;
                break;
            case CreateNotify:
                usleep(20000);
                initWindow(display, ev.xcreatewindow.window, p, &clients);
                break;
            case DestroyNotify:
                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].window != ev.xdestroywindow.window)
                        continue;

                    XDestroyWindow(display, clients.data[i].border);
                
                    /* 
                    clients +_ 
                    */
                    clients.cap--;
                    client* nclients = malloc(sizeof(client) * clients.cap);
                    memcpy(nclients, clients.data, (sizeof(client) * (i + 1)));
                    
                    if (clients.cap - 1 > i)
                        memcpy(nclients + i, clients.data + (i + 1), (sizeof(client) * (clients.cap - i)));

                    free(clients.data);
                    clients.data = nclients;

                    clients.len--;
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
        
        if (stat("/tmp/RWM_END", &tmp) == 0) {
            remove("/tmp/RWM_END");
            break;
        }
    }

    quit:

    XFreePixmap(display, p);
    for (i = 0; i < clients.len; i++) 
        XDestroyWindow(display, clients.data[i].border);
    
    free(clients.data);

    XCloseDisplay(display);
}