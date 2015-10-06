/** @file       liblisp_x11.c
 *  @brief      a small X11 window module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *
 *  See:
 *  <http://math.msu.su/~vvb/2course/Borisenko/CppProjects/GWindow/xintro.html>
 *
 *  @todo Finish functionality
 *  @bug  Check return status of X-Window functions
 *  **/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <liblisp.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define START_X         (10u)
#define START_Y         (20u)
#define START_HEIGHT    (400u) /**< default window height*/
#define START_WIDTH     (400u) /**< default window width*/
#define BORDER_WIDTH    (10u)   /**< default border width*/

extern lisp *lglobal; /* from main.c */

#define SUBROUTINE_XLIST\
        X(subr_draw_line,      "draw-line")\
        X(subr_erase_line,     "erase-line")\
        X(subr_draw_text,      "draw-text")\
        X(subr_erase_text,     "erase-text")\
        X(subr_clear_window,   "clear-window")\
        X(subr_resize_window,  "resize-window")\
        X(subr_raise_window,   "raise-window")\
        X(subr_draw_arc,       "draw-arc")\
        X(subr_draw_rectangle, "draw-rectangle")\
        X(subr_fill_arc,       "fill-arc")\
        X(subr_fill_rectangle, "fill-rectangle")\
        X(subr_select_input,   "select-input")\
        X(subr_create_window,  "create-window")\
        X(subr_destroy_window, "destroy-window")\
        X(subr_window_info,    "window-information")\
        X(subr_set_font,       "set-font")\
        X(subr_set_foreground, "set-foreground")\
        X(subr_set_background, "set-background")

#define X(SUBR, NAME) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(SUBR, NAME) { SUBR, NAME },
static struct module_subroutines { subr p; char *name; } primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static Display *xdisplay; /**< the display to be used by this module*/
static int xscreen;       /**< the screen to use*/
static XSizeHints xhints; /**< hints for the window, such as its size*/
static XSetWindowAttributes xattributes; /**< window attributes*/
static Window xrootwin;       /**< root window of screen*/
static GC solid_GC, clear_GC; /**< the graphics contexts */
static XGCValues solid_GC_values, 
                 clear_GC_values;
static Colormap xcolormap;

static intptr_t ud_x11 = 0;

static void close_window(Window w);
static void ud_x11_free(cell *f) {
        if(!is_closed(f))
                close_window((Window)userval(f));
        free(f);
}

static int ud_x11_print(io *o, unsigned depth, cell *f) {
        return lisp_printf(NULL, o, depth, "%B<X-WINDOW:%d>%t", userval(f));
}

static Window create_window(void) {
        XFontStruct *fontstruct; /* the font info to be used */
        Window w = 0;

        xhints.width  = START_WIDTH;
        xhints.height = START_HEIGHT;
        xhints.x      = START_X;
        xhints.y      = START_Y;
        xhints.flags  = PSize | PPosition; 

        w = XCreateSimpleWindow(
                        xdisplay, xrootwin,
                        xhints.x, xhints.y,
                        xhints.width, xhints.height,
                        BORDER_WIDTH,
                        BlackPixel(xdisplay, xscreen),
                        WhitePixel(xdisplay, xscreen)
                        );

        XSetStandardProperties(xdisplay, w, "Default Window", "Icon", None, NULL, 0, &xhints);
        xcolormap = DefaultColormap(xdisplay, xscreen);

        solid_GC = XCreateGC(xdisplay, w, None, &solid_GC_values);
        clear_GC = XCreateGC(xdisplay, w, None, &clear_GC_values);

        XSetBackground(xdisplay, solid_GC, BlackPixel(xdisplay, xscreen));
        XSetForeground(xdisplay, solid_GC, BlackPixel(xdisplay, xscreen));
        XSetBackground(xdisplay, clear_GC, WhitePixel(xdisplay, xscreen));
        XSetForeground(xdisplay, clear_GC, WhitePixel(xdisplay, xscreen));

        if (!(fontstruct = XLoadQueryFont(xdisplay,"8x13"))) {
                lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "could not open font\n");
                return -1;
        }

        XSetFont(xdisplay, solid_GC, fontstruct->fid);
        XSetFont(xdisplay, clear_GC, fontstruct->fid);

        XMapWindow(xdisplay, w);

        XChangeWindowAttributes(xdisplay, w, CWBitGravity, &xattributes);
        XFlush(xdisplay);

        return w;
}

