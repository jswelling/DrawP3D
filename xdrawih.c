/****************************************************************************
 * xdrawih.c
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmu/StdCmap.h>

#include <Xm/BulletinB.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/MwmUtil.h>
#include <X11/cursorfont.h>

#ifndef LOCAL
#ifdef SHM
#include <sys/types.h>
#include <sys/ipc.h>
/*#include <sys/shm.h>*/
#include <X11/extensions/xshm.h>
#endif /* SHM */
#endif /* LOCAL */

#ifdef SHARE_XWS_CMAP
extern XStandardColormap* xws_best_rgb_stdmap;
#endif

#include "xdrawih.h"


static Cursor watch=0;

static int pxl_compare( const void *p1, const void *p2 ) {
  /* This routine is used by qsort to sort a list of pixel values. */
  
  if(dbg_good) fprintf(stderr,"pxl_compare\n");
  if (*(long *)p1 > *(long *)p2) return(-1);
  if (*(long *)p1 == *(long *)p2) return(0);
  return(1);
}

static void generate_sun_rgbmap(XdIH *self) {
  /* This routine generates an rgb map that won't frighten X11/News */
  
  int noSet= 0, noGrabbed= 0, noAlloced= 0;
  XColor *myColors= (XColor *)0;
  unsigned long pixels[512]; 
  int contig_pixels, start_pixel= 0;
  int l, m;
  unsigned int i, j, k;
  int found_map_space;
  
  if(dbg) fprintf(stderr,"generate_sun_rgbmap\n");
  /* The actual color map will be the display's default map */
  self->map_info.colormap= self->attrib.colormap;
  self->map_info_set= 1;
  self->map_info_owned= 0;
  
  /* grab as many colours as possible */ 
  /* assume < 512 colours needed */ 
  for (l=256; l>0; l /= 2) {
    if (XAllocColorCells(self->dpy, self->map_info.colormap, False, 0, 0,
			 pixels + noAlloced, l)) {
      noAlloced += l;
    }
  }
  /* Sort the available pixels */
  (void)qsort(pixels, noAlloced, sizeof(long), pxl_compare);
  
  /* Find a contiguous stretch to make into a map (top preferred) */
  m= 0;
  found_map_space= 0;
  while (!found_map_space && m<noAlloced) {
    contig_pixels= 0;
    start_pixel= m;
    m += 1;
    while (m<noAlloced && pixels[m] == pixels[m-1]-1) {
      contig_pixels++;
      m++;
    }
    if (contig_pixels>=64) { /* Found a place to put the map */
      if (contig_pixels>=125) { /* 5x5x5 case */
	self->map_info.red_max= 4;
	self->map_info.green_max= 4;
	self->map_info.blue_max= 4;
      }
      else if (contig_pixels>=100) { /* 5x5x4 case */
	self->map_info.red_max= 4;
	self->map_info.green_max= 4;
	self->map_info.blue_max= 3;
      }
      else if (contig_pixels>=80) { /* 5x4x4 case */
	self->map_info.red_max= 4;
	self->map_info.green_max= 3;
	self->map_info.blue_max= 3;
      }
      else { /* 4x4x4 case */
	self->map_info.red_max= 3;
	self->map_info.green_max= 3;
	self->map_info.blue_max= 3;
      }
      self->map_info.blue_mult= 1;
      self->map_info.green_mult= self->map_info.blue_max+1;
      self->map_info.red_mult= self->map_info.green_mult * (self->map_info.green_max + 1);
      self->map_info.base_pixel= start_pixel;
      noSet = (int) ((self->map_info.red_max + 1) * (self->map_info.green_max + 1)
			 * (self->map_info.blue_max + 1));
      found_map_space= 1;
    }
  }
  if (!found_map_space) {
    fprintf(stderr,
     "generate_sun_rgbmap: couldn't find color table space!\n");
    exit(-1);
  }
  self->map_info.base_pixel= pixels[0] - noSet + 1;

  /* Free up unneeded color cells */
  if (start_pixel != 0) 
    XFreeColors(self->dpy, self->map_info.colormap, pixels, start_pixel, 0);
  if (start_pixel+noSet < noAlloced)
    XFreeColors(self->dpy, self->map_info.colormap, pixels+start_pixel+noSet, 
		noAlloced-(start_pixel+noSet), 0);    
  
  /* make up our default colour table */ 
  myColors= (XColor *)malloc(noSet*sizeof(XColor));

  /* Create the map */
  noGrabbed= 0;
  for (i=0; i<=self->map_info.red_max; ++i)
      for (j=0;j<=self->map_info.green_max; ++j)
	for (k=0; k<=self->map_info.blue_max;++k) {
	  myColors[noGrabbed].flags =
	    DoRed | DoGreen | DoBlue;
	myColors[noGrabbed].pixel = self->map_info.base_pixel + 
	    i * self->map_info.red_mult +
	      j * self->map_info.green_mult +
		k * self->map_info.blue_mult;
	/* now fill out the values */
	myColors[noGrabbed].red = (unsigned short)
	    ((i * 65535)/ self->map_info.red_max);
	myColors[noGrabbed].green = (unsigned short)
	    ((j * 65535) / self->map_info.green_max);
	myColors[noGrabbed].blue = (unsigned short)
	    ((k * 65535)/ self->map_info.blue_max);
	noGrabbed++;
    } 
  
  /* now store the colours, should be OK */ 
  XStoreColors(self->dpy, self->map_info.colormap, myColors, noGrabbed); 
  
  /* Clean up */
  free(myColors);
}

