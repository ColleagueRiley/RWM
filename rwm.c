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

/* standard library headers */
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <X11/Xutil.h> /* X11 utilities */
#include <X11/cursorfont.h>

/* utilities, macros, functions, structures */
#include "util.h"

int main(int argc, char** argv) {
    if (argc > 1) { /* check for args */
        /* check if this arg is "end" */ 
        if (*(int*)argv[1] == *(int*)"end\0") {
            FILE* f = fopen("/tmp/RWM_END", "w+");
            
            if (f)
                fclose(f);
            return 0;
        }
    } 

    /* init */
    
    /* create display */
    Display* display = XOpenDisplay(0);    

    if (display == NULL) {
        printf("RWM : Failed to load X11 display. Please ensure an X11 server is running.\n");
        return 1;
    }

    /* events */

    int i;
    for (i = 0; i < 2; i++) /* grab button (1 and 3)*/
        XGrabButton(display, 1 + (2 * i), 0, DefaultRootWindow(display), True, 
                           ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeSync, GrabModeAsync, None, None);

    //for (i = 0; i < 2; i++) /* grab XK_Alt_L and XK_Alt_R*/
    //    XGrabKey(display, XK_Alt_L + i, Mod1Mask, DefaultRootWindow(display), True, GrabModeAsync, GrabModeSync);

    XGrabKey(display, XK_x, ControlMask | Mod1Mask, XDefaultRootWindow(display), false, GrabModeAsync, GrabModeAsync);

    XSelectInput(display, XDefaultRootWindow(display), StructureNotifyMask|KeyPressMask|KeyReleaseMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|SubstructureNotifyMask); 

    /* load atoms */

    net_wm_window_type = XInternAtom((Display *)display, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_dock = XInternAtom((Display *)display, "_NET_WM_WINDOW_TYPE_DOCK", False);

    Atom WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);


    /* variables for the loop */
    XWindowAttributes attr;
    XButtonEvent start;

    Window createWindow = None, focusWindow, cursorWindow;

    start.subwindow = None;

    int s = XDefaultScreen(display);

    /* load + set font */
    XFontStruct* font_info = NULL;
    for (i = 0; i < sizeof(font_names) / sizeof(char*) && font_info == NULL; i++)
        font_info = XLoadQueryFont(display, font_names[i]);
    
    if (font_info)
        XSetFont(display, XDefaultGC(display, s), font_info->fid);
    else
        printf("Failed to load a font, using default font\n");

    /* draw basic title bar button onto pixmap */
    Pixmap p = XCreatePixmap(display, XDefaultRootWindow(display), 70, 32, XDefaultDepth(display, s));
    GC gc = XDefaultGC(display, s);

    XSetForeground(display, gc, RGB(27, 34, 36));
    XFillRectangle(display, p, gc, 0, 0, 70, 32);

    XSetForeground(display, gc, RGB(90, 97, 100));

    /* hide button */
    XPoint points[3] = {
                        3, (32 / 2) - 4,
                        0, (32 / 2) + 2,
                        6, (32 / 2) + 2
                    };
    
    XFillPolygon(display, p, gc, points, 3, Convex, CoordModeOrigin);
    XDrawRectangle(display, p, gc, 0, (32 / 2) + 3, 6, 1);

    /* minimize */
    XDrawRectangle(display, p, gc, 15, 32 / 2, 6, 1);

    /* fullscreen */
    XDrawRectangle(display, p, gc, 30, (32 / 2) - 5, 10, 10);

    /* draw x button*/
    XDrawLine(display, p, gc, 49, 11,                 49 + 10,     11 + 10);
    XDrawLine(display, p, gc, 49 + 10,  11,           49, 11 + 10);

    /* set background */
    XSetWindowBackground(display, XDefaultRootWindow(display), RGB(100, 100, 120));
    XClearWindow(display, XDefaultRootWindow(display));

    /* init windows that already exist */

    Window* wins;
    Window w, w2;
    unsigned int size;

    XQueryTree(display, XDefaultRootWindow(display), &w, &w2, &wins, &size);

    /* init clients array */
    client_array clients;
    clients.cap = ((size > 10) ? (size + 5) : 10);
    clients.data = malloc(sizeof(client) * clients.cap);
    clients.len = 0;

    for (i = 0; i < size; i++)
        initWindow(display, wins[i], p, &clients);

    /* cursor */
    Cursor cursors[] = {
                        XCreateFontCursor(display, XC_arrow),
                        XCreateFontCursor(display, XC_pirate),
                        
                        XCreateFontCursor(display, XC_left_side),
                        XCreateFontCursor(display, XC_top_left_corner),
                        XCreateFontCursor(display, XC_top_side),
                        XCreateFontCursor(display, XC_top_right_corner),

                        XCreateFontCursor(display, XC_right_side),
                        XCreateFontCursor(display, XC_bottom_right_corner),
                        XCreateFontCursor(display, XC_bottom_side),
                        XCreateFontCursor(display, XC_bottom_left_corner),
                    };

    size_t currentCursor = 0, cursetCursor = 0;

    XEvent ev;

    while (1) {
        XNextEvent(display, &ev);

        if (cursetCursor != currentCursor) {
            XDefineCursor(display, XDefaultRootWindow(display), cursors[currentCursor]);
        
            cursetCursor = currentCursor;
        }

        switch (ev.type) {
            case Expose:
            case VisibilityNotify: {
                if (
                    ev.type == VisibilityNotify && 
                    ev.xvisibility.state != VisibilityPartiallyObscured
                )
                    break;

                Window w = (ev.type == Expose) ? ev.xexpose.window : ev.xvisibility.window;

                XWindowAttributes attr;
                XGetWindowAttributes(display, w, &attr);

                if (attr.map_state == IsUnmapped)
                    break;

                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].border != w)
                        continue;

                    redraw(display, p, clients.data[i], font_info);
                    break;
                }
                break;
            }
            case KeyPress:
                if (ev.xkey.keycode == XK_x)
                    currentCursor = 1;
                break;
            case ButtonPress:                            
                XAllowEvents(display, ReplayPointer, ev.xbutton.time);
                XSync(display, 0);

                if(ev.xbutton.subwindow == None)
                    break;

                XGetWindowAttributes(display, ev.xbutton.subwindow, &attr);

                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].window != ev.xbutton.subwindow && 
                        clients.data[i].border != ev.xbutton.subwindow
                    )
                        continue;
                    
                    currentCursor = 0;
                    if (clients.data[i].window == ev.xbutton.subwindow) {
                        int x = ev.xmotion.x - attr.x;
                        if (x <= 6) {
                                if ((ev.xmotion.y - attr.y - attr.height) + 3 >= 3)
                                    currentCursor = 9;
                                else 
                                    currentCursor = 2;
                        }
                        else if (x >= clients.data[i].width - 6) {
                            if ((ev.xmotion.y - attr.y - attr.height) + 3 >= 3)
                                currentCursor = 7;
                            else
                                currentCursor = 6;
                        }
                        else if ((ev.xmotion.y - attr.y - (attr.height - 3)) >= 3)
                            currentCursor = 8;
                    }

                    else if (clients.data[i].border == ev.xbutton.subwindow || currentCursor) {
                        start = ev.xbutton;

                        XGrabPointer(display, DefaultRootWindow(display), false,
                                    ButtonPressMask|ButtonReleaseMask|ButtonMotionMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, 
                                    RootWindow(display, XDefaultScreen(display)), None, CurrentTime);
                        
                    }

                    XRaiseWindow(display, clients.data[i].border);
                    XRaiseWindow(display, clients.data[i].window);

                    XWindowAttributes attr;
                    XGetWindowAttributes(display, clients.data[i].window, &attr);

                    if (attr.map_state == IsViewable) {
                        focusWindow = clients.data[i].window;
                        XSetInputFocus(display, clients.data[i].window, RevertToNone, CurrentTime);
                    }

                    break;
                }
                break;
            case MotionNotify:
                XAllowEvents(display, ReplayPointer, ev.xmotion.time);
                XSync(display, 0);

                currentCursor = 0;

                if (start.subwindow == None) {
                    for (i = 0; i < clients.len; i++) {
                        XWindowAttributes a;

                        if (XDefaultRootWindow(display) == ev.xbutton.window) {
                            XGetWindowAttributes(display, clients.data[i].window, &a);
                         
                            if (RECT_COLLIDE_POINT(ev.xmotion.x, ev.xmotion.y, a.x - 2, a.y - 2, a.width + 4, a.height + 4) == false)
                                continue;

                            int x = ev.xmotion.x - a.x;
                            if (x <= 6) {
                                    if ((ev.xmotion.y - a.y - a.height) + 3 >= 3)
                                        currentCursor = 9;
                                    else 
                                        currentCursor = 2;
                            }
                            else if (x >= clients.data[i].width - 6) {
                                if ((ev.xmotion.y - a.y - a.height) + 3 >= 3)
                                    currentCursor = 7;
                                else
                                    currentCursor = 6;
                            }
                            else if ((ev.xmotion.y - a.y - a.height) + 3 >= 3)
                                currentCursor = 8;

                            cursorWindow = clients.data[i].window; 
                            
                            break;
                        }

                        else if (clients.data[i].border != ev.xbutton.window)
                            continue;
                        
                        XGetWindowAttributes(display, clients.data[i].border, &a);

                        int x = 70 + ev.xbutton.x_root - a.x - a.width;

                        if (RWM_HOVER & clients.data[i].status)
                            redraw(display, p, clients.data[i], font_info);

                        XSetForeground(display, gc,  RGB(37, 44, 46));  
                        if (between(x, 15, 21)) {
                            clients.data[i].status |= RWM_HOVER;

                            XFillRectangle(display, clients.data[i].border, gc, a.width - 70 + 11, 0, 16, 32);
                            
                            XSetForeground(display, gc, RGB(90, 97, 100));
                            XDrawRectangle(display, clients.data[i].border, gc, (a.width - 70 + 15), 32 / 2, 6, 1);
                        }
                        else if (between(x, 30, 40)) {
                            clients.data[i].status |= RWM_HOVER;
                            
                            XFillRectangle(display, clients.data[i].border, gc, a.width - 70 + 30 - 2, 0, 16, 32);
                            
                            XSetForeground(display, gc, RGB(90, 97, 100));
                            XDrawRectangle(display, clients.data[i].border, gc, (a.width - 70 + 30), (32 / 2) - 5, 10, 10);
                        }
                        else if (between(x, 49, 59)) {
                            clients.data[i].status |= RWM_HOVER;
                            
                            XSetForeground(display, gc, RGB(202, 86, 92));
                            XFillRectangle(display, clients.data[i].border, gc, a.width - 60 + 41 - 2, 0, 16, 32);

                            XSetForeground(display, gc, RGB(90, 97, 100));
                            XDrawLine(display, clients.data[i].border, gc, (a.width - 60 + 41), 11,                 (a.width - 60 + 41) + 10,     11 + 10);
                            XDrawLine(display, clients.data[i].border, gc, (a.width - 60 + 41) + 10,  11,           (a.width - 60 + 41), 11 + 10);
                        } 
                        else {
                            if (ev.xmotion.y > 3)
                                break;
                            
                            if (ev.xmotion.x <= 6)
                                currentCursor = 3;
                            else if (ev.xmotion.x >= clients.data[i].width - 6)
                                currentCursor = 5;
                            else    
                                currentCursor = 4;
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

                    if ((attr.x + attr.width >= DisplayWidth(display, s) && xdiff > 0) || 
                        (attr.x <= 0 && xdiff < 0))
                            xdiff = 0; 

                    if ((attr.y <= 32 && ydiff < 0) ||
                        (attr.y + attr.height >= DisplayHeight(display, s) && ydiff > 0))
                            ydiff = 0;
                    
                    if (xdiff == 0 && ydiff == 0)
                        break;


                    printf("%i\n", currentCursor);
                    XWindowAttributes a;
                    XGetWindowAttributes(display, clients.data[i].window, &a);

                    XMoveResizeWindow(display, clients.data[i].border,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0),
                        a.width, 32
                    );

                    XMoveResizeWindow(display, clients.data[i].window,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0) + 32,
                        a.width, a.height
                    );

                    break;
                }

                break;
            case LeaveNotify:
                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].border != ev.xmotion.window)
                        continue;

                    if (RWM_HOVER & clients.data[i].status)
                        redraw(display, p, clients.data[i], font_info);

                    break;
                }
                break;
            case ButtonRelease: {
                Window w = start.subwindow;
                start.subwindow = None;
                XUngrabPointer(display, CurrentTime);

                XAllowEvents(display, ReplayPointer, ev.xbutton.time);
                XSync(display, 0);
                
                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].border != w || clients.data[i].border == None) 
                        continue;

                    XWindowAttributes a;
                    XGetWindowAttributes(display, w, &a);
                    
                    int x = 70 + ev.xbutton.x_root - a.x - a.width;
                    
                    if (between(x, 0, 14)) {
                        if (RWM_HIDE & clients.data[i].status) {
                            XMapWindow(display, clients.data[i].window);
                            clients.data[i].status ^= RWM_HIDE;
                            break;
                        }

                        XUnmapWindow(display, clients.data[i].window);
                        clients.data[i].status |= RWM_HIDE;
                    }
                    if (between(x, 15, 21)) {
                        XUnmapWindow(display, clients.data[i].border);
                        XUnmapWindow(display, clients.data[i].window);
                        clients.data[i].status |= RWM_MINI;
                    }
                    else if (between(x, 30, 40)) {
                        if (RWM_FULL & clients.data[i].status) {
                            XMoveResizeWindow(display, clients.data[i].window, clients.data[i].x, clients.data[i].y, clients.data[i].w, clients.data[i].h);
                            XMoveResizeWindow(display, clients.data[i].border, clients.data[i].x, clients.data[i].y - 32, clients.data[i].w, clients.data[i].h);
                            clients.data[i].width = clients.data[i].w;
                            clients.data[i].status ^= RWM_FULL;
                            break;
                        }
  
                        XGetWindowAttributes(display, clients.data[i].window, &a);
                        clients.data[i].x = a.x;
                        clients.data[i].y = a.y;
                        clients.data[i].w = a.width;
                        clients.data[i].h = a.height;
                        clients.data[i].status |= RWM_FULL;

                        XMoveResizeWindow(display, clients.data[i].window, 0, 32, DisplayWidth(display, s), DisplayHeight(display, s) - 32);
                        XMoveResizeWindow(display, clients.data[i].border, 0, 0, DisplayWidth(display, s), 32);
                        clients.data[i].width = DisplayWidth(display, s);
                        break;
                    }
                    else if (between(x, 49, 59)) {
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
                break;
            }
            case UnmapNotify:
                if (focusWindow == ev.xunmap.window)
                    XSetInputFocus(display, XDefaultRootWindow(display), RevertToNone, CurrentTime);

                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].window != ev.xunmap.window)
                        continue;

                    if (RWM_HIDE & clients.data[i].status)
                        break;
                    
                    if (clients.data[i].border != None)
                        XUnmapWindow(display, clients.data[i].border);
                    break;
                }
                break;
            case DestroyNotify:
                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].window != ev.xdestroywindow.window)
                        continue;

                    if (clients.data[i].border != None)
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
            case CreateNotify:
                createWindow = ev.xcreatewindow.window;
                break;

            case MapNotify:
                if (createWindow == ev.xmap.window) {
                    initWindow(display, ev.xmap.window, p, &clients);
                    createWindow = None;
                }
                break;
            case ConfigureNotify:
                for (i = 0; i < clients.len; i++) {
                    if (clients.data[i].window != ev.xconfigure.window)
                        continue;

                    XMoveResizeWindow(display, clients.data[i].border, ev.xconfigure.x, ev.xconfigure.y - 32, ev.xconfigure.width, 32);
                    clients.data[i].width = ev.xconfigure.width;
                }
                break;
            default:
                XFlush(display);
                ev.type = 0;
                break;
        }  
        
        struct stat tmp;
        if (stat("/tmp/RWM_END", &tmp) == 0) {
            remove("/tmp/RWM_END");
            break;
        }
    }

    quit:

    for (i = 0; i < sizeof(cursors) / sizeof(Cursor); i++)
        XFreeCursor(display, cursors[i]);

    XSetInputFocus(display, XDefaultRootWindow(display), RevertToNone, CurrentTime);

    XFreePixmap(display, p);
    for (i = 0; i < clients.len; i++) {
        XDestroyWindow(display, clients.data[i].border);
        
        if (clients.data[i].title != None)
            XFreePixmap(display, clients.data[i].title);
    }

    free(clients.data);

    XCloseDisplay(display);
}