static void close_window(Window w) {
        XDestroyWindow(xdisplay, w);
        XFlush(xdisplay);
}

static cell* subr_create_window(lisp *l, cell *args) {
        cell *ret;
        lisp_validate(l, 0, "", args, 1);
        if(!(ret = mk_user(l, (void*)create_window(), ud_x11)))
                HALT(l, "\"%s\"", "out of memory");
        return ret;
}

static cell* subr_destroy_window(lisp *l, cell *args) {
        UNUSED(l); UNUSED(args);
        return gsym_nil();
}

static cell* subr_draw_line(lisp *l, cell *args) {
        if(!cklen(args, 5) || !is_usertype(car(args), ud_x11) || 
             !is_int(CADR(args))   || !is_int(CADDR(args)) || 
             !is_int(CADDDR(args)) || !is_int(CADDDDR(args)))
                RECOVER(l, "\"expected (window int-x1 int-y1 int-x2 int-y2)\" '%S", args);
        XDrawLine(xdisplay, (Window) userval(car(args)), solid_GC, 
                        intval(CADR(args)), intval(CADDR(args)), intval(CADDDR(args)), intval(CADDDDR(args)));
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_erase_line(lisp *l, cell *args) {
        if(!cklen(args, 5) || !is_usertype(car(args), ud_x11) || 
             !is_int(CADR(args))   || !is_int(CADDR(args)) || 
             !is_int(CADDDR(args)) || !is_int(CADDDDR(args)))
                RECOVER(l, "\"expected (window int-x1 int-y1 int-x2 int-y2)\" '%S", args);
        XDrawLine(xdisplay, (Window) userval(car(args)), clear_GC, 
                        intval(CADR(args)), intval(CADDR(args)), intval(CADDDR(args)), intval(CADDDDR(args)));
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_draw_text(lisp *l, cell *args) {
        if(!cklen(args, 4) || !is_usertype(car(args), ud_x11) || 
             !is_str(CADR(args)) || !is_int(CADDR(args))  || !is_int(CADDDR(args)))
                RECOVER(l, "\"expected (window string int int)\" '%S", args);
        XDrawString(xdisplay, (Window) userval(car(args)), solid_GC,
                intval(CADDR(args)), intval(CADDDR(args)), strval(CADR(args)), lisp_get_cell_length(CADR(args))); 
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_erase_text(lisp *l, cell *args) {
        if(!cklen(args, 4) || !is_usertype(car(args), ud_x11) || 
             !is_str(CADR(args)) || !is_int(CADDR(args))  || !is_int(CADDDR(args)))
                RECOVER(l, "\"expected (window string int int)\" '%S", args);
        XDrawString(xdisplay, (Window) userval(car(args)), clear_GC,
                intval(CADDR(args)), intval(CADDDR(args)), strval(CADR(args)), lisp_get_cell_length(CADR(args))); 
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_clear_window(lisp *l, cell *args) {
        if(!cklen(args, 1) || !is_usertype(car(args), ud_x11))
                RECOVER(l, "\"expected (window)\" '%S", args);
        XClearWindow(xdisplay, (Window) userval(car(args)));
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_resize_window(lisp *l, cell *args) {
        if(!cklen(args, 3) || !is_usertype(car(args), ud_x11)
            || !is_int(CADR(args))  || !is_int(CADDR(args)))
                RECOVER(l, "\"expected (window int int)\" '%S", args);
        XResizeWindow(xdisplay, (Window)userval(car(args)), intval(CADR(args)), intval(CADDR(args)));
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_raise_window(lisp *l, cell *args) {
        if(!cklen(args, 1) || !is_usertype(car(args), ud_x11))
                RECOVER(l, "\"expected (window)\" '%S", args);
        XRaiseWindow(xdisplay, (Window)userval(car(args)));
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_draw_arc(lisp *l, cell *args) {
        int x, y, width, height, angle1, angle2;
        cell *v;
        if(!cklen(args, 7) || !is_usertype(car(args), ud_x11))
                RECOVER(l, "\"expected (window x y width height angle-1 angle-2)\" '%S", args);
        for(v = cdr(args); !is_nil(v); v = cdr(v))
                if(!is_int(car(v)))
                        goto fail;
        v = cdr(args);
        x      = intval(car(v)); v = cdr(v);
        y      = intval(car(v)); v = cdr(v);
        width  = intval(car(v)); v = cdr(v);
        height = intval(car(v)); v = cdr(v);
        angle1 = intval(car(v)); v = cdr(v);
        angle2 = intval(car(v)); v = cdr(v);
        XDrawArc(xdisplay, (Window) car(args), solid_GC,
           x, y, width, height, angle1, angle2);
        XFlush(xdisplay);
        return gsym_tee();
fail:   RECOVER(l, "\"expected (window x y width height width height angle-1 angle-2)\" '%S", args);
        return gsym_error();
}

static cell* subr_draw_rectangle(lisp *l, cell *args) {
        int x, y, width, height;
        cell *v;
        if(!cklen(args, 5) || !is_usertype(car(args), ud_x11))
                goto fail;
        for(v = cdr(args); !is_nil(v); v = cdr(v))
                if(!is_int(car(v)))
                        goto fail;
        v = cdr(args);
        x      = intval(car(v)); v = cdr(v);
        y      = intval(car(v)); v = cdr(v);
        width  = intval(car(v)); v = cdr(v);
        height = intval(car(v)); v = cdr(v);
        XDrawRectangle(xdisplay, (Window)userval(car(args)), solid_GC, x, y, width, height) ;
        XFlush(xdisplay);
        return gsym_tee();
fail:   RECOVER(l, "\"expected (window x y width height)\" '%S", args);
        return gsym_error();
}

static cell* subr_fill_arc(lisp *l, cell *args) {
        int x, y, width, height, angle1, angle2; 
        cell *v;
        if(!cklen(args, 7) || !is_usertype(car(args), ud_x11))
                RECOVER(l, "\"expected (window x y width height angle-1 angle-2)\" '%S", args);
        for(v = cdr(args); !is_nil(v); v = cdr(v))
                if(!is_int(car(v)))
                        goto fail;
        v = cdr(args);
        x      = intval(car(v)); v = cdr(v);
        y      = intval(car(v)); v = cdr(v);
        width  = intval(car(v)); v = cdr(v);
        height = intval(car(v)); v = cdr(v);
        angle1 = intval(car(v)); v = cdr(v);
        angle2 = intval(car(v)); v = cdr(v);
        XFillArc(xdisplay, (Window) car(args), solid_GC,
           x, y, width, height, angle1, angle2);
        XFlush(xdisplay);
        return gsym_tee();
fail:   RECOVER(l, "\"expected (window x y width height width height angle-1 angle-2)\" '%S", args);
        return gsym_error();

}

static cell* subr_fill_rectangle(lisp *l, cell *args) {
        int x, y, width, height;
        cell *v;
        if(!cklen(args, 5) || !is_usertype(car(args), ud_x11))
                goto fail;
        for(v = cdr(args); !is_nil(v); v = cdr(v))
                if(!is_int(car(v)))
                        goto fail;
        v = cdr(args);
        x      = intval(car(v)); v = cdr(v);
        y      = intval(car(v)); v = cdr(v);
        width  = intval(car(v)); v = cdr(v);
        height = intval(car(v)); v = cdr(v);
        XFillRectangle(xdisplay, (Window)userval(car(args)), solid_GC, x, y, width, height) ;
        XFlush(xdisplay);
        return gsym_tee();
fail:   RECOVER(l, "\"expected (window x y width height)\" '%S", args);
        return gsym_error();
}

static cell* subr_window_info(lisp *l, cell *args) {
        Window rw; /*root window*/
        int x = 0, y = 0;
        unsigned width = 0, height = 0, border_width = 0, bit_depth = 0;
        if(!cklen(args, 1) || !is_usertype(car(args), ud_x11))
                RECOVER(l, "\"expected (window)\" '%S", args);
        XGetGeometry(xdisplay, (Window)userval(car(args)), &rw,
                        &x, &y, &width, &height, &border_width, &bit_depth);
        return cons(l, mk_user(l, (void*)rw, ud_x11),
                cons(l, mk_int(l, x),
                cons(l, mk_int(l, y),
                cons(l, mk_int(l, width),
                cons(l, mk_int(l, height),
                cons(l, mk_int(l, border_width),
                cons(l, mk_int(l, bit_depth), gsym_nil())))))));
}

static cell* subr_select_input(lisp *l, cell *args) { /*for event loop*/
        XEvent e;
        KeySym key;
        char text[256];
        cell *rd, *ks, *mx, *my;
        if(!cklen(args, 1) || !is_usertype(car(args), ud_x11))
                RECOVER(l, "\"expected (window)\" '%S", args);
        rd = ks = mx = my = gsym_nil();
        XSelectInput(xdisplay, (Window) userval(car(args)), ExposureMask|ButtonPressMask|KeyPressMask);
        XNextEvent(xdisplay, &e);
        if(e.type == Expose && !e.xexpose.count)
                rd = gsym_tee(); /*call a hypothetical redraw() function here*/
        if(e.type == KeyPress && XLookupString(&e.xkey, text, 255, &key, 0) == 1)
                ks = mk_str(l, lstrdup(text));
        if(e.type == ButtonPress)
                mx = mk_int(l, e.xbutton.x), my = mk_int(l, e.xbutton.y);
        return cons(l, rd, cons(l, ks, cons(l, mx, cons(l, my, gsym_nil()))));
}

static cell* subr_set_font(lisp *l, cell *args) { 
        XFontStruct *fontstruct; 
        if(!cklen(args, 1) || !is_str(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        if ((fontstruct = XLoadQueryFont(xdisplay, strval(car(args)))) == NULL)
                return gsym_nil();
        XSetFont(xdisplay, solid_GC, fontstruct->fid);
        XSetFont(xdisplay, clear_GC, fontstruct->fid);
        XFlush(xdisplay);
        return gsym_tee();
}

static cell* subr_set_background(lisp *l, cell *args) { 
        if(!cklen(args, 2) || !is_usertype(car(args), ud_x11) || !is_str(CADR(args)))
                RECOVER(l, "\"expected (window string)\" '%S", args);

        return gsym_nil();
}

static cell* subr_set_foreground(lisp *l, cell *args) { 
        if(!cklen(args, 1) || !is_str(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        return gsym_nil();
}

static void construct(void) {
        size_t i;
        assert(lglobal);
        for(i = 0; primitives[i].p; i++) /*add all primitives from this module*/
                if(!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p))
                        goto fail;
        ud_x11 = newuserdef(lglobal, ud_x11_free, NULL, NULL, ud_x11_print);
        if(!(xdisplay = XOpenDisplay(""))) {
                lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "cannot open display\n");
                goto fail;
        }
        xscreen  = DefaultScreen(xdisplay);
        xrootwin = RootWindow(xdisplay, xscreen);
        /*w = open_window();*/
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: x11 loaded\n");
        return;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: x11 load failure\n");
}

static void destruct(void) {
}

