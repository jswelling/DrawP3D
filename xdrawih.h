/****************************************************************************
 * xdrawih.h
 * Author Joe Demers, Damerlin Thompson
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/

static Boolean dbg = False, dbg_good = False, dbg_cb = False;

typedef struct _XdIH {
  int pixheight,pixwidth,screen;
  Boolean inited;
  Display *dpy;
  Window win;
  XWindowAttributes attrib; 
  Pixmap current_pixmap;
  XStandardColormap map_info;
  int map_info_set;
  int map_info_owned;
  GC gc;
  unsigned int win_x_size, win_y_size;
  /* unsigned int image_x_size, image_y_size;
     XImage ximage; */
#ifdef LOCAL
  Boolean using_local;
#else
#ifdef SHM
  Boolean using_shm;
  XShmSegmentInfo shminfo;
#endif /* SHM */
#endif /* LOCAL */

  void (*init_fun)(struct _XdIH* self);
  void (*display_fun)(struct _XdIH* self, XImage *ximage_in);
} XdIH;

typedef struct _XmadIH {
  int pixheight,pixwidth,redrawn_before;
  Atom atomcolormapwindows;
  XdIH *the_image;
  Widget draw_me,draw_parent;
  Boolean does_top;
} XmadIH;

void* xdrawih_create(Display *d_in, int s_in, Window w_in, 
				int width, int height);
void xdrawih_delete(void *XdrawImageHandler);
void xdrawih_ppoint(void *XdrawImageHandler,
			       int r, int g, int b, int pnum, XPoint *points);
void xdrawih_pline(void *XdrawImageHandler,
			      int r, int g, int b, int pnum, XPoint *points);
void xdrawih_pgon(void *XdrawImageHandler,
			     int r, int g, int b, int pnum, XPoint *points);
void xdrawih_winsize(void *XdrawImageHandler, 
				int *height, int *width);
void xdrawih_redraw(void *XdrawImageHandler);
void xdrawih_clear(void *XdrawImageHandler, int r, int g, int b);
Window xdrawih_window( void *XdrawImageHandler );

void* xmadih_create(Widget parent, int height, int width);
void xmadih_delete(void *XmautodrawImageHandler);
void xmadih_ppoint(void *XmautodrawImageHandler,
			      int r, int g, int b, int pnum, XPoint *points);
void xmadih_pline(void *XmautodrawImageHandler,
			     int r, int g, int b, int pnum, XPoint *points);
void xmadih_pgon(void *XmautodrawImageHandler,
			    int r, int g, int b, int pnum, XPoint *points);
void xmadih_winsize(void *XmautodrawImageHandler, 
			       int *height, int *width);
void xmadih_redraw(void *XmautodrawImageHandler);
void xmadih_clear(void *XmautodrawImageHandler, 
			     int r, int g, int b);
Widget xmadih_widget( void *XmautodrawImageHandler );
Window xmadih_window( void *XmautodrawImageHandler );

static void get_window_size(XdIH *self);
static unsigned long calc_pixel(XdIH *self,int r, int g, int b);
static void monochrome_init(XdIH *self);
static void monochrome_display(XdIH *self, XImage *ximage_in);
static void directcolor_init(XdIH *self);
static void directcolor_display(XdIH *self, XImage *ximage_in);
static void pseudocolor_init(XdIH *self);
static void pseudocolor_display(XdIH *self, XImage *ximage_in);
static int server_is_sun_or_hp(XdIH *self);
static void generate_sun_rgbmap(XdIH *self);
static int pxl_compare( const void *p1, const void *p2 );

static void redraw_meCB(Widget w, XmadIH *self, caddr_t call_data);
static void resize_meCB(Widget w, XmadIH *self, caddr_t call_data);
static Boolean check(XmadIH *self);
static void xmadih_resize( void *XmautodrawImageHandler );
static void xmadih_resize_IH(XmadIH *self);