static int server_is_sun_or_hp(XdIH *self) {
/* This routine returns true if the display is a Sun X11/News or HP server */

  char *vendor= XServerVendor(self->dpy);

  if(dbg) fprintf(stderr,"server_is_sun_or_hp\n");
  return( !strncmp(vendor, "X11/NeWS", 8)
         || !strncmp(vendor, "Sun Microsystems, Inc.", 22)
         || !strncmp(vendor, "Hewlett-Packard Company",23) );
}

static void monochrome_init(XdIH *self) {
  if(dbg) fprintf(stderr,"monochrome_init\n");
  self->inited=True;
}

static void monochrome_display(XdIH *self, XImage *ximage_in) {

  if(dbg) fprintf(stderr,"monochrome_display\n");
}

static void directcolor_init(XdIH *self) {
  /* We assume this is a 24 bit system with 8 bits per color, but
     what order are the colors stored in? */

  XColor r, g, b;

  if(dbg) fprintf(stderr,"directcolor_init\n");
  self->inited=True;

  (void)XParseColor(self->dpy,DefaultColormap(self->dpy,self->screen),
		    "rgb:ff/00/00",&r);
  (void)XParseColor(self->dpy,DefaultColormap(self->dpy,self->screen),
		    "rgb:00/ff/00",&g);
  (void)XParseColor(self->dpy,DefaultColormap(self->dpy,self->screen),
		    "rgb:00/00/ff",&b);
  (void)XAllocColor(self->dpy,DefaultColormap(self->dpy,self->screen), &r);
  (void)XAllocColor(self->dpy,DefaultColormap(self->dpy,self->screen), &g);
  (void)XAllocColor(self->dpy,DefaultColormap(self->dpy,self->screen), &b);

  self->map_info.base_pixel= 0;
  self->map_info.colormap= DefaultColormap(self->dpy,self->screen);

  if (r.pixel>g.pixel) {
    if (g.pixel>b.pixel) {
      /* RGB */
      self->map_info.blue_mult= 1;
      self->map_info.blue_max= b.pixel/(self->map_info.blue_mult);
      self->map_info.green_mult= b.pixel+1;
      self->map_info.green_max= g.pixel/(self->map_info.green_mult);
      self->map_info.red_mult= (b.pixel+1)*(g.pixel+1);
      self->map_info.red_max= r.pixel/(self->map_info.red_mult);
    }
    else if (r.pixel>b.pixel) {
      /* RBG */ 
      self->map_info.green_mult= 1;
      self->map_info.green_max= g.pixel/(self->map_info.green_mult);
      self->map_info.blue_mult= g.pixel+1;
      self->map_info.blue_max= b.pixel/(self->map_info.blue_mult);
      self->map_info.red_mult= (b.pixel+1)*(g.pixel+1);
      self->map_info.red_max= r.pixel/(self->map_info.red_mult);
    }
    else {
      /* BRG */
      self->map_info.green_mult= 1;
      self->map_info.green_max= g.pixel/(self->map_info.green_mult);
      self->map_info.red_mult= g.pixel+1;
      self->map_info.red_max= r.pixel/(self->map_info.red_mult);
      self->map_info.blue_mult= (r.pixel+1)*(g.pixel+1);
      self->map_info.blue_max= b.pixel/(self->map_info.blue_mult);
    }
  }
  else {
    if (g.pixel>b.pixel) {
      if (b.pixel>r.pixel) {
	/* GBR */
	self->map_info.red_mult= 1;
	self->map_info.red_max= r.pixel/(self->map_info.red_mult);
	self->map_info.blue_mult= (r.pixel+1);
	self->map_info.blue_max= b.pixel/(self->map_info.blue_mult);
	self->map_info.green_mult= (r.pixel+1)*(b.pixel+1);
	self->map_info.green_max= g.pixel/(self->map_info.green_mult);
      }
      else {
	/* GRB */
	self->map_info.blue_mult= 1;
	self->map_info.blue_max= b.pixel/(self->map_info.blue_mult);
	self->map_info.red_mult= (b.pixel+1);
	self->map_info.red_max= r.pixel/(self->map_info.red_mult);
	self->map_info.green_mult= (r.pixel+1)*(b.pixel+1);
	self->map_info.green_max= g.pixel/(self->map_info.green_mult);
      }
    }
    else {
      /* BGR */
      self->map_info.red_mult= 1;
      self->map_info.red_max= r.pixel/(self->map_info.red_mult);
      self->map_info.green_mult= (r.pixel+1);
      self->map_info.green_max= g.pixel/(self->map_info.green_mult);
      self->map_info.blue_mult= (r.pixel+1)*(g.pixel+1);
      self->map_info.blue_max= b.pixel/(self->map_info.blue_mult);
    }
  }
}

