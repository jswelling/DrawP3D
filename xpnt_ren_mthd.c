/****************************************************************************
 * xpainter.c
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Joe Demers, based upon code by Chris Nuuja
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
/* This module is the core of the XPainter renderer */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmu/StdCmap.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>

#ifndef LOCAL
#ifdef SHM
#include <sys/types.h>
#include <sys/ipc.h>
/*#include <sys/shm.h>*/
#include <X11/extensions/XShm.h>
#endif /* SHM */
#endif /* LOCAL */

#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "gen_paintr_strct.h"
#include "gen_painter.h"
#include "xdrawih.h"

#define MIN(x, y) ((x<y)?x:y)

/* commands to handle_alarm() */
#define BEGIN  1
#define IGNORE 2
#define NOTICE 3
#define END    4

#define DEFAULT_WIDTH 500
#define DEFAULT_HEIGHT 500
#define ALARM_VAL 100000
#define ALARM_OFF 0

/* 
The following is designed to figure out what machine this is running
on, and include appropriate definition files.
*/
#ifdef unix
#define USE_UNIX
#endif
#ifdef CRAY
#undef USE_UNIX
#endif
#ifdef ardent
#undef USE_UNIX
#endif
#ifdef CRAY
#include "unicos_defs.h"
#endif
#ifdef ardent
#include "unicos_defs.h"
#endif
#ifdef USE_UNIX
#include "unix_defs.h"
#endif
	
/* In case we need to create an Xt environment... */
static Widget toplevel=0, main_window= 0;
#ifdef never
static Atom atomcolormapwindows;
#endif

/* The following structures are used across all instances of the renderer.
 * They are buffers for storing coordinates in the process of being
 * transformed from world to screen space and clipped.
 */
extern int TempCoordBuffSz;

/* number of automanagers there are */
static int using_alarm = 0;
static int ignoring_alarm = 0;

/* number of renderers awaiting X event handling to finish creation */
static int rens_being_created= 0;

/* number of renderers awaiting X event handling to finish being destroyed */
static int rens_being_destroyed= 0;

/* Space for default color map */
static P_Renderer_Cmap default_map;

/* default color map function */
static void default_mapfun(float *val, float *r, float *g, float *b, float *a)
/* This routine provides a simple map from values to colors within the
 * range 0.0 to 1.0.
 */
{
  /* No debugging; called too often */
  *r= *g= *b= *a;
}

/*
  Global Data Used: NONE
  Expl: start a repeating timer interrupt every ms milliseconds
        turn off timer if ms is zero
*/
static void set_alarm(unsigned long ms)
{
  static struct itimerval new, old;

  if (ms) {
    new.it_value.tv_sec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = ms;
    new.it_interval.tv_usec = ms;
#ifndef CRAY_C90
#ifndef DEC_PMAX
    if (setitimer(ITIMER_REAL, &new, &old)) {
      perror("Error calling setitimer");
      abort();
    }
#endif
#endif
    return;
  } else {
    new.it_value.tv_sec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = 0;
    new.it_interval.tv_usec = 0;
#ifndef CRAY_C90
#ifndef DEC_PMAX
    if (setitimer(ITIMER_REAL, &new, &old)) {
      perror("Error calling setitimer for reset");
      abort();
    }
#endif
#endif
    return;
  }
}

/*
  Global Data Used: NONE
  Expl: processes all XEvents in the queue
*/
static void MyXEventLoop()
{

  while (XtPending()) {
    XEvent event;
    XtNextEvent(&event);
    XtDispatchEvent(&event);
  }
}

/*
  Global Data Used: NONE
  Expl: processes XEvents until the queue is empty and 
        all pending renderer creations are complete
*/
static void CreationXEventLoop()
{
  while (XtPending() || rens_being_created) {
    XEvent event;
    XtNextEvent(&event);
    XtDispatchEvent(&event);
  }
}

/*
  Global Data Used: NONE
  Expl: processes XEvents until the queue is empty and all
        pending renderer destructions are complete
*/
static void DestructionXEventLoop()
{
  while (XtPending() || rens_being_destroyed) {
    XEvent event;
    XtNextEvent(&event);
    XtDispatchEvent(&event);
  }
}

static void do_nothing()
{
}


