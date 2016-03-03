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
 *  @bug  Return statuses of X-Window functions is not checked
 *  @todo Lock module so only one lisp interpreter can make a window
 *  **/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <pthread.h>
#include <liblisp.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define START_X         (10u)  /**< default start position (x)*/
#define START_Y         (20u)  /**< default start position (y)*/
#define START_HEIGHT    (400u) /**< default window height*/
#define START_WIDTH     (400u) /**< default window width*/
#define BORDER_WIDTH    (10u)  /**< default border width*/

#define SUBROUTINE_XLIST\
	X("clear-window",       subr_clear_window,   NULL, "clear a window")\
	X("create-window",      subr_create_window,  NULL, "create a new X11 window")\
	X("destroy-window",     subr_destroy_window, NULL, "destroy an X11 window")\
	X("draw-arc",           subr_draw_arc,       NULL, "draw a arc on a X11 window")\
	X("draw-line",          subr_draw_line,      NULL, "draw a line on a X11 window")\
	X("draw-rectangle",     subr_draw_rectangle, NULL, "draw a rectangle X11 window")\
	X("draw-text",          subr_draw_text,      NULL, "draw text on a X11 window")\
	X("erase-line",         subr_erase_line,     NULL, "erase a line on a X11 window")\
	X("erase-text",         subr_erase_text,     NULL, "erase text on a X11 window")\
	X("fill-arc",           subr_fill_arc,       NULL, "create a filled arc on a X11 window")\
	X("fill-rectangle",     subr_fill_rectangle, NULL, "fill a rectangle on a X11 window")\
	X("raise-window",       subr_raise_window,   NULL, "raise a X11 window")\
	X("resize-window",      subr_resize_window,  NULL, "resize a X11 window")\
	X("select-input",       subr_select_input,   NULL, "block until a X11 window gets an event")\
	X("set-background",     subr_set_background, NULL, "set the back ground color of an X11 window")\
	X("set-font",           subr_set_font,       NULL, "set the font for drawing text of all X11 windows")\
	X("set-foreground",     subr_set_foreground, NULL, "set the foreground drawing color of an X11 window")\
	X("window-information", subr_window_info,    NULL, "get information about an X11 window")

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { SUBR, NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING) },
static struct module_subroutines {
	subr p;
	char *name, *validation, *docstring;
} primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

static pthread_mutex_t mutex_single_threaded_module = PTHREAD_MUTEX_INITIALIZER;
static int initialized = 0;
static Display *xdisplay; /**< the display to be used by this module*/
static int xscreen;	  /**< the screen to use*/
static XSizeHints xhints; /**< hints for the window, such as its size*/
static XSetWindowAttributes xattributes; /**< window attributes*/
static Window xrootwin;	      /**< root window of screen*/
static GC solid_GC, clear_GC; /**< the graphics contexts */
static XGCValues solid_GC_values, clear_GC_values;
static Colormap xcolormap;

static int ud_x11 = 0;

static void close_window(Window w);
static void ud_x11_free(cell * f)
{
	if (!is_closed(f))
		close_window((Window) get_user(f));
	free(f);
}

static int ud_x11_print(io * o, unsigned depth, cell * f)
{
	return lisp_printf(NULL, o, depth, "%B<X-WINDOW:%d:%s>%t", get_user(f), is_closed(f) ? "CLOSED" : "OPEN");
}

static Window create_window(lisp *l)
{
	XFontStruct *fontstruct;	/* the font info to be used */
	Window w = 0;

	xhints.width = START_WIDTH;
	xhints.height = START_HEIGHT;
	xhints.x = START_X;
	xhints.y = START_Y;
	xhints.flags = PSize | PPosition;

	w = XCreateSimpleWindow(xdisplay, xrootwin,
				xhints.x, xhints.y,
				xhints.width, xhints.height, BORDER_WIDTH, BlackPixel(xdisplay, xscreen), WhitePixel(xdisplay, xscreen)
	    );

	XSetStandardProperties(xdisplay, w, "Default Window", "Icon", None, NULL, 0, &xhints);
	xcolormap = DefaultColormap(xdisplay, xscreen);

	solid_GC = XCreateGC(xdisplay, w, None, &solid_GC_values);
	clear_GC = XCreateGC(xdisplay, w, None, &clear_GC_values);

	XSetBackground(xdisplay, solid_GC, BlackPixel(xdisplay, xscreen));
	XSetForeground(xdisplay, solid_GC, BlackPixel(xdisplay, xscreen));
	XSetBackground(xdisplay, clear_GC, WhitePixel(xdisplay, xscreen));
	XSetForeground(xdisplay, clear_GC, WhitePixel(xdisplay, xscreen));

	if (!(fontstruct = XLoadQueryFont(xdisplay, "8x13"))) {
		lisp_printf(l, lisp_get_logging(l), 0, "could not open font\n");
		return -1;
	}

	XSetFont(xdisplay, solid_GC, fontstruct->fid);
	XSetFont(xdisplay, clear_GC, fontstruct->fid);

	XMapWindow(xdisplay, w);

	XChangeWindowAttributes(xdisplay, w, CWBitGravity, &xattributes);
	XFlush(xdisplay);

	return w;
}