static void directcolor_display(XdIH *self, XImage *ximage_in) {
  
  if(dbg) fprintf(stderr,"directcolor_display\n");
}

static void pseudocolor_init(XdIH *self) {

  self->inited=True;
  
  if(dbg) fprintf(stderr,"pseudocolor_init\n");
  if(dbg && self->map_info_set) fprintf(stderr,"pseudocolor_init: map info is set already\n");
  /* Have to handle Sun X11/News and HP separately */
  if (!self->map_info_set) {
    if (server_is_sun_or_hp(self)) {
#ifdef SHARE_XWS_CMAP
      if (xws_best_rgb_stdmap) {
	self->map_info= *xws_best_rgb_stdmap; /* bitwise copy will work */
      }
      else {
	generate_sun_rgbmap(self);
	xws_best_rgb_stdmap= &self->map_info;
      }
#else
      generate_sun_rgbmap(self);
#endif
    } else {
      XStandardColormap *mymaps = XAllocStandardColormap();
      int count;
      self->map_info_owned= 1;
      if (XGetRGBColormaps(self->dpy, RootWindow(self->dpy,self->screen), &mymaps, &count,
			   XA_RGB_DEFAULT_MAP)) {
	self->map_info= *mymaps; /* bitwise copy will work */
      } 
#ifdef HPUX
#ifdef SHARE_XWS_CMAP
      else {
	if (xws_best_rgb_stdmap)
	  self->map_info= *xws_best_rgb_stdmap; /* bitwise copy will work */
	else {
	  generate_sun_rgbmap(self);
	  xws_best_rgb_stdmap= &self->map_info;
	  }
      }
#else  /* SHARE_XWS_CMAP */
#endif /* SHARE_XWS_CMAP */
#else /* HPUX */
      else {
	if (XmuLookupStandardColormap(self->dpy, self->screen,
				      XVisualIDFromVisual(self->attrib.visual),
				      self->attrib.depth, 
				      XA_RGB_DEFAULT_MAP, 
				      False, True)) {
	  if (XGetRGBColormaps(self->dpy, RootWindow(self->dpy,self->screen),
			       &mymaps, &count,
			       XA_RGB_DEFAULT_MAP)) {
	    self->map_info= *mymaps; /* bitwise copy will work */
	  } else {
	    fprintf(stderr,
		    "Can't get appropriate colormap!\n");
	    exit(-1);
	  }
	} 
	else {
	  fprintf(stderr,"Can't get appropriate colormap!\n");
	  exit(-1);
	}
      }
#endif /* HPUX */
    }
    self->map_info_set= 1;
  }
  if (self->map_info_set)
    XSetWindowColormap(self->dpy, self->win, self->map_info.colormap);
}

