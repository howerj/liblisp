/** @file       liblisp_x11.c
 *  @brief      a small X11 window module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *
 *  See:
 *  <http://math.msu.su/~vvb/2course/Borisenko/CppProjects/GWindow/xintro.html>
 *  **/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <assert.h>
#include <liblisp.h>

#define START_X         (0u)
#define START_Y         (0u)
#define START_HEIGHT    (400u) /**< default window height*/
#define START_WIDTH     (400u) /**< default window width*/
#define BORDER_WIDTH    (1u)   /**< default border width*/

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
static XFontStruct *xfontstruct; /* the font info to be used */

static Window open_window(void) {
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

        if (!(xfontstruct = XLoadQueryFont(xdisplay,"8x13"))) {
                lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "could not open font\n");
                return -1;
        }

        XSetFont(xdisplay, solid_GC, xfontstruct->fid);
        XSetFont(xdisplay, clear_GC, xfontstruct->fid);

        XMapWindow(xdisplay, w);

        XChangeWindowAttributes(xdisplay, w, CWBitGravity, &xattributes);
        XFlush(xdisplay);

        return w;
}

static void close_window(Window w) {
        XDestroyWindow(xdisplay, w);
        XFlush(xdisplay);
}

static cell* subr_draw_line(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_erase_line(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_draw_text(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_erase_text(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_clear_window(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_resize_window(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_raise_window(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_draw_arc(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_draw_rectangle(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_fill_arc(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_fill_rectangle(lisp *l, cell *args) {
        return gsym_nil();
}

static cell* subr_select_input(lisp *l, cell *args) { /*event loop*/
        return gsym_nil();
}

static void construct(void) {
        size_t i;
        Window w;
        assert(lglobal);
        for(i = 0; primitives[i].p; i++) /*add all primitives from this module*/
                if(!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p))
                        goto fail;
        if(!(xdisplay = XOpenDisplay(""))) {
                lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "cannot open display\n");
                goto fail;
        }
        xscreen  = DefaultScreen(xdisplay);
        xrootwin = RootWindow(xdisplay, xscreen);
        w = open_window();
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: x11 loaded\n");
        return;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: x11 load failure\n");
}

static void destruct(void) {
}