/*
  Global Data Used: NONE
  Expl: sets signal handler to one of:
        ignore interrupt, run event loop, or restore old handler
*/
static void handle_alarm(int command)
{
  static struct sigaction ignore, act, oact;

  switch (command) {
  case BEGIN: 
    ignore.sa_handler = do_nothing;
#ifndef CRAY_C90
    ignore.sa_handler = SIG_IGN;
#endif
    sigemptyset(&ignore.sa_mask);
    sigaddset(&ignore.sa_mask, SIGALRM);
    ignore.sa_flags = 0;

    act.sa_handler = MyXEventLoop;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGALRM);
    act.sa_flags = 0;

#ifndef DEC_PMAX
#ifndef CRAY_C90
ignore.sa_flags = SA_RESTART;
act.sa_flags = SA_RESTART;
#endif
#endif

    if (sigaction(SIGALRM, &act, &oact) != 0) {
      fprintf(stderr, "Error calling sigaction\n");
      abort();
    }
    return;

  case IGNORE: 
#ifdef never
    if (sigaction(SIGALRM, &ignore, &act) != 0) {
#endif
    if (sigaction(SIGALRM, &ignore, NULL) != 0) {
      fprintf(stderr, "Error calling sigaction\n");
      abort();
    }
    return;

  case NOTICE: 
#ifdef never
    if (sigaction(SIGALRM, &act, &ignore) != 0) {
#endif
    if (sigaction(SIGALRM, &act, NULL) != 0) {
      fprintf(stderr, "Error calling sigaction\n");
      abort();
    }
    return;

  case END: 
#ifdef never
    if (sigaction(SIGALRM, &oact, &act) != 0) {
#endif
    if (sigaction(SIGALRM, &oact, NULL) != 0) {
      fprintf(stderr, "Error calling sigaction\n");
      abort();
    }
    return;

  default:
    fprintf(stderr, "handle_alarm called with unknown command: %d\n", command);
    return;
  }
}

/*
   Global Data Used: NONE (The wire-frame model switch will go here so that
		           the interior style will be hollow)

   Expl:  Draws a polygon,polyline, or polymarker with it's recorded color 
*/
static void xpnt_render_polyrec(P_Renderer *self, int depth_index)
{
  int i,error=0,poly_index,numcoords,x_index,y_index,color;
  int red,green,blue;
  XPoint *coords;

  poly_index = DEPTHBUFFER(self)[depth_index].poly;
  
  x_index = DPOLYBUFFER(self)[poly_index].x_index;
  y_index = DPOLYBUFFER(self)[poly_index].y_index;
  numcoords = DPOLYBUFFER(self)[poly_index].numcoords;
  
  color = DPOLYBUFFER(self)[poly_index].color;
  red   = (int)(0.5 + 255 * DCOLORBUFFER(self)[color].r);
  green = (int)(0.5 + 255 * DCOLORBUFFER(self)[color].g);
  blue  = (int)(0.5 + 255 * DCOLORBUFFER(self)[color].b);
  
  coords = (XPoint *) calloc(numcoords, sizeof(XPoint));
  if (!coords)
    ger_error("ERROR: in xpnt_render_polyrec: couldn't alloc %d coordinates\n",
	      numcoords);

  for (i=0;i<numcoords;i++) {
    coords[i].x = XOFFSET(self) 
      + (int)((MIN(WIDTH(self), HEIGHT(self)) * 0.5)
	      * (*(DCOORDBUFFER(self) + x_index + i)) + 0.5);
    coords[i].y = HEIGHT(self) 
      - YOFFSET(self) - (int)((MIN(WIDTH(self), HEIGHT(self)) * 0.5)
			      * (*(DCOORDBUFFER(self)+y_index+i)) +0.5);
  }
  switch(DPOLYBUFFER(self)[poly_index].type)
    {
    case POLYGON:
      if (XMAUTO(self))
	xmadih_pgon(IHANDLER(self), red, green, blue, 
		    numcoords, (XPoint *)coords);
      else 
	xdrawih_pgon(IHANDLER(self), red, green, blue, 
		     numcoords, (XPoint *)coords);
      break;
    case POLYLINE:
      if (XMAUTO(self))
	xmadih_pline(IHANDLER(self), red, green, blue, 
		     numcoords, (XPoint *)coords);
      else
	xdrawih_pline(IHANDLER(self), red, green, blue, 
		      numcoords, (XPoint *)coords);
      break;
    case POLYMARKER:
      if (XMAUTO(self))
	xmadih_ppoint(IHANDLER(self), red, green, blue, 
		      numcoords, (XPoint *)coords);
      else
	xdrawih_ppoint(IHANDLER(self), red, green, blue, 
		       numcoords, (XPoint *)coords);
      break;
    default:
      ger_error("ERROR: Unknown primitive:%d\n",error);
    }
  free(coords);
}

static void new_widget_cb( Widget w, P_Renderer *self, XtPointer call_data )
{
  if (using_alarm) handle_alarm(IGNORE);
  if ( !(RENDATA(self)->initialized) ) {
    Window canvas_win;
    rens_being_created--;

    if (!XMAUTO(self)) 
      IHANDLER(self)= xdrawih_create(XtDisplay(w), 0,
				     XtWindow(w), 300, 300);

#ifdef never
    canvas_win= XtWindow( w );
    XChangeProperty(XtDisplay(toplevel), XtWindow(toplevel), 
		    atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		    (unsigned char *)&canvas_win, 1 );
#endif
    RENDATA(self)->initialized= 1;
  }
  if (using_alarm) handle_alarm(NOTICE);
}

static void kill_widget_cb( Widget w, P_Renderer *self, XtPointer call_data )
{
  if (using_alarm) handle_alarm(IGNORE);
  rens_being_destroyed--;
  if (using_alarm) handle_alarm(NOTICE);
}

/*
   Global Data Used: LightLocation,LightIntensity,AmbientIntensity,
		     ViewMatrix, Zmin, Zmax,
		     pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer,
		     xmauto, automanage, widget, ihandler, height, width

   Expl:  initializes all global variables, and creates an image handler
           Zmin, Zmax, and EyeMatrix will all be set to appropriate values
	   in ren_camera (located in painter_ren.c).  Light characteristics are
	   set, but are not (currently) used.
*/
static void xpnt_init_renderer(P_Renderer *self, char *device, char *data)
{
  int i;
  
  /*  
    ViewMatrix is the matrix that translates  coordinates from
    their position in the world to their position relative to the
    Eye/Camera.  EyeMatrix hold projection information.
    */
  VIEWMATRIX(self) = (float *) malloc(16*sizeof(float));
  if (!(VIEWMATRIX(self)))
    ger_fatal("xpainter: pnt_init_renderer: couldn't allocate 16 floats!");
  for (i=0;i<16;i++) VIEWMATRIX(self)[i]=0.0;
  VIEWMATRIX(self)[0] = 1.0;
  VIEWMATRIX(self)[5] = 1.0;
  VIEWMATRIX(self)[10] = 1.0;
  VIEWMATRIX(self)[15] = 1.0;
  
  EYEMATRIX(self) = (float *) malloc(16*sizeof(float));
  if (!(EYEMATRIX(self)))
    ger_fatal("xpainter: pnt_init_renderer: couldn't allocate 16 floats!");
  for (i=0;i<16;i++) EYEMATRIX(self)[i]=0.0;
  EYEMATRIX(self)[0] = 1.0;
  EYEMATRIX(self)[5] = 1.0;
  EYEMATRIX(self)[10] = 1.0;
  EYEMATRIX(self)[15] = 1.0;
  
  /* Set (default) Z clipping boundries,  Zmax is hither, Zmin is yon */
  ZMAX(self) = -0.01;
  ZMIN(self) = -900.0;
  
  /*
    These buffers hold coordinates of polygons, polylines, markers, etc
    as they are transformed from World coordinates to screen coordinates
    The pnt_Xclip_buffer,pnt_Zclip_buffer,pnt_Yclip_buffer are used to
    hold coordinates between clippings against the hither and
    yon planes (Zmin and Zmax).
    */
  pnt_Xcoord_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
  pnt_Ycoord_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
  pnt_Zcoord_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
  pnt_Xclip_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
  pnt_Yclip_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
  pnt_Zclip_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
  if ( !pnt_Xcoord_buffer || !pnt_Ycoord_buffer || !pnt_Zcoord_buffer
      || !pnt_Xclip_buffer || !pnt_Yclip_buffer || !pnt_Zclip_buffer )
    ger_fatal("xpainter: pnt_init_renderer: memory allocation failed!\n");

  if (data) {
    char *ptr;
    char* datadup= strdup(data);
    if (ptr = strchr(datadup, 'x')) {
      *(ptr) = '\0';
      WIDTH(self) = atoi(datadup);
      HEIGHT(self) = atoi(ptr+1);
    } else {
      WIDTH(self) = DEFAULT_WIDTH;
      HEIGHT(self) = DEFAULT_HEIGHT;
    }
    free(datadup);
  }
  else {
    WIDTH(self) = DEFAULT_WIDTH;
    HEIGHT(self) = DEFAULT_HEIGHT;
  }

  /* automanage uses timer signal and MyXEventLoop, and xmautodrawimagehandler
     manual uses xmautodrawimaeghandler
     widget=xxxx uses xdrawimagehandler
     */

  if (!strstr(device, "widget=")) {
    XMAUTO(self)= 1;
    if (strcmp(device,"manual")) AUTOMANAGE(self)= 1;
    else AUTOMANAGE(self)= 0;
  }
  else {
    AUTOMANAGE(self)= 0;
    XMAUTO(self)= 0;
  }

  /* Protect X from stepping on itself */
  if (using_alarm) handle_alarm(IGNORE);

  if (XMAUTO(self)) {

    /* Need to potentially create an entire Xt environment */
    if (!(toplevel)) { /* if Xt not initialized, then do so */
      Arg args[16];

      i = 0;
      toplevel = XtInitialize("p3d", "xpainter", NULL, 0, &i, NULL);
      i = 0;
      main_window = XmCreateMainWindow(toplevel, "main", args, i);

      XtManageChild(main_window);
      
      XtRealizeWidget(toplevel);

#ifdef never
      atomcolormapwindows = 
	XInternAtom(XtDisplay(toplevel), "WM_COLORMAP_WINDOWS", False);
#endif
    }
    rens_being_created++;
    IHANDLER(self) = (void*)xmadih_create(toplevel, HEIGHT(self), WIDTH(self));
    WIDGET(self)= xmadih_widget( IHANDLER(self) );
    XtAddCallback( WIDGET(self), XmNexposeCallback, 
		  (XtCallbackProc)new_widget_cb, (XtPointer)self );
  }
  else {
    /* 'widget=' only  -  create xdrawimagehandler */
    char* ptr = strchr(device, '=');
    WIDGET(self) = (Widget)atol(ptr+1);
    if (XtIsRealized(WIDGET(self))) { /* App already created widget and win */
      Window canvas_win;
      
      IHANDLER(self)= xdrawih_create(XtDisplay(WIDGET(self)), 0,
				     XtWindow(WIDGET(self)), 300, 300);
      
#ifdef never
      canvas_win= XtWindow( WIDGET(self) );
      XChangeProperty(XtDisplay(toplevel), XtWindow(toplevel), 
		      atomcolormapwindows, XA_WINDOW, 32, PropModeAppend, 
		      (unsigned char *)&canvas_win, 1 );
#endif
      RENDATA(self)->initialized= 1;
      
    }
    else { /* creation of widget still pending */
      rens_being_created++; /* callback calls xdrawih_create */
      XtAddCallback( WIDGET(self), XmNexposeCallback, 
		    (XtCallbackProc)new_widget_cb, (XtPointer)self );
    }
  }
  XtAddCallback( WIDGET(self), XmNdestroyCallback, 
		(XtCallbackProc)kill_widget_cb, (XtPointer)self );
  
  if (XMAUTO(self)) {
    XFlush(XtDisplay(toplevel));
    
    CreationXEventLoop();
  }

  /* Start handling alarm events again */
  if (using_alarm) handle_alarm(NOTICE);

  if (AUTOMANAGE(self) && (using_alarm++ == 0)) {
    handle_alarm(BEGIN);
    set_alarm(ALARM_VAL);
  }
}

/*
   Global Data Used: xmauto, automanage, ihandler
   Expl:  shutdown handler.
*/
void xpnt_shutdown_renderer(P_Renderer *self)
{
  Widget kill_this_widget;
  if (AUTOMANAGE(self) && (--using_alarm == 0)) {
    set_alarm(ALARM_OFF);
    handle_alarm(END);
  }
  if (using_alarm) handle_alarm(IGNORE);
  kill_this_widget= WIDGET(self);
  if (XMAUTO(self)) {
    xmadih_delete(IHANDLER(self));
    rens_being_destroyed++;
    XSync(XtDisplay(kill_this_widget), False);
    DestructionXEventLoop();
  }
  else {
    xdrawih_delete(IHANDLER(self));
    rens_being_destroyed++;
  }
  if (using_alarm) handle_alarm(NOTICE);
}

/*
  Global Data Used: xmauto, automanage, ihandler, background, 
                    yoffset, xoffset, height, width
  Expl: ignore alarm if running, clear window, and center picture in window
*/
static void before(P_Renderer *self)
{
  if ((using_alarm) && (ignoring_alarm++ == 0)) handle_alarm(IGNORE);
  if (XMAUTO(self)) {
    xmadih_winsize(IHANDLER(self), &HEIGHT(self), &WIDTH(self));
    xmadih_clear(IHANDLER(self), (int)(255*BACKGROUND(self).r),
		 (int)(255*BACKGROUND(self).g), (int)(255*BACKGROUND(self).b));
  } else {
    xdrawih_winsize(IHANDLER(self), &HEIGHT(self), &WIDTH(self));
    xdrawih_clear(IHANDLER(self), 
		  (int)(255*BACKGROUND(self).r), 
		  (int)(255*BACKGROUND(self).g), 
		  (int)(255*BACKGROUND(self).b));
  }
  XOFFSET(self) = HEIGHT(self) < WIDTH(self) ? 
    (WIDTH(self) - HEIGHT(self)) / 2 : 0;
  YOFFSET(self) = HEIGHT(self) > WIDTH(self) ?
    (HEIGHT(self) - WIDTH(self)) / 2 : 0;
}

/*
  Global Data Used: xmauto, automanage, ihandler
  Expl: redraw window, turn alarm back on if needed
*/
static void after(P_Renderer *self)
{
  if (XMAUTO(self)) {
    xmadih_redraw(IHANDLER(self));
    XSync(XtDisplay(WIDGET(self)), False);
    if (using_alarm && (--ignoring_alarm == 0))
      handle_alarm(NOTICE);
  } else {
    xdrawih_redraw(IHANDLER(self));
    XSync(XtDisplay(WIDGET(self)), False);
  }
}

/*
  Global Data Used: depthcount, maxdepthpoly
  Expl: draw the scene, from back to front
*/
void xdraw_DepthBuffer(P_Renderer *self)
{
  register int i;

  ger_debug("xpnt_ren_mthd: xdraw_DepthBuffer");

  before(self);

  if (DEPTHCOUNT(self) >= MAXDEPTHPOLY(self))
    ger_fatal("ERROR, was about to overflow!");
  for (i=0;i<DEPTHCOUNT(self);i++)
    xpnt_render_polyrec(self,i);
  DEPTHCOUNT(self)=0;			/* reset DepthBuffer */

  after(self);
}

/*
  Global Data Used: a lot
  Expl: malloc memory and initialize global variables
        initialize renderer, setup buffers, and fill methods
*/
P_Renderer *po_create_xpainter_renderer( char *device, char *datastr )
/* This routine creates a ptr-generating renderer object */
{
  P_Renderer *self;
  P_Renderer_data *rdata;
  static int sequence_number= 0;
  int length,i;

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_xpainter_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_xpainter_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;

  /* Fill out default color map */
  strcpy(default_map.name,"default-map");
  default_map.min= 0.0;
  default_map.max= 1.0;
  default_map.mapfun= default_mapfun;

  /* Fill out public and private object data */
  sprintf(self->name,"xpainter%d",sequence_number++);
  rdata->open= 0;  /* renderer created in closed state */
  CUR_MAP(self)= &default_map;

  ASSIST(self)= po_create_assist(self);

  BACKCULLSYMBOL(self)= create_symbol("backcull");
  TEXTHEIGHTSYMBOL(self)= create_symbol("text-height");
  COLORSYMBOL(self)= create_symbol("color");
  MATERIALSYMBOL(self)= create_symbol("material");

  length = 1 + strlen(device);
  MANAGER(self) = malloc(length);
  strcpy(MANAGER(self),device);

  length = 1 + strlen(datastr);
  GEOMETRY(self) = malloc(length);
  strcpy(GEOMETRY(self),datastr);

  XPDATA(self) = (XP_data *)malloc(sizeof(XP_data));

  DCOORDINDEX(self)=0;
  MAXDCOORDINDEX(self) = INITIAL_MAX_DCOORD;
  DCOORDBUFFER(self) = (float *) malloc(MAXDCOORDINDEX(self)*sizeof(float));
  
  DCOLORCOUNT(self) = 0;
  MAXDCOLORCOUNT(self) = INITIAL_MAX_DCOLOR;
  DCOLORBUFFER(self) = (Pnt_Colortype *)
    malloc( MAXDCOLORCOUNT(self)*sizeof(Pnt_Colortype) );

  if ( !DCOORDBUFFER(self) || !DCOLORBUFFER(self) )
    ger_fatal("po_create_xpainter_renderer: memory allocation failed!");

  MAXPOLYCOUNT(self) = INITIAL_MAX_DEPTHPOLY;
  MAXDEPTHPOLY(self) = MAXPOLYCOUNT(self);
  
  MAXDLIGHTCOUNT(self)= INITIAL_MAX_DLIGHTS;

  xpnt_init_renderer(self, device, datastr);
  
  setup_Buffers(self);

  RENDATA(self)->initialized= 0;

  return fill_methods(self);
}