static void pseudocolor_display(XdIH *self, XImage *ximage_in) {
  
  if(dbg) fprintf(stderr,"pseudocolor_display\n");
}

static unsigned long calc_pixel(XdIH *self, int r, int g, int b) {

  unsigned long result= self->map_info.base_pixel
    +(((self->map_info.red_max*r)+128) >> 8) * self->map_info.red_mult
    +(((self->map_info.green_max*g)+128) >> 8) * self->map_info.green_mult
    +(((self->map_info.blue_max*b)+128) >> 8) * self->map_info.blue_mult;
  
  if(dbg_good) fprintf(stderr,"calc_pixel\n");
  return result;
}

void xdrawih_clear(void *XdrawImageHandler, int r, int g, int b) {

  XdIH *self;

  if(dbg_good) fprintf(stderr,"reg_clear\n");
  self = (XdIH *)XdrawImageHandler;
  get_window_size(self);
  
  if ((self->win_x_size != self->pixwidth) || (self->win_y_size != 
					       self->pixwidth)) {
    if (self->current_pixmap) XFreePixmap(self->dpy, self->current_pixmap);
    self->current_pixmap = XCreatePixmap(self->dpy,
					 (Drawable)self->attrib.root, 
					 self->win_x_size, self->win_y_size,
					 self->attrib.depth);
    self->pixwidth = self->win_x_size;
    self->pixheight = self->win_y_size;
  }
  if (self->attrib.depth != 1) /* if not monochrome */
    XSetForeground(self->dpy, self->gc, calc_pixel(self,r,g,b));
  else XSetForeground(self->dpy, self->gc, BlackPixel(self->dpy,self->screen));
  XFillRectangle(self->dpy, self->current_pixmap, self->gc, 0, 0,
		 self->win_x_size, self->win_y_size);
  XSetForeground(self->dpy, self->gc, WhitePixel(self->dpy, self->screen));
}

static void get_window_size(XdIH *self) {
  
  /* This routine adjusts this object's ideas about window size to match
     the current state of its window. */

  Drawable root;
  int x, y;
  unsigned int width, height, border_width, depth_dummy;

  if(dbg_good) fprintf(stderr,"get_window_size\n");
  
  if ( !XGetGeometry(self->dpy, (Drawable)self->win, &root, &x, &y, &width,
		     &height, &border_width, &depth_dummy) ) {
    fprintf(stderr,"get_window_size: fatal error!\n");
    exit(-1);
  }
  self->win_x_size= width;
  self->win_y_size= height;
}