static void close_window(Window w)
{
	XDestroyWindow(xdisplay, w);
	XFlush(xdisplay);
}

static cell *subr_create_window(lisp * l, cell * args)
{
	cell *ret;
	VALIDATE(l, __func__, 0, "", args, 1);
	if (!(ret = mk_user(l, (void *)create_window(l), ud_x11)))
		HALT(l, "\"%s\"", "out of memory");
	return ret;
}

static cell *subr_destroy_window(lisp * l, cell * args)
{
	if (!cklen(args, 1) || !is_usertype(car(args), ud_x11))
		RECOVER(l, "\"expected (window)\" '%S", args);
	close_window((Window) get_user(car(args)));
	close_cell(car(args));
	return car(args);
}

static cell *subr_draw_line(lisp * l, cell * args)
{
	if (!cklen(args, 5) || !is_usertype(car(args), ud_x11) ||
	    !is_int(CADR(args)) || !is_int(CADDR(args)) || !is_int(CADDDR(args)) || !is_int(CADDDDR(args)))
		RECOVER(l, "\"expected (window int-x1 int-y1 int-x2 int-y2)\" '%S", args);
	XDrawLine(xdisplay, (Window) get_user(car(args)), solid_GC,
		  get_int(CADR(args)), get_int(CADDR(args)), get_int(CADDDR(args)), get_int(CADDDDR(args)));
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_erase_line(lisp * l, cell * args)
{
	if (!cklen(args, 5) || !is_usertype(car(args), ud_x11) ||
	    !is_int(CADR(args)) || !is_int(CADDR(args)) || !is_int(CADDDR(args)) || !is_int(CADDDDR(args)))
		RECOVER(l, "\"expected (window int-x1 int-y1 int-x2 int-y2)\" '%S", args);
	XDrawLine(xdisplay, (Window) get_user(car(args)), clear_GC,
		  get_int(CADR(args)), get_int(CADDR(args)), get_int(CADDDR(args)), get_int(CADDDDR(args)));
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_draw_text(lisp * l, cell * args)
{
	if (!cklen(args, 4) || !is_usertype(car(args), ud_x11) || !is_str(CADR(args)) || !is_int(CADDR(args)) || !is_int(CADDDR(args)))
		RECOVER(l, "\"expected (window string int int)\" '%S", args);
	XDrawString(xdisplay, (Window) get_user(car(args)), solid_GC,
		    get_int(CADDR(args)), get_int(CADDDR(args)), get_str(CADR(args)), get_length(CADR(args)));
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_erase_text(lisp * l, cell * args)
{
	if (!cklen(args, 4) || !is_usertype(car(args), ud_x11) || !is_str(CADR(args)) || !is_int(CADDR(args)) || !is_int(CADDDR(args)))
		RECOVER(l, "\"expected (window string int int)\" '%S", args);
	XDrawString(xdisplay, (Window) get_user(car(args)), clear_GC,
		    get_int(CADDR(args)), get_int(CADDDR(args)), get_str(CADR(args)), get_length(CADR(args)));
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_clear_window(lisp * l, cell * args)
{
	if (!cklen(args, 1) || !is_usertype(car(args), ud_x11))
		RECOVER(l, "\"expected (window)\" '%S", args);
	XClearWindow(xdisplay, (Window) get_user(car(args)));
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_resize_window(lisp * l, cell * args)
{
	if (!cklen(args, 3) || !is_usertype(car(args), ud_x11)
	    || !is_int(CADR(args)) || !is_int(CADDR(args)))
		RECOVER(l, "\"expected (window int int)\" '%S", args);
	XResizeWindow(xdisplay, (Window) get_user(car(args)), get_int(CADR(args)), get_int(CADDR(args)));
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_raise_window(lisp * l, cell * args)
{
	if (!cklen(args, 1) || !is_usertype(car(args), ud_x11))
		RECOVER(l, "\"expected (window)\" '%S", args);
	XRaiseWindow(xdisplay, (Window) get_user(car(args)));
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_draw_arc(lisp * l, cell * args)
{
	int x, y, width, height, angle1, angle2;
	cell *v;
	if (!cklen(args, 7) || !is_usertype(car(args), ud_x11))
		RECOVER(l, "\"expected (window x y width height angle-1 angle-2)\" '%S", args);
	for (v = cdr(args); !is_nil(v); v = cdr(v))
		if (!is_int(car(v)))
			goto fail;
	v = cdr(args);
	x = get_int(car(v));
	v = cdr(v);
	y = get_int(car(v));
	v = cdr(v);
	width = get_int(car(v));
	v = cdr(v);
	height = get_int(car(v));
	v = cdr(v);
	angle1 = get_int(car(v));
	v = cdr(v);
	angle2 = get_int(car(v));
	v = cdr(v);
	XDrawArc(xdisplay, (Window) car(args), solid_GC, x, y, width, height, angle1, angle2);
	XFlush(xdisplay);
	return gsym_tee();
 fail:	RECOVER(l, "\"expected (window x y width height width height angle-1 angle-2)\" '%S", args);
	return gsym_error();
}

static cell *subr_draw_rectangle(lisp * l, cell * args)
{
	int x, y, width, height;
	cell *v;
	if (!cklen(args, 5) || !is_usertype(car(args), ud_x11))
		goto fail;
	for (v = cdr(args); !is_nil(v); v = cdr(v))
		if (!is_int(car(v)))
			goto fail;
	v = cdr(args);
	x = get_int(car(v));
	v = cdr(v);
	y = get_int(car(v));
	v = cdr(v);
	width = get_int(car(v));
	v = cdr(v);
	height = get_int(car(v));
	v = cdr(v);
	XDrawRectangle(xdisplay, (Window) get_user(car(args)), solid_GC, x, y, width, height);
	XFlush(xdisplay);
	return gsym_tee();
 fail:	RECOVER(l, "\"expected (window x y width height)\" '%S", args);
	return gsym_error();
}

static cell *subr_fill_arc(lisp * l, cell * args)
{
	int x, y, width, height, angle1, angle2;
	cell *v;
	if (!cklen(args, 7) || !is_usertype(car(args), ud_x11))
		goto fail;
	for (v = cdr(args); !is_nil(v); v = cdr(v))
		if (!is_int(car(v)))
			goto fail;
	v = cdr(args);
	x = get_int(car(v));
	v = cdr(v);
	y = get_int(car(v));
	v = cdr(v);
	width = get_int(car(v));
	v = cdr(v);
	height = get_int(car(v));
	v = cdr(v);
	angle1 = get_int(car(v));
	v = cdr(v);
	angle2 = get_int(car(v));
	v = cdr(v);
	XFillArc(xdisplay, (Window) car(args), solid_GC, x, y, width, height, angle1, angle2);
	XFlush(xdisplay);
	return gsym_tee();
 fail:	RECOVER(l, "\"expected (window x y width height width height angle-1 angle-2)\" '%S", args);
	return gsym_error();

}

static cell *subr_fill_rectangle(lisp * l, cell * args)
{
	int x, y, width, height;
	cell *v;
	if (!cklen(args, 5) || !is_usertype(car(args), ud_x11))
		goto fail;
	for (v = cdr(args); !is_nil(v); v = cdr(v))
		if (!is_int(car(v)))
			goto fail;
	v = cdr(args);
	x = get_int(car(v));
	v = cdr(v);
	y = get_int(car(v));
	v = cdr(v);
	width = get_int(car(v));
	v = cdr(v);
	height = get_int(car(v));
	v = cdr(v);
	XFillRectangle(xdisplay, (Window) get_user(car(args)), solid_GC, x, y, width, height);
	XFlush(xdisplay);
	return gsym_tee();
 fail:	RECOVER(l, "\"expected (window x y width height)\" '%S", args);
	return gsym_error();
}

static cell *subr_window_info(lisp * l, cell * args)
{
	Window rw;		/*root window */
	int x = 0, y = 0;
	unsigned width = 0, height = 0, border_width = 0, bit_depth = 0;
	if (!cklen(args, 1) || !is_usertype(car(args), ud_x11))
		RECOVER(l, "\"expected (window)\" '%S", args);
	XGetGeometry(xdisplay, (Window) get_user(car(args)), &rw, &x, &y, &width, &height, &border_width, &bit_depth);
	return mk_list(l, mk_user(l, (void *)rw, ud_x11),
		       mk_int(l, x), mk_int(l, y), mk_int(l, width), mk_int(l, height), mk_int(l, border_width), mk_int(l, bit_depth), NULL);
}

/** @todo make either a frame work, or hack, so this does not block 
 *  see https://stackoverflow.com/questions/8592292/how-to-quit-the-blocking-of-xlibs-xnextevent
 **/
static cell *subr_select_input(lisp * l, cell * args)
{				/*for event loop */
	XEvent e;
	KeySym key;
	char text[256];
	cell *rd, *ks, *mx, *my;
	if (!cklen(args, 1) || !is_usertype(car(args), ud_x11))
		RECOVER(l, "\"expected (window)\" '%S", args);
	rd = ks = mx = my = gsym_nil();
	XSelectInput(xdisplay, (Window) get_user(car(args)), ExposureMask | ButtonPressMask | KeyPressMask);
	XNextEvent(xdisplay, &e);
	if (e.type == Expose && !e.xexpose.count)
		rd = gsym_tee();	/*call a hypothetical redraw() function here */
	if (e.type == KeyPress && XLookupString(&e.xkey, text, 255, &key, 0) == 1)
		ks = mk_str(l, lstrdup(text));
	if (e.type == ButtonPress)
		mx = mk_int(l, e.xbutton.x), my = mk_int(l, e.xbutton.y);
	return mk_list(l, rd, ks, mx, my, NULL);
}

static cell *subr_set_font(lisp * l, cell * args)
{
	XFontStruct *fontstruct;
	if (!cklen(args, 1) || !is_str(car(args)))
		RECOVER(l, "\"expected (string)\" '%S", args);
	if ((fontstruct = XLoadQueryFont(xdisplay, get_str(car(args)))) == NULL)
		return gsym_nil();
	XSetFont(xdisplay, solid_GC, fontstruct->fid);
	XSetFont(xdisplay, clear_GC, fontstruct->fid);
	XFlush(xdisplay);
	return gsym_tee();
}

static cell *subr_set_background(lisp * l, cell * args)
{
	if (!cklen(args, 2) || !is_usertype(car(args), ud_x11) || !is_str(CADR(args)))
		RECOVER(l, "\"expected (window string)\" '%S", args);
	/**@todo implement this*/
	return gsym_nil();
}

static cell *subr_set_foreground(lisp * l, cell * args)
{
	if (!cklen(args, 1) || !is_str(car(args)))
		RECOVER(l, "\"expected (string)\" '%S", args);
	return gsym_nil();
}

int lisp_module_initialize(lisp *l)
{
	size_t i;
	assert(l);
	if(pthread_mutex_trylock(&mutex_single_threaded_module)) {
		if(lisp_verbose_modules) {
			lisp_printf(l, lisp_get_logging(l), 0, "module: line editor load failure (module already in use)\n");
		}
		return -1;
	}
	ud_x11 = new_user_defined_type(l, ud_x11_free, NULL, NULL, ud_x11_print);
	if (ud_x11 < 0)
		goto fail;
	for (i = 0; primitives[i].p; i++)	/*add all primitives from this module */
		if (!lisp_add_subr(l, primitives[i].name, primitives[i].p, primitives[i].validation, primitives[i].docstring))
			goto fail;
	if (!(xdisplay = XOpenDisplay(""))) {
		lisp_printf(l, lisp_get_logging(l), 0, "cannot open display\n");
		goto fail;
	}
	xscreen = DefaultScreen(xdisplay);
	xrootwin = RootWindow(xdisplay, xscreen);
	/*w = open_window(); */
	if (lisp_verbose_modules)
		lisp_printf(l, lisp_get_logging(l), 0, "module: x11 loaded\n");
	initialized = 1;
	return 0;
 fail:	lisp_printf(l, lisp_get_logging(l), 0, "module: x11 load failure\n");
	return -1;
}

static void cleanup(void)
{
	if (initialized)
		XCloseDisplay(xdisplay);
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void)
{
}

static void destruct(void)
{
	cleanup();
}
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpvReserved);
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		cleanup();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}
#endif