void *xdrawih_create(Display *d_in, int s_in, Window w_in, int width, int height) {

  XdIH *self;
  unsigned long valuemask;
  XGCValues xgcv;

  if(dbg) {
    fprintf(stderr,"reg_create\n");
#ifdef LOCAL
    fprintf(stderr,"LOCAL defined\n");
#endif
#ifdef SHM
    fprintf(stderr,"SHM defined\n");
#endif
#ifdef HPUX
    fprintf(stderr,"HPUX defined\n");
#endif
#ifdef SHARE_XWS_CMAP
    fprintf(stderr,"SHARE_XWS_CMAP\n");
#endif
#ifdef SHM_ONE
    fprintf(stderr,"SHM_ONE\n");
#endif
    }
  self = (XdIH *)malloc(sizeof(XdIH));
  
  self->dpy = d_in;
  self->screen = s_in;
  self->win = w_in;
  self->current_pixmap = 0;
  self->inited = False;
  self->map_info_set = 0;
  self->map_info_owned = 0;

  valuemask= (GCForeground | GCBackground);
  self->gc= XCreateGC(self->dpy, self->win, valuemask, &xgcv);
  XSetBackground(self->dpy,self->gc,BlackPixel(self->dpy, self->screen));
  XSetForeground(self->dpy,self->gc,WhitePixel(self->dpy, self->screen));
#ifdef LOCAL
  using_local = False;
#else /* LOCAL */
#ifdef SHM
  using_shm = False;
#endif /* SHM */
#endif /* LCOAL */
  get_window_size(self);

  /* Determine the color capabilities of the device*/

  /*Since we're passed a window, use the window's visual and depth.*/
  XGetWindowAttributes(self->dpy, self->win, &(self->attrib));

  /* Check for monochrome case */
  if (self->attrib.depth == 1) {
    self->init_fun = &monochrome_init;
    self->display_fun = &monochrome_display;
    if(dbg) fprintf(stderr,"Its monochrome\n");
    return;
  }
  /* Handle all other cases */
  switch ((self->attrib.visual)->class) {
  case (int)TrueColor:
    if(dbg) fprintf(stderr,"Its trucolor\n");
  case (int)DirectColor:
    self->init_fun = &directcolor_init;
    self->display_fun= &directcolor_display;
    if(dbg) fprintf(stderr,"Its directcolor\n");
    break;
  case (int)PseudoColor:
    self->init_fun = &pseudocolor_init;
    self->display_fun= &pseudocolor_display;
    if(dbg) fprintf(stderr,"Its pseudocolor\n");
    break;
  case (int)StaticColor:
    fprintf(stderr,
    "xdrawih_create: Sorry, no color support for StaticColor visual type\n");
    exit(-1);
    break;
  case (int)GrayScale:
    fprintf(stderr,
   "xdrawih_create: Sorry, no color support for GrayScale visual type\n");
    exit(-1);
    break;
  case (int)StaticGray:
    fprintf(stderr,
   "xdrawih_create: Sorry, no color support for StaticGray visual type\n");
    exit(-1);
    break;
  }

  self->pixheight = height;
  self->pixwidth = width;
  self->current_pixmap = XCreatePixmap(self->dpy, (Drawable)self->attrib.root,
				       self->pixwidth, self->pixheight, 
				       self->attrib.depth);
  if(!self->inited) (*self->init_fun)(self);
  xdrawih_clear(self,0,0,0);

  return( (XdIH *)self );
}

void xdrawih_delete(void *XdrawImageHandler) {

  XdIH *self;

  if(dbg) fprintf(stderr,"reg_delete\n");
  self = (XdIH *)XdrawImageHandler;

#ifdef LOCAL
    if (self->using_local) {
      ;
    } else
#else /* LOCAL */
#ifdef SHM
    if (self->using_shm) {
#ifndef SHM_ONE
	XShmDetach(self->dpy, &self->shminfo);
#endif /* SHM_ONE */
#ifndef SHM_ONE	
	shmdt(self->shminfo.shmaddr);
#endif /* SHM_ONE */	
     shmctl(self->shminfo.shmid, IPC_RMID, 0);
    }
#endif /* SHM */
#endif /* LOCAL */
    if (self->current_pixmap) XFreePixmap(self->dpy, self->current_pixmap);
    self->current_pixmap = 0; 
    if (self->gc) XFreeGC(self->dpy, self->gc);
    if (self->map_info_set && self->map_info_owned) XFree(&self->map_info);
    free(self);
}

void xdrawih_ppoint(void *XdrawImageHandler, int r, int g, int b, int pnum, XPoint *points) {
  XdIH *self;
  int i;

  if(dbg_good) fprintf(stderr,"reg_ppoint\n");
  self = (XdIH *)XdrawImageHandler;

  if (self->attrib.depth != 1) /* if not monochrome */
    XSetForeground(self->dpy, self->gc, calc_pixel(self,r,g,b));
  for (i=0; i<pnum; i++) 
    XDrawPoint(self->dpy, self->current_pixmap, self->gc,
	       points[i].x, points[i].y);
}

void xdrawih_pline(void *XdrawImageHandler, int r, int g, int b, int pnum, XPoint *points) {
  XdIH *self;

  if(dbg_good) fprintf(stderr,"reg_pline\n");
  self = (XdIH *)XdrawImageHandler;
  if (self->attrib.depth != 1) /* if not monochrome */
    XSetForeground(self->dpy, self->gc, calc_pixel(self,r,g,b));
  XDrawLines(self->dpy, self->current_pixmap, self->gc, points, pnum, CoordModeOrigin);
}

void xdrawih_pgon(void *XdrawImageHandler, int r, int g, int b, int vnum, XPoint *points) {

  XdIH *self;

  if(dbg_good) fprintf(stderr,"reg_pgon\n");
  self = (XdIH *)XdrawImageHandler;
  if (self->attrib.depth != 1) /* if not monochrome */
    XSetForeground(self->dpy, self->gc, calc_pixel(self,r,g,b));
  XFillPolygon(self->dpy, self->current_pixmap, self->gc, 
	       points, vnum, Complex, CoordModeOrigin);

}

void xdrawih_winsize(void *XdrawImageHandler, int *height, int *width) {


  XdIH *self;

  if(dbg_good) fprintf(stderr,"reg_winsize\n");
  self = (XdIH *)XdrawImageHandler;
  get_window_size(self);

  *height = self->win_y_size;
  *width = self->win_x_size;
}

void xdrawih_redraw(void *XdrawImageHandler) {
  /*Heh. BitBlt is so nice...*/

  XdIH *self;

  if(dbg) fprintf(stderr,"reg_redraw\n");
  self = (XdIH *)XdrawImageHandler;
  get_window_size(self);

  if (self->inited) { 
#ifdef LOCAL
    if (self->using_local);
    else
#else
#ifdef SHM
      if (self->using_shm) {
#ifdef SHM_ONE
	XShmAttach(self->dpy, &self->shminfo);
#endif /* SHM_ONE */
#ifdef SHM_ONE
	XShmDetach(self->dpy, &self->shminfo);
#endif /* SHM_ONE */
      } else
#endif /* SHM */
#endif /* LOCAL */
	XCopyArea(self->dpy, self->current_pixmap, self->win, self->gc, 0, 0, 
		  self->win_x_size, self->win_y_size, 0, 0);
  }
  XFlush(self->dpy);
}

Window xdrawih_window(void *XdrawImageHandler) {

  XdIH *self;

  if(dbg) fprintf(stderr,"reg_window\n");
  self = (XdIH *)XdrawImageHandler;
  return self->win;
}

/* *************************************************************************************************************************************************************************************************************************************** */


static void redraw_meCB(Widget w, XmadIH *self, caddr_t call_data) { 
  
  if(dbg) fprintf(stderr,"redraw_meCB\n");
  if(dbg_cb) fprintf(stderr,"redraw_meCB is called\n");
  
  xmadih_redraw( (void *)self);
}

static void resize_meCB(Widget w, XmadIH *self, caddr_t call_data) {

  if(dbg) fprintf(stderr,"resize_meCB\n");
  if(dbg_cb) fprintf(stderr,"resize_meCB is called\n");
  
  xmadih_resize( (void *)self);
}

static void xmadih_resize(void *XmautodrawImageHandler){

  XmadIH *self;

  if(dbg) fprintf(stderr,"auto_resize\n");
  self = (XmadIH *)XmautodrawImageHandler;

  if (check(self)) {
    if (! watch)
      watch=XCreateFontCursor(XtDisplay(self->draw_parent), XC_watch);
    XDefineCursor(XtDisplay(self->draw_parent), XtWindow(self->draw_parent),
		  watch);
    xmadih_resize_IH(self);
    XUndefineCursor(XtDisplay(self->draw_parent), XtWindow(self->draw_parent));
  }
}

static void xmadih_resize_IH(XmadIH *self) {

  unsigned int old_win_x_size = self->the_image->win_x_size;
  unsigned int old_win_y_size = self->the_image->win_y_size;

  if(dbg) fprintf(stderr,"auto_resize_IH\n");
  get_window_size(self->the_image);

  if (self->the_image->inited)
    if ((self->the_image->win_x_size > old_win_x_size) || (self->the_image->win_y_size > old_win_y_size)) {
      xmadih_redraw(self);
    }
}

void xmadih_redraw(void *XmautodrawImageHandler) {

  XmadIH *self;
  
  if(dbg) fprintf(stderr,"auto_redraw\n");
  self = (XmadIH *)XmautodrawImageHandler;

  if (self->does_top && !self->redrawn_before) {
    Window canvas_win = XtWindow(self->draw_me);
    XChangeProperty(XtDisplay(self->draw_me), canvas_win, 
		    self->atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		    (unsigned char *)&canvas_win, 1 );
    self->redrawn_before = 0;
  }
  if (check(self)) {
    if (! watch)
      watch=XCreateFontCursor(XtDisplay(self->draw_parent), XC_watch);
    XDefineCursor(XtDisplay(self->draw_parent), XtWindow(self->draw_parent), 
		  watch);
    xdrawih_redraw(self->the_image);
    XUndefineCursor(XtDisplay(self->draw_parent), XtWindow(self->draw_parent));
  }
}

static Boolean check(XmadIH *self) {

  if(dbg_good) fprintf(stderr,"check\n");
  if (self->the_image != NULL) /*Is it created?*/
    return(True);

  if (XtIsRealized(self->draw_me)) {
    XSync(XtDisplay(self->draw_me), 0);
    self->the_image = xdrawih_create(XtDisplay(self->draw_me),0,
				      XtWindow(self->draw_me),
				      self->pixheight,self->pixwidth);
    if (self->the_image) return(True);
    else return(False);
  }

  return(False); /* draw_me wasn't realized - couldn't create the_image */
}

void* xmadih_create(Widget parent, int height, int width){
  

  Arg args[6],*top_args = NULL;
  register int n;
  XmadIH *self;
  int top_n = 0;
  Widget base_parent = NULL;

  if(dbg) fprintf(stderr,"auto_create\n");
  self = (XmadIH *)malloc(sizeof(XmadIH));

  if (self->does_top = !base_parent) {
    Widget temp;

    self->draw_parent=XmCreateFormDialog(parent, "xmautoimagehandlerW",
				   top_args, top_n);
    n=0;
    XtSetArg(args[n], XtNwidth, width); n++;
    XtSetArg(args[n], XtNheight, height); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    self->draw_me= XmCreateDrawingArea(self->draw_parent, "xmautoimagehandler",
				 args, n);
    while (temp = XtParent(parent)) parent = temp;
    self->redrawn_before = 0;
    self->atomcolormapwindows= 
      XInternAtom(XtDisplay(parent), "WM_COLORMAP_WINDOWS", False);
  } 
  else {
    self->draw_parent=base_parent;
    self->draw_me= XmCreateDrawingArea(self->draw_parent, "xmautoimagehandler",
				 top_args, top_n);
  }
  XtManageChild(self->draw_me);
  XtAddCallback(self->draw_me, XmNresizeCallback,
		(XtCallbackProc) resize_meCB,
		(XtPointer) self);
  XtAddCallback(self->draw_me, XmNexposeCallback,
		(XtCallbackProc) redraw_meCB,
		(XtPointer) self);
  if (self->does_top)
    XtManageChild(self->draw_parent);
  self->the_image = NULL;
  self->pixheight = height;
  self->pixwidth = width;

  return( (XmadIH *)self );
}

void xmadih_delete(void *XmautodrawImageHandler){

  XmadIH *self;
  
  if(dbg) fprintf(stderr,"auto_delete\n");
  self = (XmadIH *)XmautodrawImageHandler;
  
  if (self->does_top)
    XtDestroyWidget(self->draw_parent);
  else
    XtDestroyWidget(self->draw_me);
  
  if(self->the_image) xdrawih_delete(self->the_image);

  free(self);
}

void xmadih_ppoint(void *XmautodrawImageHandler,
			      int r, int g, int b, int pnum, XPoint *points){

  XmadIH *self;
  
  if(dbg_good) fprintf(stderr,"auto_ppoint\n");
  self = (XmadIH *)XmautodrawImageHandler;
  if(check(self)) {
    xdrawih_ppoint(self->the_image,r,g,b,pnum,points);
  }

}
void xmadih_pline(void *XmautodrawImageHandler,
			     int r, int g, int b, int pnum, XPoint *points){

  XmadIH *self;

  if(dbg_good) fprintf(stderr,"auto_pline\n");
  self = (XmadIH *)XmautodrawImageHandler;

  if(check(self)) {
    xdrawih_pline(self->the_image,r,g,b,pnum,points);
  }
}
void xmadih_pgon(void *XmautodrawImageHandler,
			    int r, int g, int b, int pnum, XPoint *points){

  XmadIH *self;
  
  if(dbg_good) fprintf(stderr,"auto_pgon\n");
  self = (XmadIH *)XmautodrawImageHandler;

  if(check(self)) {
    xdrawih_pgon(self->the_image,r,g,b,pnum,points);
  }
}
void xmadih_winsize(void *XmautodrawImageHandler, 
			       int *height, int *width){

  XmadIH *self;
  
  if(dbg) fprintf(stderr,"auto_winsize\n");
  self = (XmadIH *)XmautodrawImageHandler;

  if(check(self)) {
    xdrawih_winsize(self->the_image,height,width);
  }
}

void xmadih_clear(void *XmautodrawImageHandler, 
			     int r, int g, int b){

  XmadIH *self;
  
  if(dbg) fprintf(stderr,"auto_clear\n");
  self = (XmadIH *)XmautodrawImageHandler;

  if(check(self)) {
    xdrawih_clear(self->the_image,r,g,b);
  }
}
Widget xmadih_widget( void *XmautodrawImageHandler ){

  XmadIH *self;
  
  if(dbg) fprintf(stderr,"auto_widget\n");
  self = (XmadIH *)(XmautodrawImageHandler);
  return( (Widget)self->draw_me );
}
Window xmadih_window( void *XmautodrawImageHandler ){

  XmadIH *self;
  
  if(dbg) fprintf(stderr,"auto_window\n");
  self = (XmadIH *)XmautodrawImageHandler;

  if(check(self)) {
    return( xdrawih_window(self->the_image) );
  }
  else return 0;
}


