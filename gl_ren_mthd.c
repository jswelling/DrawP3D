/****************************************************************************
 * gl_ren_mthd.c
 * Author Robert Earhart
 * Copyright 1992, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
/*
This module provides renderer methods for the IRIS gl renderer
*/

/* Notes-
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#ifdef USE_OPENGL
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#else
#include <gl/glws.h>
#include <gl/gl.h>
#include <X11/Xirisw/GlxMDraw.h>
#endif
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"

#ifndef USE_OPENGL
#define Object GL_Object
#include <gl/sphere.h>
#undef Object
#endif

#include "indent.h"
#include "gl_strct.h"

#ifdef WIREGL
#include "wiregl_papi.h"
#endif

/*
 * Mesh stripping control.  If STRIP_MESHES is non-zero, an attempt will
 * be made to convert meshes consisting entirely of triangle strips to
 * meshes.  The search for strip "neighbors" will be STRIP_PATIENCE
 * triangles wide, and the stripped mesh will be used rather than
 * the original mesh if the number of strips (including length 1) is
 * STRIP_BENEFIT * (original triangle count) or less.
 *
 * Stripping is time consuming, but it can greatly improve rendering speed.
 */
#ifndef STRIP_MESHES
#define STRIP_MESHES 1
#endif
#define STRIP_PATIENCE 10000
#define STRIP_BENEFIT 0.5

/* Used only with Chromium, but it's easier to define it generally */
#define BARRIER_BASE 100


#ifdef CHROMIUM

#include "chromium.h"
#define LOAD( x ) gl##x##CR = (cr##x##Proc) glXGetProcAddressARB( "cr"#x )
#define LOAD2( x ) gl##x##CR = (gl##x##CRProc) glXGetProcAddressARB( "gl"#x )

/* If we have the good fortune to be running under Chromium, we'll
 * want these...
 */

static crCreateContextProc  glCreateContextCR  = NULL;
static crMakeCurrentProc    glMakeCurrentCR    = NULL;
static crSwapBuffersProc    glSwapBuffersCR    = NULL;
static crDestroyContextProc glDestroyContextCR = NULL;
static glBarrierCreateCRProc  glBarrierCreateCR  = NULL;
static glBarrierExecCRProc    glBarrierExecCR    = NULL;
static glBarrierDestroyCRProc glBarrierDestroyCR = NULL;

#else
typedef GLuint (*crCreateContextProc)( GLint window, GLint ctx );
typedef void (*crMakeCurrentProc)( GLint window, GLint ctx );
typedef void (*crSwapBuffersProc)( GLint window, GLint ctx );
typedef void (*crDestroyContextProc)( GLint ctx );
#define LOAD( x ) gl##x##CR = (cr##x##Proc)glXGetProcAddressARB( "cr"#x )
#define LOAD2( x ) gl##x##CR = glXGetProcAddressARB( "gl"#x )
#ifdef never
static GLuint (*glCreateContextCR)( GLint, GLint )        = NULL;
static void (*glMakeCurrentCR)( GLint window, GLint ctx ) = NULL;
static void (*glSwapBuffersCR)( GLint i, GLint j )        = NULL;
static void (*glDestroyContextCR)( GLint ctx )            = NULL;
#endif
static crCreateContextProc  glCreateContextCR             = NULL;
static crMakeCurrentProc    glMakeCurrentCR               = NULL;
static crSwapBuffersProc    glSwapBuffersCR               = NULL;
static crDestroyContextProc glDestroyContextCR            = NULL;
static void (*glBarrierCreateCR)( GLuint i, GLuint j)     = NULL;
static void (*glBarrierExecCR)( GLuint i )                = NULL;
static void (*glBarrierDestroyCR)( GLuint i )             = NULL;
#define CR_RGB_BIT            0x1
#define CR_DEPTH_BIT          0x4
#define CR_DOUBLE_BIT         0x20

#endif

/* And if we have been deprived of Chromium, we may want these... */

static void dummyMakeCurrent( GLint window, GLint ctx )
{ ger_debug("crMakeCurrentCR is not loaded!\n"); }

static void dummySwapBuffers( GLint i, GLint j )
{ ger_debug("crSwapBuffersCR is not loaded!\n"); }

static void dummyDestroyContext( GLint ctx )
{ ger_debug("crDestroyContextCR is not loaded!\n"); }

static void dummyBarrierCreate( GLuint i, GLuint j )
{ ger_debug("glBarrierCreateCR is not loaded!\n"); }

static void dummyBarrierExec( GLuint i )
{ ger_debug("glBarrierExecCR is not loaded!\n"); }

static void dummyBarrierDestroy( GLuint i )
{ ger_debug("glBarrierDestroyCR is not loaded!\n"); }

#define GLPROF(x)

#define BLACKPATTERN 0
#define GREYPATTERN 1

#ifdef USE_OPENGL
#define SPHERE_SLICES 10
#define SPHERE_STACKS 10
#define CYLINDER_SLICES 20
#define CYLINDER_STACKS 1
#endif

#define N_MATERIALS 6
#define N_ENTRIES 15
static gl_material materials[N_MATERIALS]= {
  { .5, .5, .5, 30.0, 0 }, /* default_material */
  { .9, .9, .1, 5.0, 0 },  /* dull_material */
  { .5, .5, .5, 50.0, 0 }, /* shiny_material */
  { .1, .1, .9, 100.0, 0 },/* metallic_material */
  { 1.0, 1.0, 0.0, 0.0, 0 }, /* matte_material */
  { 0.25, 0.25, 0.75, 6.0, 0 } /* aluminum_material */
};

#ifdef USE_OPENGL

/* A struct to hold lighting info */
typedef struct pgl_light_struct {
  GLenum light_num;
  GLfloat cvals[4];
  GLfloat xvals[4];
} PGL_Light;

#define DEFAULT_WINDOW_GEOMETRY "300x300"

static GLubyte greyPattern[128]= 
{
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA,
  0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA
};

/* Used in defining tori */
#define rad .7071067811865 /*  square root of 2 all divided by 2  */
static GLfloat knots[12] = {0.,0.,0.,1.,1.,2.,2.,3.,3.,4.,4.,4.};
static GLfloat bx[9] = {1., rad, 0.,-rad, -1., -rad, 0., rad, 1.};
static GLfloat by[9] = {0., rad, 1.,rad, 0., -rad, -1., -rad, 0.};
static GLfloat  w[9] = {1., rad, 1., rad,  1.,  rad, 1., rad, 1.};
#undef rad


#else
static unsigned short greyPattern[16] =
{
  0x5555,
  0xAAAA,
  0x5555,
  0xAAAA,
  0x5555,
  0xAAAA,
  0x5555,
  0xAAAA,
  0x5555,
  0xAAAA,
  0x5555,
  0xAAAA,
  0x5555,
  0xAAAA,
  0x5555,
  0xAAAA
};

static short light_num = 3;

static float initiallm[]={
  AMBIENT, .6, .6, .6,
  LOCALVIEWER, 0.0,
  TWOSIDE, 0.0,
  LMNULL};    

/* Used in defining tori */
#define rad .7071067811865 /*  square root of 2 all divided by 2  */
static double knots[12] = {0.,0.,0.,1.,1.,2.,2.,3.,3.,4.,4.,4.};
static double bx[9] = {1., rad, 0.,-rad, -1., -rad, 0., rad, 1.};
static double by[9] = {0., rad, 1.,rad, 0., -rad, -1., -rad, 0.};
static double  w[9] = {1., rad, 1., rad,  1.,  rad, 1., rad, 1.};
#undef rad

/*So we don't have to realloc these EVERY TIME (primitive caching)*/
static gl_gob *my_cylinder = NULL, *my_sphere = NULL;

#endif

static short hastransparent;

static int ren_seq_num= 0; /* counts gl renderers */

/*Definitions for cylinder nurbs*/

#define CYL_NUMKNOTSX	4
#define CYL_NUMKNOTSY	12
#define CYL_NUMCOORDS	3
#define CYL_ORDERX      2
#define CYL_ORDERY      6
#define CYL_NUMPOINTSX	(CYL_NUMKNOTSX - CYL_ORDERX)
#define CYL_NUMPOINTSY	(CYL_NUMKNOTSY - CYL_ORDERY)

/* function prototypes */
static void init_gl_structure (P_Renderer *self); 
static void init_gl_gl (P_Renderer *self); 
static void ren_print();

static void init_chromium_hooks()
{
#ifdef USE_OPENGL
    if (!glBarrierCreateCR) LOAD2( BarrierCreate );
    if (!glBarrierCreateCR) glBarrierCreateCR= dummyBarrierCreate;
    if (!glBarrierExecCR) LOAD2( BarrierExec );
    if (!glBarrierExecCR) glBarrierExecCR= dummyBarrierExec;
    if (!glBarrierDestroyCR) LOAD2( BarrierDestroy );
    if (!glBarrierDestroyCR) glBarrierDestroyCR= dummyBarrierDestroy;
#endif /* USE_OPENGL */
}

static int chromium_in_use()
{
  if (glBarrierCreateCR == NULL) /* not initialized */
    init_chromium_hooks();
  return (glCreateContextCR != NULL);
}

static void get_drawing_area( P_Renderer* self, 
			      int *x_corner, int *y_corner, 
			      unsigned int* width, unsigned int* height )
{
  /* Assumes set_drawing_window has been called */
#ifdef USE_OPENGL
#if defined(WIREGL)
  GLint params[4];
  glGetIntegerv(GL_VIEWPORT,params);
  *x_corner= params[0];
  *y_corner= params[1];
  *width= params[2];
  *height= params[3];
#else
  Arg args[10];
  int n= 0;
  unsigned int border_width;
  unsigned int depth;
  Window root;
  Status s;
  if (MANAGE(self) && !chromium_in_use()) {
    s= XGetGeometry(XDISPLAY(self),XWINDOW(self),&root,x_corner,y_corner,
		    width,height, &border_width, &depth);
    if (s != True) ger_fatal("Error: XGetGeometry failed!\n");
  }
  else {
    GLint params[4];
    glGetIntegerv(GL_VIEWPORT,params);
    *x_corner= params[0];
    *y_corner= params[1];
    *width= params[2];
    *height= params[3];
  }
#endif
#else
    getsize(width,height);
#endif
}

static void set_drawing_window( P_Renderer* self )
{
#ifdef USE_OPENGL
#if defined(WIREGL)
  wireGLMakeCurrent();
#else
  if (chromium_in_use()) {
    glMakeCurrentCR(0, CRCONTEXT(self));
  }
  else {
    if (glXMakeCurrent(XDISPLAY(self), XWINDOW(self), GLXCONTEXT(self)) 
	!= True)
      ger_error("Error: set_drawing_window: unable to set context!\n");
  }
#endif
#else
  if (!AUTO (self)) {
    winset(WINDOW(self));
  }
  else {
    GLXwinset (XtDisplay (WIDGET (self)), XtWindow (WIDGET (self)));
  }
#endif
}

static Bool WaitForNotify(Display *d, XEvent *e, char *arg) 
{
  return (e->type == MapNotify) && (e->xmap.window == (Window)arg); 
}

static void attach_drawing_window( P_Renderer* self )
{
  /* AUTO(self) is always true in here */

#ifdef USE_OPENGL
#if defined(WIREGL)
  XWINDOW(self)= 0;
  wireGLSetRank( RANK(self) );
  wireGLCreateContext();
  GLXCONTEXT(self)= NULL;
  wireGLMakeCurrent();
  if (NPROCS(self)>1) glBarrierCreate(BARRIER(self), NPROCS(self));
#else
  if (chromium_in_use()) {
    /* We're in a Chromium universe */
    XWINDOW(self)= 0;
    
    CRCONTEXT(self) = 
      glCreateContextCR(0, CR_RGB_BIT | CR_DEPTH_BIT | CR_DOUBLE_BIT);
    if (CRCONTEXT(self) <= 0) {
      ger_error("glCreateContextCR() call failed!\n");
      return;
    }
    glMakeCurrentCR(0, CRCONTEXT(self));
    
    if (NPROCS(self)>1) {
      glBarrierCreateCR(BARRIER(self), NPROCS(self));
    }
    
    GLXCONTEXT(self)= NULL;
  }
  else {

    XVisualInfo *vi;
    int attributeList[10];
    unsigned int width, height;
    int x, y;
    XSetWindowAttributes swa;
    XEvent event;
    Colormap cmap;
    
    attributeList[0]= GLX_RGBA;
    attributeList[1]= GLX_DOUBLEBUFFER;
    attributeList[2]= None;
    
    /* First we wait for the parent to be built */
    get_drawing_area(self, &x, &y, &width, &height); /* works since window is set to parent */
    
    vi= glXChooseVisual(XDISPLAY(self), DefaultScreen(XDISPLAY(self)), 
			attributeList);
    if (!vi) {
      ger_fatal("attach_drawing_window: unable to get a reasonable visual!");
    }
    GLXCONTEXT(self)= glXCreateContext(XDISPLAY(self), vi, 0, GL_TRUE);
    
    if (!GLXCONTEXT(self)) {
      ger_error("gl_ren_mthd: attach_drawing_widget: glXCreateContext failed; widget or window cannot support GL renderer!\n");
    }
    cmap = XCreateColormap(XDISPLAY(self), RootWindow(XDISPLAY(self), vi->screen),
			   vi->visual, AllocNone);
    if (!cmap) {
      ger_fatal("attach_drawing_window: XCreateColormap failed unexpectedly!");
    }
    
    /* create the sub window */
    swa.colormap = cmap;
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    XWINDOW(self)= XCreateWindow(XDISPLAY(self), XWINDOW(self), 0, 0, width, height,
				 0, vi->depth, InputOutput, vi->visual,
				 CWBorderPixel|CWColormap|CWEventMask, &swa);
    if (!XWINDOW(self)) {
      ger_fatal("attach_drawing_window: XCreateWindow of gl subwindow failed!");
    }
    
    /* map the subwindow */
    if (!XMapWindow(XDISPLAY(self), XWINDOW(self))) {
      ger_fatal("attach_drawing_window: cannot map created subwindow!");
    }
    XIfEvent(XDISPLAY(self), &event, WaitForNotify, (char*)XWINDOW(self));
  }

  RENDATA(self)->initialized= 1;

#endif
#else

  if (XtIsRealized (WIDGET (self))) {
    set_drawing_window(self);
    init_gl_gl(self);
    RENDATA(self)->initialized= 1;
  }
  else {
    ger_fatal("Error: Cannot initialize DrawP3D gl with an unrealized widget!\n");
    /* give up now;  calling sequence error */
  }

#endif
}

static void create_drawing_window( P_Renderer* self, char* size_info )
{
  /* AUTO(self) is always false in here */
#ifdef USE_OPENGL
#if defined(WIREGL)
  XWINDOW(self)= 0;
  wireGLSetRank( RANK(self) );
  wireGLCreateContext();
  wireGLMakeCurrent();
  GLXCONTEXT(self)= NULL;
  if (NPROCS(self)>1) glBarrierCreate(BARRIER(self), NPROCS(self));
#else
  if (chromium_in_use()) {
    /* We're in a Chromium universe */
    XWINDOW(self)= 0;
    
    CRCONTEXT(self) = 
      glCreateContextCR(0, CR_RGB_BIT | CR_DEPTH_BIT | CR_DOUBLE_BIT);
    if (CRCONTEXT(self) <= 0) {
      ger_error("glCreateContextCR() call failed!\n");
      return;
    }
    glMakeCurrentCR( 0, CRCONTEXT(self) );
    
    if (NPROCS(self)>1) {
      glBarrierCreateCR(BARRIER(self), NPROCS(self));
    }
    
    GLXCONTEXT(self)= NULL;
  }
  else {
    Display *dpy;
    Window win;
    int n;
    static char* fake_argv[]= {"drawp3d",NULL};
    int fake_argc= 1;
    XVisualInfo *vi;
    int attributeList[10];
    Colormap cmap;
    GLXContext cx;
    XEvent event;
    XSetWindowAttributes swa;

    attributeList[0]= GLX_RGBA;
    attributeList[1]= GLX_DOUBLEBUFFER;
    attributeList[2]= None;

    XDISPLAY(self)= dpy= XOpenDisplay(0);
    if (!dpy) {
      ger_fatal("create_drawing_window: unable to access display!");
    }
    vi= glXChooseVisual(dpy, DefaultScreen(dpy), attributeList);
    if (!vi) {
      ger_fatal("create_drawing_window: unable to get a reasonable visual!");
    }
    GLXCONTEXT(self)= cx= glXCreateContext(dpy, vi, 0, GL_TRUE);
    if (!cx) {
      ger_fatal("create_drawing_window: unable to make a GL context!");
    }
    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
			   vi->visual, AllocNone);
    if (!cmap) {
      ger_fatal("create_drawing_window: XCreateColormap failed unexpectedly!");
    }
    
    /* create a window */
    swa.colormap = cmap;
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 512, 512,
			0, vi->depth, InputOutput, vi->visual,
			CWBorderPixel|CWColormap|CWEventMask, &swa);
    if (!win) {
      ger_fatal("create_drawing_window: XCreateWindow failed unexpectedly!");
    }
    XWINDOW(self)= win;
    if (!XMapWindow(dpy, win)) {
      ger_fatal("create_drawing_window: cannot map created window!");
    }
    XIfEvent(dpy, &event, WaitForNotify, (char*)win);
  }

  /* OK, the window should be up. */
#endif
#else
  {
    int result; 
    int x; 
    int y;
    unsigned int width; 
    unsigned int height;
    
    if (size_info)
      result = XParseGeometry(size_info, &x, &y, &width, &height);
    else result= 0;

    if (result & (XValue | YValue)) {
      if (result & XNegative)
	x = getgdesc(GD_XPMAX) - x;
      if (! (result & YNegative))
	y = getgdesc(GD_YPMAX) - y;
      prefposition(x, x+width, y, y+height);
    };

    foreground();

    if (result & (WidthValue | HeightValue))
      prefsize(width, height);

    WINDOW(self) = winopen(NAME(self));
    winconstraints();
  }
#endif
}

static void release_drawing_window( P_Renderer* self )
{
  /* Destroys the window if appropriate */
#ifdef USE_OPENGL
  if (chromium_in_use()) {
    glDestroyContextCR(CRCONTEXT(self));
    CRCONTEXT(self)= -1;
  }
  else {
    glXDestroyContext(XDISPLAY(self),GLXCONTEXT(self));
    XDestroyWindow(XDISPLAY(self),XWINDOW(self));
  }
#else
  if (AUTO(self)) {
    /* Doesn't seem to be anything to do, since the caller owns the widget */
  }
  else {
    winclose(WINDOW(self));
  }
#endif
}

static void screen_door_transp( int flag )
{
#ifdef USE_OPENGL
  if (flag) {
    /* Transparency on */
    glPolygonStipple(greyPattern);
    glEnable(GL_POLYGON_STIPPLE);
    glDepthMask(GL_FALSE);
  }
  else {
    /* Transparency off */
    glDisable(GL_POLYGON_STIPPLE);
    glDepthMask(GL_TRUE);
  }
#else
  if (flag) {
    /* Transparency on */
    setpattern(GREYPATTERN);
    zwritemask(0);
  }
  else {
    /* Transparency off */
    setpattern(BLACKPATTERN);
    zwritemask(0xFFFFFFFF);
    lmcolor(LMC_COLOR);
  }
#endif
}

static void define_materials(P_Renderer *self)
{
#ifdef USE_OPENGL
  int lupe;

  /* in OpenGL, must define materials separately in each context */
  for (lupe=0; lupe < N_MATERIALS; lupe++) {
    GLfloat params[4];
    if (MANAGE(self)) set_drawing_window(self);
    glNewList( materials[lupe].obj=glGenLists(1) , GL_COMPILE);
    params[0]= params[1]= params[2]= materials[lupe].ambient;
    params[3]= 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, params);
    params[0]= params[1]= params[2]= materials[lupe].diffuse;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, params);
    params[0]= params[1]= params[2]= materials[lupe].specular;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, params);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 
		(GLfloat)materials[lupe].shininess);
    glEndList();
  }

#else

  static int materials_defined= 0;
  int lupe;

  if (!materials_defined) {
    static float mat_data[15]= {
      AMBIENT, 0.0, 0.0, 0.0,
      DIFFUSE, 0.0, 0.0, 0.0,
      SPECULAR, 0.0, 0.0, 0.0,
      SHININESS, 0.0,
      LMNULL
    };
    for (lupe=0; lupe < N_MATERIALS; lupe++) {
      mat_data[1]= mat_data[2]= mat_data[3]= materials[lupe].ambient;
      mat_data[5]= mat_data[6]= mat_data[7]= materials[lupe].diffuse;
      mat_data[9]= mat_data[10]= mat_data[11]= materials[lupe].specular;
      mat_data[13]= materials[lupe].shininess;
      lmdef(DEFMATERIAL, lupe+1, 15, mat_data);
      materials[lupe].index= lupe+1;
    }
    materials_defined= 1;
  }
#endif

}

static void update_materials(P_Renderer *self, int mat_index)
{
  if (mat_index < N_MATERIALS) {
    CURMATERIAL(self) = mat_index;
  } else {
    CURMATERIAL(self) = 0;
  }
#ifdef USE_OPENGL
  if (MANAGE(self)) set_drawing_window(self);
  glCallList(materials[CURMATERIAL(self)].obj);
#else
  lmbind(MATERIAL, materials[CURMATERIAL(self)].index);
#endif
}

static void get_rgb_color(P_Renderer *self, float* col, float val) {
  /*Return the RGB value from the colormap.*/

  /* Scale, clip, and map. */
  val= ( val-MAP_MIN(self) )/( MAP_MAX(self)-MAP_MIN(self) );
  if ( val > 1.0 ) val= 1.0;
  if ( val < 0.0 ) val= 0.0;
  (*MAP_FUN(self))( &val, &col[0], &col[1], &col[2], &col[3]);
}

static P_Cached_Vlist* cache_vlist( P_Renderer *self, P_Vlist *vlist )
     /* This routine outputs a vertex list */
{
  int i;
  P_Cached_Vlist* result;

  ger_debug("pvm_ren_mthd: cache_vlist");

  if ( !(result=(P_Cached_Vlist*)malloc(sizeof(P_Cached_Vlist))) ) {
    fprintf(stderr,"pvm_ren_mthd: cache_vlist: unable to allocate %d bytes!\n",
	    sizeof(P_Cached_Vlist));
    exit(-1);
  }

  METHOD_RDY(vlist)
    result->length= vlist->length;

  if ((vlist->type != P3D_CVTX)
      && (vlist->type != P3D_CCVTX)
      && (vlist->type != P3D_CNVTX)
      && (vlist->type != P3D_CCNVTX)
      && (vlist->type != P3D_CVVTX)
      && (vlist->type != P3D_CVNVTX)
      && (vlist->type != P3D_CVVVTX)) {
    fprintf(stderr,"pvm_ren_mthd: cache_vlist: got unknown vertex type %d!\n",
	    vlist->type);
  }

  /* Allocate memory as needed */
  if ( !(result->coords= 
	 (float*)malloc( 3*result->length*sizeof(float) )) ) {
    fprintf(stderr,"pvm_ren_mthd: cannot allocate %d bytes!\n",
	    3*result->length*sizeof(float));
    exit(-1);
  }

  if ((vlist->type==P3D_CCVTX)
      || (vlist->type==P3D_CCNVTX)
      || (vlist->type==P3D_CVVTX)
      || (vlist->type==P3D_CVVVTX)
      || (vlist->type==P3D_CVNVTX)) {
    if ( !(result->colors= 
	   (float*)malloc( 4*result->length*sizeof(float) )) ) {
      fprintf(stderr,"pvm_ren_mthd: cannot allocate %d bytes!\n",
	      4*result->length*sizeof(float));
      exit(-1);
    }
  }
  else {
    result->colors= NULL;
  }

  if ((vlist->type==P3D_CNVTX)
      || (vlist->type==P3D_CCNVTX)
      || (vlist->type==P3D_CVNVTX)) {
    if ( !(result->normals= 
	   (float*)malloc( 3*result->length*sizeof(float) )) ) {
      fprintf(stderr,"gl_ren_mthd: cannot allocate %d bytes!\n",
	      3*result->length*sizeof(float));
      exit(-1);
    }
  }
  else result->normals= NULL;

  if (result->colors) {
    if (result->normals) result->type= P3D_CCNVTX;
    else result->type= P3D_CCVTX;
  }
  else {
    if (result->normals) result->type= P3D_CNVTX;
    else result->type= P3D_CVTX;
  }

  for (i=0; i<result->length; i++) {
    result->coords[3*i]= (*(vlist->x))(i);
    result->coords[3*i+1]= (*(vlist->y))(i);
    result->coords[3*i+2]= (*(vlist->z))(i);
    switch (vlist->type) {
    case P3D_CVTX:
      break;
    case P3D_CNVTX:
      result->normals[3*i]= (*(vlist->nx))(i);
      result->normals[3*i+1]= (*(vlist->ny))(i);
      result->normals[3*i+2]= (*(vlist->nz))(i);
      break;
    case P3D_CCVTX:
      result->colors[4*i]= (*(vlist->r))(i);
      result->colors[4*i+1]= (*(vlist->g))(i);
      result->colors[4*i+2]= (*(vlist->b))(i);
      result->colors[4*i+3]= (*(vlist->a))(i);
      break;
    case P3D_CCNVTX:
      result->colors[4*i]= (*(vlist->r))(i);
      result->colors[4*i+1]= (*(vlist->g))(i);
      result->colors[4*i+2]= (*(vlist->b))(i);
      result->colors[4*i+3]= (*(vlist->a))(i);
      result->normals[3*i]= (*(vlist->nx))(i);
      result->normals[3*i+1]= (*(vlist->ny))(i);
      result->normals[3*i+2]= (*(vlist->nz))(i);
      break;
    case P3D_CVVTX:
    case P3D_CVVVTX:
      get_rgb_color( self, result->colors+4*i, (*(vlist->v))(i) );
      break;
    case P3D_CVNVTX:
      get_rgb_color( self, result->colors+4*i, (*(vlist->v))(i) );
      result->normals[3*i]= (*(vlist->nx))(i);
      result->normals[3*i+1]= (*(vlist->ny))(i);
      result->normals[3*i+2]= (*(vlist->nz))(i);
      break;
      /* We've already checked that it is a known case */
    }
  }

  return( result );
}

static void free_cached_vlist( P_Cached_Vlist* cache )
{
  if (cache->normals) free( (P_Void_ptr)(cache->normals) );
  if (cache->colors) free( (P_Void_ptr)(cache->colors) );
  free( (P_Void_ptr)cache->coords );
  free( (P_Void_ptr)cache );
}

static void send_cached_vlist( P_Cached_Vlist* cvlist )
{
  int i;
  float* crd_runner= cvlist->coords;
  float* clr_runner= cvlist->colors;
  float* nrm_runner= cvlist->normals;
  switch(cvlist->type) {
  case P3D_CVTX:
    for (i=0; i<cvlist->length; i++) {
#ifdef USE_OPENGL
      glVertex3fv(crd_runner);
#else
      v3f(crd_runner);
#endif
      crd_runner += 3;
    }
    break;
  case P3D_CCVTX:
    for (i=0; i<cvlist->length; i++) {
#ifdef  USE_OPENGL
      glColor4fv(clr_runner);
      glVertex3fv(crd_runner);
#else
      c4f(clr_runner);
      v3f(crd_runner);
#endif
      clr_runner += 4;
      crd_runner += 3;
    }
    break;
  case P3D_CNVTX:
    for (i=0; i<cvlist->length; i++) {
#ifdef USE_OPENGL
      glNormal3fv(nrm_runner);
      glVertex3fv(crd_runner);
#else
      n3f(nrm_runner);
      v3f(crd_runner);
#endif
      nrm_runner += 3;
      crd_runner += 3;
    }
    break;
  case P3D_CCNVTX:
    for (i=0; i<cvlist->length; i++) {
#ifdef USE_OPENGL
      glNormal3fv(nrm_runner);
      glColor4fv(clr_runner);
      glVertex3fv(crd_runner);
#else
      n3f(nrm_runner);
      c4f(clr_runner);
      v3f(crd_runner);
#endif
      nrm_runner += 3;
      clr_runner += 4;
      crd_runner += 3;
    }
    break;
  default:
    ger_error("gl_ren_mthd: send_cached_vlist: unknown vertex type!");
  }
}

static void destroy_object(P_Void_ptr the_thing) {
    
  gl_gob *it = (gl_gob *)the_thing;
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_object\n");
  
  if (it) {
#ifdef USE_OPENGL
    if (it->obj_info.obj && glIsList(it->obj_info.obj)) {
      glDeleteLists( it->obj_info.obj, 1);
    }
#else
    if (it->obj_info.obj && isobj(it->obj_info.obj)) {
      delobj(it->obj_info.obj);	
    }
#endif
    if (it->cvlist) free_cached_vlist( it->cvlist );
    free((void *)it);
  }
  METHOD_OUT
}

static void destroy_mesh(P_Void_ptr the_thing) {
    
  gl_gob *it = (gl_gob *)the_thing;
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_mesh\n");

#ifdef USE_GL_OBJ
  destroy_obj(the_thing);
#else
  if (it->obj_info.mesh_obj.indices) 
    free((void*)it->obj_info.mesh_obj.indices);
  if (it->obj_info.mesh_obj.facet_lengths) 
    free((void*)it->obj_info.mesh_obj.facet_lengths);
  if (it->cvlist) free_cached_vlist( it->cvlist );
  free((void *)it);
#endif

  METHOD_OUT
}

static void destroy_sphere(P_Void_ptr the_thing) {
  /*Destroy a sphere.*/
    
  gl_gob *it = (gl_gob *)the_thing;
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_sphere\n");

#ifdef USE_OPENGL
  if (the_thing == SPHERE(self)) {
    METHOD_OUT
    return;
  }
#else
  if (the_thing == my_sphere) {
    METHOD_OUT
    return;
  }
#endif

#ifdef AVOID_NURBS
  METHOD_RDY(ASSIST(self));
  (*(ASSIST(self)->destroy_sphere))(the_thing);
#else

  if (it) {
#ifdef USE_OPENGL
    if (it->obj_info.quad_obj) gluDeleteQuadric(it->obj_info.quad_obj);
#else
    if (it->obj_info.obj && isobj(it->obj_info.obj)) 
      delobj(it->obj_info.obj);	
#endif
    free((void *)it);
  }
#endif
  METHOD_OUT
}

static void destroy_cylinder(P_Void_ptr the_thing) {
  /*Destroys a cylinder.*/
    
  gl_gob *it = (gl_gob *)the_thing;
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_cylinder\n");

#ifdef USE_OPENGL
  if (it == CYLINDER(self)) {
    METHOD_OUT
    return;
  }
#else
  if (it == my_cylinder) {
    METHOD_OUT
    return;
  }
#endif

#ifdef AVOID_NURBS
  METHOD_RDY(ASSIST(self));
  (*(ASSIST(self)->destroy_cylinder))(the_thing);
#else

  if (it) {
#ifdef USE_OPENGL
    if (it->obj_info.quad_obj) gluDeleteQuadric(it->obj_info.quad_obj);
#else
    if (it->obj_info.obj && isobj(it->obj_info.obj)) delobj(it->obj_info.obj);	
#endif
    if (it->cvlist) free_cached_vlist( it->cvlist );
    free((void *)it);

  }
#endif

  METHOD_OUT
}

static void destroy_torus(P_Void_ptr the_thing) {
    
  gl_gob *it = (gl_gob *)the_thing;
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_torus\n");

#ifdef AVOID_NURBS
  METHOD_RDY(ASSIST(self));
  (*(ASSIST(self)->destroy_torus))(the_thing);
#else

  if (it) {
#ifdef USE_OPENGL
    free((P_Void_ptr)(it->obj_info.nurbs_obj.data));
    gluDeleteNurbsRenderer(it->obj_info.nurbs_obj.ren);
#else
    free((P_Void_ptr)(it->obj_info.nurbs_obj));
#endif
  }
  if (it->cvlist) free_cached_vlist( it->cvlist );
  free((void *)it);

#endif

  METHOD_OUT
}

static void destroy_bezier(P_Void_ptr the_thing) {
    
  gl_gob *it = (gl_gob *)the_thing;
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_bezier\n");

#ifdef AVOID_NURBS
  METHOD_RDY(ASSIST(self));
  (*(ASSIST(self)->destroy_bezier))(the_thing);
#else

  if (it) {
#ifdef USE_OPENGL
    free((P_Void_ptr)(it->obj_info.nurbs_obj.data));
    gluDeleteNurbsRenderer(it->obj_info.nurbs_obj.ren);
#else
    if (it->obj_info.obj && isobj(it->obj_info.obj)) {
      delobj(it->obj_info.obj);	
    }
#endif
  }
  if (it->cvlist) free_cached_vlist( it->cvlist );
  free((void *)it);

#endif

  METHOD_OUT
}

static void ren_transform(P_Transform trans) {
#ifdef USE_OPENGL
  int i,j;
  GLfloat theMatrix[16];
  float* runner1= theMatrix;
  float* runner2;
  for (i=0; i<4; i++) {
    runner1= theMatrix + i;
    runner2= trans.d + 4*i;
    for (j=0; j<4; j++) {
      *runner1= *runner2++;
      runner1 += 4;
    }
  }
  glMultMatrixf(theMatrix);
#else
  Matrix theMatrix;
  int lupe;
  int lup;
  int loope;
    
  for (lupe=lup=0;lupe<16;lupe+=4, lup++)
    for (loope=0;loope<4;loope++)
      theMatrix[loope][lup]=trans.d[lupe+loope];
  multmatrix(theMatrix);
#endif
}

static void ren_object(P_Void_ptr the_thing, P_Transform *transform,
		       P_Attrib_List *attrs) {
    
  /*P_Transform is an struct which contains float d[16];*/
  /*P_Attrib is a struct of
    P_Symbol attribute; where P_Symbol is just a P_Void_ptr
    int type; where type is te value type
    P_Void_ptr value;
    and two structs for the next and prev things in the list.
  */

  P_Renderer *self = (P_Renderer *)po_this;
  P_Color *pcolor;
  float color[4];
  P_Material *mat;
  METHOD_IN
	
  gl_gob *it = (gl_gob *)the_thing;
    
  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_object");
    if (!it) {
      ger_error("gl_ren_mthd: ren_object: null object data found.");
      METHOD_OUT
      return;
    }

    /*get the attributes*/
    METHOD_RDY(ASSIST(self));
    pcolor= (*(ASSIST(self)->color_attribute))(COLORSYMBOL(self));
#ifdef  USE_OPENGL
    if ( ((*(ASSIST(self)->bool_attribute))(BACKCULLSYMBOL(self))) )
      glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
#else
    backface((*(ASSIST(self)->bool_attribute))(BACKCULLSYMBOL(self)));
#endif
    mat= (*(ASSIST(self)->material_attribute))(MATERIALSYMBOL(self));
	
    /*prepare attributes*/
    /*we've already defined all of our materials at open time... :)*/
    /*Note: there is a significant performance penalty for frequently
      changing the current material via lmbind...*/

    if (CURMATERIAL(self) != mat->type)
      update_materials(self, mat->type);

    /*Set color mode...*/
#ifdef USE_OPENGL
    if (it->color_mode) {
      glEnable(GL_LIGHTING);
    }
    else {
      glDisable(GL_LIGHTING);
    }

#else
    lmcolor(it->color_mode); /*Some like it AD, some like it COLOR...*/
#endif

    /* What we're doing in the above call is distinguishing between the
     * case in which we're updating the COLOR of an object and the case
     * in which we're updating the color of the MATERIAL of the object.
     * 
     * This is a Really Important Distinction. Things with inherent color
     * attributes are updating their COLOR at various spots, while things
     * without color attributes, i.e. surfaces of vertices and normals,
     * must update their materials.
     *
     * I guess this'd be easier if we didn't implement materials, but...
     */
	

    /*Set color...*/
    rgbify_color(pcolor);

    color[0]= pcolor->r;
    color[1]= pcolor->g;
    color[2]= pcolor->b;
    color[3]= pcolor->a;
#ifdef USE_OPENGL
    glColor4fv(color);
#else
    c4f(color);
#endif

    if ((! hastransparent) && (color[3] < .5)) {
      screen_door_transp(1);
    }

    /*we should almost never need this segment, but then again,
      we might... */
    if (transform) {
#ifdef USE_OPENGL
      glPushMatrix();
#else
      pushmatrix();
#endif
      ren_transform(*transform);
    }
	
    /*and render*/
#ifdef USE_OPENGL
    if (MANAGE(self)) set_drawing_window(self);
    if (it->obj_info.obj) glCallList(it->obj_info.obj);
#else
    if (it->obj_info.obj) callobj(it->obj_info.obj);
#endif
    else ger_error("gl_ren_mthd: ren_object: null gl object found!");
    /*And restore*/
    if (transform)
#ifdef USE_OPENGL
      glPopMatrix();
#else
    popmatrix();
#endif

    if ((! hastransparent) && (color[3] < .5)) {
      screen_door_transp(0);
    }

    /*Gee, wasn't that easy? :)  */
    METHOD_OUT
  }
}

static int ren_prim_setup( P_Renderer* self, gl_gob* it, 
			   P_Transform* trans, P_Attrib_List* attrs )
{
  /* P_Transform is an struct which contains float d[16];*/
  /* P_Attrib is a struct of
   * P_Symbol attribute; where P_Symbol is just a P_Void_ptr
   * int type; where type is te value type
   * P_Void_ptr value;
   * and two structs for the next and prev things in the list.
   */
  P_Color *pcolor;
  float color[4];
  P_Material *mat;
  int screendoor_set= 0;

  /*get the attributes*/
  METHOD_RDY(ASSIST(self));
  pcolor= (*(ASSIST(self)->color_attribute))(COLORSYMBOL(self));
#ifdef USE_OPENGL
  if ( (*(ASSIST(self)->bool_attribute))(BACKCULLSYMBOL(self)) ) {
    glEnable(GL_CULL_FACE);
  }
  else {
    glDisable(GL_CULL_FACE);
  }
#else
  backface((*(ASSIST(self)->bool_attribute))(BACKCULLSYMBOL(self)));
#endif
  mat= (*(ASSIST(self)->material_attribute))(MATERIALSYMBOL(self));
	
  /* prepare attributes*/
  /* we've already defined all of our materials at open time... :)*/
  /* Note: there is a significant performance penalty for frequently
   * changing the current material via lmbind...*/

  if (CURMATERIAL(self) != mat->type) update_materials(self,mat->type);

  /*Set color mode...*/
#ifdef USE_OPENGL
  if (it->color_mode) {
    glEnable(GL_LIGHTING);
  }
  else {
    glDisable(GL_LIGHTING);
  }
#else
  lmcolor(it->color_mode); /*Some like it AD, some like it COLOR...*/
#endif

  /* What we're doing in the above call is distinguishing between the
   * case in which we're updating the COLOR of an object and the case
   * in which we're updating the color of the MATERIAL of the object.
   * 
   * This is a Really Important Distinction. Things with inherent color
   * attributes are updating their COLOR at various spots, while things
   * without color attributes, i.e. surfaces of vertices and normals,
   * must update their materials.
   *
   * I guess this'd be easier if we didn't implement materials, but...
   */
	
  /*Set color...*/
  rgbify_color(pcolor);

  color[0]= pcolor->r;
  color[1]= pcolor->g;
  color[2]= pcolor->b;
  color[3]= pcolor->a;
#ifdef USE_OPENGL
  glColor4fv(color);
#else
  c4f(color);
#endif

  if ((! hastransparent) && (color[3] < .5)) {
    screen_door_transp(1);
    screendoor_set= 1;
  }

  /*we should almost never need this segment, but then again,
	  we might... */
  if (trans) {
#ifdef USE_OPENGL
    glPushMatrix();
#else
    pushmatrix();
#endif
    ren_transform(*trans);
  }

  return screendoor_set;
}

static void ren_prim_finish( P_Transform *trans, int screendoor_set )
{
  if (trans)
#ifdef USE_OPENGL
    glPopMatrix();
#else
  popmatrix();
#endif
  
  if (screendoor_set) {
    screen_door_transp(0);
  }
}

static void sendVertex(int type, int index, float* coords,
		       float* colors, float* normals)
{
  switch(type) {
  case P3D_CVTX:
    /*Coordinate vertex list: feed and draw.*/
#ifdef USE_OPENGL
    glVertex3fv(coords + 3*index);
#else
    v3f(coords + 3*index);
#endif
    break;
  case P3D_CCVTX:
    /*Coordinate/color vertex list*/
#ifdef USE_OPENGL
    glColor4fv(colors+4*index);
    glVertex3fv(coords+3*index);
#else
    c4f(colors+4*index);
    v3f(coords+3*index);
#endif
    break;
  case P3D_CCNVTX:
    /*Coordinate/color/normal vertex list*/
#ifdef USE_OPENGL
    glNormal3fv(normals+3*index);
    glColor4fv(colors+4*index);
    glVertex3fv(coords+3*index);
#else
    n3f(normals+3*index);
    c4f(colors+4*index);
    v3f(coords+3*index);
#endif
    break;
  case P3D_CNVTX:
    /*Coordinate/normal vertex list*/
#ifdef USE_OPENGL
    glNormal3fv(normals+3*index);
    glVertex3fv(coords+3*index);
#else
    n3f(normals+3*index);
    v3f(coords+3*index);
#endif
    break;
  default:
    printf("gl_ren_mthd: sendVertex: null vertex type.\n");
    break;
  } /*switch(vertices->type)*/
}

static void ren_mesh(P_Void_ptr the_thing, P_Transform *transform,
		     P_Attrib_List *attrs) {
    
  /*P_Transform is an struct which contains float d[16];*/
  /*P_Attrib is a struct of
    P_Symbol attribute; where P_Symbol is just a P_Void_ptr
    int type; where type is te value type
    P_Void_ptr value;
    and two structs for the next and prev things in the list.
  */

  P_Renderer *self = (P_Renderer *)po_this;
  P_Color *pcolor;
  P_Material *mat;
  int screendoor_set= 0;
  int *facet_lengths;
  int *indices;
  int nfacets;
  float* coords;
  float* colors;
  float* normals;
  int lupe;
  int loope;
  METHOD_IN
	
  gl_gob *it = (gl_gob *)the_thing;
    
  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_mesh");
    if (!it) {
      ger_error("gl_ren_mthd: ren_mesh: null object data found.");
      METHOD_OUT
      return;
    }

#ifdef USE_GL_OBJ
    ren_object(the_thing, transform, attrs);
#else
      
    screendoor_set= ren_prim_setup( self, it, transform, attrs );

    if (it->cvlist 
	&& it->obj_info.mesh_obj.indices 
	&& it->obj_info.mesh_obj.facet_lengths) {

      facet_lengths= it->obj_info.mesh_obj.facet_lengths;
      indices= it->obj_info.mesh_obj.indices;
      nfacets= it->obj_info.mesh_obj.nfacets;
      coords= it->cvlist->coords;
      colors= it->cvlist->colors;
      normals= it->cvlist->normals;

#ifdef USE_OPENGL
      switch (it->cvlist->type) {
      case P3D_CVTX:
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, coords);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	break;
      case P3D_CCVTX:
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, coords);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, colors);
	glDisableClientState(GL_NORMAL_ARRAY);
	break;
	glEnableClientState(GL_COLOR_ARRAY);
      case P3D_CCNVTX:
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, coords);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, colors);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, normals);
	break;
      case P3D_CNVTX:
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, coords);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, normals);
	break;
      default:
	ger_error("Error: ren_mesh: unsupported vertex type %d!\n", it->cvlist->type);
      }
      switch (it->obj_info.mesh_obj.type) {
      case MESH_MIXED:
	for (loope=0; loope < nfacets; loope++) {
	  glDrawElements(GL_POLYGON, facet_lengths[loope], GL_UNSIGNED_INT, indices);
	  indices += facet_lengths[loope];
	}
	break;
      case MESH_TRI:
	glDrawElements(GL_TRIANGLES, 3*nfacets, GL_UNSIGNED_INT, indices);
	break;
      case MESH_QUAD:
	glDrawElements(GL_QUADS, 4*nfacets, GL_UNSIGNED_INT, indices);
	break;
      case MESH_STRIP:
	for (loope=0; loope < nfacets; loope++) {
	  int run;
	  if (facet_lengths[loope]==3) {
	    /* Try to gather up tris */
	    run= 1;
	    while (loope+run<nfacets) {
	      if (facet_lengths[loope+run]!=3) break;
	      run++;
	    }
	    glDrawElements(GL_TRIANGLES, 3*run, GL_UNSIGNED_INT, indices);
	    indices += 3*run;
	    loope+= run-1;
	  }
	  else {
	    /* A real strip */
	    glDrawElements(GL_TRIANGLE_STRIP, facet_lengths[loope], 
			   GL_UNSIGNED_INT, indices);
	    indices += facet_lengths[loope];
	  }
	}
	break;
      }
	
      /* Ideally I'd return to the original state, but I can't tell what it
       *  was. 
       */
#else

      for (loope=0; loope < nfacets; loope++) {
	bgnpolygon();
	for (lupe=0;lupe < facet_lengths[loope]; lupe++, indices++)
	  sendVertex(it->cvlist->type, *indices, coords, colors, normals);
	endpolygon();
      }
	  
#endif

    }
    else ger_error("gl_ren_mthd: ren_mesh: null mesh info found!");
    ren_prim_finish( transform, screendoor_set );

#endif /* ifdef USE_GL_OBJ */

    /*Gee, wasn't that easy? :)  */
    METHOD_OUT
  }
}

static void ren_sphere(P_Void_ptr the_thing, P_Transform *transform,
		       P_Attrib_List *attrs) {
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it = (gl_gob *)the_thing;
  int screendoor_set;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_sphere");
    if (!it) {
      ger_error("gl_ren_mthd: ren_sphere: null object data found.");
      METHOD_OUT
      return;
    }

#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_sphere))(the_thing, transform, attrs);
#else

    screendoor_set= ren_prim_setup( self, it, transform, attrs );

#ifdef USE_OPENGL
    if (it->obj_info.quad_obj) 
      gluSphere(it->obj_info.quad_obj, 1.0, SPHERE_SLICES, SPHERE_STACKS);
#else
    if (it->obj_info.obj) callobj(it->obj_info.obj);
#endif
    else ger_error("gl_ren_mthd: ren_sphere: null sphere obj found!");

    ren_prim_finish( transform, screendoor_set );

#endif

    METHOD_OUT
  }
}

static void ren_cylinder(P_Void_ptr the_thing, P_Transform *transform,
			 P_Attrib_List *attrs) {
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it = (gl_gob *)the_thing;
  int screendoor_set;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_cylinder");
    if (!it) {
      ger_error("gl_ren_mthd: ren_cylinder: null object data found.");
      METHOD_OUT
      return;
    }
#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_cylinder))(the_thing, transform, attrs);
#else

    screendoor_set= ren_prim_setup( self, it, transform, attrs );

#ifdef USE_OPENGL
    if (it->obj_info.quad_obj) {
      gluCylinder(it->obj_info.quad_obj, 1.0, 1.0, 1.0, 
		  CYLINDER_SLICES, CYLINDER_STACKS);
      gluQuadricOrientation(it->obj_info.quad_obj, GLU_INSIDE);
      gluDisk( it->obj_info.quad_obj, 0.0, 1.0, CYLINDER_SLICES, 1 );
      glTranslatef(0.0,0.0,1.0);
      gluQuadricOrientation(it->obj_info.quad_obj, GLU_OUTSIDE);
      gluDisk( it->obj_info.quad_obj, 0.0, 1.0, CYLINDER_SLICES, 1 );
    }
#else
    if (it->obj_info.obj) callobj(it->obj_info.obj);
#endif
    else ger_error("gl_ren_mthd: ren_cylinder: null cylinder obj found!");

    ren_prim_finish( transform, screendoor_set );

#endif
    METHOD_OUT
  }
}

static void ren_torus(P_Void_ptr the_thing, P_Transform *transform,
		      P_Attrib_List *attrs) {
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it = (gl_gob *)the_thing;
  int screendoor_set;
  METHOD_IN
	    
  if (RENDATA(self)->open) {

    ger_debug("gl_ren_mthd: ren_torus");

#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_torus))(it, transform, attrs);
#else

    if (!it) {
      ger_error("gl_ren_mthd: ren_torus: null object data found.");
      METHOD_OUT
      return;
    }

    screendoor_set= ren_prim_setup( self, it, transform, attrs );

#ifdef USE_OPENGL
    if (it->obj_info.nurbs_obj.ren && it->obj_info.nurbs_obj.data) {
      gluBeginSurface( it->obj_info.nurbs_obj.ren );
      gluNurbsSurface( it->obj_info.nurbs_obj.ren, 12, knots, 12, knots,  
		       4,  4 * 9,  
		       it->obj_info.nurbs_obj.data,  3,  3,  GL_MAP2_VERTEX_4);
      gluEndSurface( it->obj_info.nurbs_obj.ren );
    }

#else

    if (it->obj_info.nurbs_obj) {
      bgnsurface();
      nurbssurface(12,knots,12,knots,
		   4*sizeof(double), 4*sizeof(double) * 9,
		   it->obj_info.nurbs_obj, 3, 3, N_V3DR);
      endsurface();      
    }

#endif
    else ger_error("gl_ren_mthd: ren_torus: null torus obj found!");

    ren_prim_finish( transform, screendoor_set );

#endif
    METHOD_OUT
  }
}

static void ren_bezier(P_Void_ptr the_thing, P_Transform *transform,
		       P_Attrib_List *attrs) {
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it = (gl_gob *)the_thing;
  int screendoor_set;
  METHOD_IN
	    
  if (RENDATA(self)->open) {

    ger_debug("gl_ren_mthd: ren_bezier");
#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_bezier))(the_thing, transform, attrs);
#else

    if (!it) {
      ger_error("gl_ren_mthd: ren_bezier: null object data found.");
      METHOD_OUT
      return;
    }
    screendoor_set= ren_prim_setup( self, it, transform, attrs );

#ifdef USE_OPENGL

    if (it->obj_info.nurbs_obj.ren && it->obj_info.nurbs_obj.data) {
      static GLfloat bez_knots[8]= {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
      gluBeginSurface( it->obj_info.nurbs_obj.ren );
      if (it->obj_info.nurbs_obj.flags) {
	/* color info is present */
	GLfloat* clr_data= it->obj_info.nurbs_obj.data + 3*16;
	gluNurbsSurface( it->obj_info.nurbs_obj.ren, 
			 8, bez_knots, 8, bez_knots, 4, 4*4, 
			 clr_data,  4,  4,  
			 GL_MAP2_COLOR_4);
      }
      gluNurbsSurface( it->obj_info.nurbs_obj.ren, 
		       8, bez_knots, 8, bez_knots, 3, 4*3, 
		       it->obj_info.nurbs_obj.data,  4,  4,  GL_MAP2_VERTEX_3);
      gluEndSurface( it->obj_info.nurbs_obj.ren );
    }

#else
    if (it->obj_info.obj) callobj(it->obj_info.obj);

#endif
    else ger_error("gl_ren_mthd: ren_torus: null bezier obj found!");

    ren_prim_finish( transform, screendoor_set );

#endif
    METHOD_OUT
  }
}

static void ren_polyline(P_Void_ptr the_thing, P_Transform *transform,
			 P_Attrib_List *attrs) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it= (gl_gob*)the_thing;
  int screendoor_set;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_polyline");
    if (!it) {
      ger_error("gl_ren_mthd: ren_polyline: null object data found.");
      METHOD_OUT
      return;
    }
    screendoor_set= ren_prim_setup( self, it, transform, attrs );
    if (it->cvlist) {
#ifdef USE_OPENGL
      glBegin(GL_LINE_STRIP);
#else
      bgnline();
#endif

      send_cached_vlist( it->cvlist );

#ifdef USE_OPENGL
      glEnd();
#else
      endline();
#endif
    }
    else ger_error("gl_ren_mthd: ren_polyline: null cvlist found!");
    ren_prim_finish( transform, screendoor_set );
  }
  METHOD_OUT
}

static void ren_polygon(P_Void_ptr the_thing, P_Transform *transform,
			P_Attrib_List *attrs) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it= (gl_gob*)the_thing;
  int screendoor_set;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_polygon");
    if (!it) {
      ger_error("gl_ren_mthd: ren_polygon: null object data found.");
      METHOD_OUT
      return;
    }
    screendoor_set= ren_prim_setup( self, it, transform, attrs );

    if (it->cvlist) {
#ifdef USE_OPENGL
      glBegin(GL_POLYGON);
#else      
      bgnpolygon();
#endif
      send_cached_vlist( it->cvlist );
#ifdef USE_OPENGL
      glEnd();
#else
      endpolygon();
#endif
    }
    else ger_error("gl_ren_mthd: ren_polygon: null cvlist found!");
    ren_prim_finish( transform, screendoor_set );
  }
  METHOD_OUT
}

static void ren_polymarker(P_Void_ptr the_thing, P_Transform *transform,
			   P_Attrib_List *attrs) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it= (gl_gob*)the_thing;
  int screendoor_set;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_polymarker");
    if (!it) {
      ger_error("gl_ren_mthd: ren_polymarker: null object data found.");
      METHOD_OUT
      return;
    }
    screendoor_set= ren_prim_setup( self, it, transform, attrs );
    if (it->cvlist) {
#ifdef USE_OPENGL
      glBegin(GL_POINTS);
#else
      bgnpoint();
#endif
      send_cached_vlist( it->cvlist );
#ifdef USE_OPENGL
      glEnd();
#else
      endpoint();
#endif
    }
    else ger_error("gl_ren_mthd: ren_polymarker: null cvlist found!");
    ren_prim_finish( transform, screendoor_set );
  }
  METHOD_OUT
}

static void ren_tristrip(P_Void_ptr the_thing, P_Transform *transform,
			 P_Attrib_List *attrs) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  gl_gob *it= (gl_gob*)the_thing;
  int screendoor_set;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_polyline");
    if (!it) {
      ger_error("gl_ren_mthd: ren_polyline: null object data found.");
      METHOD_OUT
      return;
    }
    screendoor_set= ren_prim_setup( self, it, transform, attrs );
    if (it->cvlist) {
#ifdef USE_OPENGL
      glBegin(GL_TRIANGLE_STRIP);
#else
      bgntmesh();
#endif
      send_cached_vlist( it->cvlist );
#ifdef USE_OPENGL
      glEnd();
#else
      endtmesh();
#endif
    }
    else ger_error("gl_ren_mthd: ren_polyline: null cvlist found!");
    ren_prim_finish( transform, screendoor_set );
  }
  METHOD_OUT
}

static P_Void_ptr def_sphere(char *name) {
  /*Define a unit sphere at the origin and return it.*/
    
  P_Renderer *self= (P_Renderer *)po_this;
  gl_gob *it;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: def_sphere");

#ifdef USE_OPENGL
    if (SPHERE_DEFINED(self)) {
      METHOD_OUT
      return(SPHERE(self));
    }
#else
    if (my_sphere) {
      METHOD_OUT
      return((P_Void_ptr)my_sphere);
    }
#endif

#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
#ifdef USE_OPENGL
    result= SPHERE(self)= (*(ASSIST(self)->def_sphere))();
    SPHERE_DEFINED(self)= 1;
#else
    my_sphere= result= (*(ASSIST(self)->def_sphere))();
#endif
    METHOD_OUT
    return( (P_Void_ptr)result );
#else

    if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
      ger_fatal("def_sphere: unable to allocate %d bytes!", sizeof(gl_gob));

#ifdef USE_OPENGL
    it->obj_info.quad_obj= gluNewQuadric();
    it->color_mode= 1;
    SPHERE(self)= it;
    SPHERE_DEFINED(self)= 1;
#else

    sphobj( it->obj_info.obj= genobj() );
    it->color_mode = LMC_AD;

#endif

    it->cvlist= NULL;
    METHOD_OUT
    return((P_Void_ptr)it);

#endif
  }
  
  METHOD_OUT
  return((P_Void_ptr)0); /* not reached */
}

#ifndef USE_OPENGL
void mycircle(float radius, int up) {
  /*Necessary for def_cylinder to work
    properly: draws a circle of radius radius
    in the xy plane at the origin.*/
    
#define NSIDES 20.0
    
  register float lupe, inc;
  float ver[3], nor[3];

  ver[2] = 0;
  inc = ((2.0*PI)/NSIDES);

  nor[0] = nor[1] = 0;
  if (up)
    nor[2] = 1;
  else
    nor[2] = -1;
    
  bgntmesh();
  n3f(nor);

  if (up) {
    for (lupe = 0.0; lupe <= PI; lupe += inc) {
      ver[0] = sin((double)lupe)*radius;
      ver[1] = cos((double)lupe)*radius;
      v3f(ver);
      ver[0] = sin(-(double)lupe)*radius;
      ver[1] = cos(-(double)lupe)*radius;
      v3f(ver);
    }
  }
  else {
    for (lupe = 0.0; lupe <= PI; lupe += inc) {
      ver[0] = sin(-(double)lupe)*radius;
      ver[1] = cos(-(double)lupe)*radius;
      v3f(ver);
      ver[0] = sin((double)lupe)*radius;
      ver[1] = cos((double)lupe)*radius;
      v3f(ver);
    }
  }
    
  endtmesh();
#undef NSIDES
}
#endif

static P_Void_ptr def_cylinder(char *name) 
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  gl_gob *it;
  METHOD_IN;
  
  if (RENDATA(self)->open) {
    
    ger_debug("gl_ren_mthd: def_cylinder");
    
    /*DON'T redefine things!*/
#ifdef USE_OPENGL
    if (CYLINDER_DEFINED(self)) {
      METHOD_OUT
      return((P_Void_ptr)CYLINDER(self));
    }
#else
    if (my_cylinder) {
      METHOD_OUT
      return((P_Void_ptr)my_cylinder);
    }
#endif
    
#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_cylinder))();
    METHOD_OUT
    return( (P_Void_ptr)result );
#else
    
    if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
      ger_fatal("def_cylinder: unable to allocate %d bytes!", sizeof(gl_gob));
    
#ifdef USE_OPENGL
    it->obj_info.quad_obj= gluNewQuadric();
    it->color_mode= 1;
    it->cvlist= NULL;
    CYLINDER(self)= it;
    CYLINDER_DEFINED(self)= 1;
#else
    {
      double surfknotsx[CYL_NUMKNOTSX] = { -1., -1., 1., 1. };
    
      double surfknotsy[CYL_NUMKNOTSY] = 
      { -1., -1., -1., -1., -1., -1., 1., 1., 1., 1., 1., 1. };
    
#define CON1 1.775
#define CON2 2.2
      double ctlpoints[CYL_NUMPOINTSY][CYL_NUMPOINTSX * CYL_NUMCOORDS] = {
	-1.0,  0.0,  0.0,
	-1.0,  0.0,  1.0,
	-1.0,  CON1,  0.0,
	-1.0,  CON1,  1.0,
	CON2,  CON1,  0.0,
	CON2,  CON1,  1.0,
	CON2, -CON1,  0.0,
	CON2, -CON1,  1.0,
	-1.0, -CON1,  0.0,
	-1.0, -CON1,  1.0,
	-1.0,  0.0,  0.0,
	-1.0,  0.0,  1.0};
#undef CON1
#undef CON2

      /*As of now, we're defining the object*/
      makeobj( it->obj_info.obj=genobj() );
      it->color_mode = LMC_AD;
      it->cvlist= NULL;
      translate(0, 0, 1);
      mycircle(1.0, TRUE);
      translate(0, 0, 0);
      mycircle(1.0, FALSE);
      
      bgnsurface();
      nurbssurface( 
		   sizeof( surfknotsx) / sizeof( double ), surfknotsx, 
		   sizeof( surfknotsy) / sizeof( double ), surfknotsy, 
		   sizeof(double) * CYL_NUMCOORDS, 
		   sizeof(double) * CYL_NUMPOINTSX * CYL_NUMCOORDS, 
		   (double *)ctlpoints, 
		   CYL_ORDERX, CYL_ORDERY, 
		   N_V3D
		   );
      endsurface();
      
      closeobj();
      my_cylinder = it;
    }
#endif
    METHOD_OUT
    return( (P_Void_ptr)it );
    
#endif
    
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_torus(char *name, double major, double minor) {
  P_Renderer *self= (P_Renderer *)po_this;
  gl_gob *it;
  P_Void_ptr result;
  METHOD_IN
  
  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: def_torus");
#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_torus))(major,minor);
    METHOD_OUT
    return( (P_Void_ptr)result );
#else
    {
      int i, j;
#ifdef USE_OPENGL
      GLfloat* knot_data= NULL;
      GLfloat* runner;
#else
      double* knot_data= NULL;
      double* runner;
#endif
	
      if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	ger_fatal("def_torus: unable to allocate %d bytes!", 
		  sizeof(gl_gob));
#ifdef USE_OPENGL
      if ( !(knot_data= (GLfloat*)malloc(9*9*4*sizeof(GLfloat))) )
#else
	if ( !(knot_data= (double*)malloc(9*9*4*sizeof(double))) )
#endif
	  ger_fatal("def_torus: unable to allocate %d bytes!",
		    9*9*4*sizeof(double));
	
      runner= knot_data;
      for(j=0;j<9;j++)  {
	for(i=0;i<9;i++)  {
	  *runner++= (minor * bx[j] + major * w[j]) * bx[i];
	  *runner++= (minor * bx[j] + major * w[j]) * by[i];
	  *runner++= minor * w[i] * by[j];
	  *runner++= w[i] * w[j];
	}
      }
	
#ifdef USE_OPENGL
      it->obj_info.nurbs_obj.data= knot_data;
      it->obj_info.nurbs_obj.ren= gluNewNurbsRenderer();
      it->obj_info.nurbs_obj.flags= 0;
      it->color_mode= 1;
#else
      it->obj_info.nurbs_obj= knot_data;
      it->color_mode= LMC_AD;
#endif
      it->cvlist= NULL;
	
      METHOD_OUT
      return((P_Void_ptr)it);
    }
#endif
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_gob(P_Void_ptr primdata, P_Transform *thistrans,
		    P_Attrib_List *thisattrlist) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
	
  ger_debug("gl_ren_mthd: ren_gob\n");
  
  if (RENDATA(self)->open) {
    
    if (!(RENDATA(self)->initialized)) {
      init_gl_gl(self);
      RENDATA(self)->initialized= 1;
    }

    if (primdata) {
      P_Gob *thisgob;
      int error=0;
      int top_level_call=0;
      P_Gob_List *kidlist;
      float *back;
      
      thisgob= (P_Gob *)primdata;

      if (thisattrlist) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->push_attributes))( thisattrlist );
      }
      
      if (thisgob->attr) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->push_attributes))( thisgob->attr );
      }
      
      /* If thistrans is non-null, this is a top-level call to
       * ren_gob, as opposed to a recursive call.
       */
      if (thistrans) {
	unsigned long cval;
	unsigned int xsize, ysize;
	int x_corner, y_corner;

	top_level_call = 1;

#ifdef USE_OPENGL
#if !defined(WIREGL)
	if (MANAGE(self) && !AUTO(self) && !chromium_in_use()) {
	  /* We have to run a little event loop, because no one else is. */
	  int event_pending= 1;
	  long event_mask= 
	    ( StructureNotifyMask | ExposureMask | ButtonPressMask | FocusChangeMask );
	  while (event_pending) {
	    XEvent event;
	    if (XCheckWindowEvent(XDISPLAY(self),XWINDOW(self),
				  event_mask,&event)) {
	      /* Think what fun we could have parsing these! */
	      switch (event.type) {
	      case CirculateNotify: fprintf(stderr,"CirculateNotify\n"); break;
	      case ConfigureNotify: fprintf(stderr,"ConfigureNotify\n"); break;
	      case DestroyNotify: fprintf(stderr,"DestroyNotify\n"); break;
	      case MapNotify: fprintf(stderr,"MapNotify\n"); break;
	      case UnmapNotify: fprintf(stderr,"UnmapNotify\n"); break;
	      }
	    }
	    else event_pending= 0;
	  }
	}
#endif
#endif

	if (MANAGE(self)) set_drawing_window(self);
	get_drawing_area(self,&x_corner,&y_corner,&xsize,&ysize);
	ASPECT (self) = (float) (xsize - 1) / (float) (ysize - 1);

#ifdef USE_OPENGL
	if (NPROCS(self)<2 || RANK(self)==0) {
	  GLPROF("Clearing");
	  glClearColor(BACKGROUND(self)[0], BACKGROUND(self)[1], 
		       BACKGROUND(self)[2], BACKGROUND(self)[3]);
	  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	}
#if defined(WIREGL)
	if (NPROCS(self)>1) glBarrierExec(BARRIER(self));
#else
	if (NPROCS(self)>1 && chromium_in_use()) 
	  glBarrierExecCR( BARRIER(self) );
#endif
	GLPROF("BuildingCoordTrans")
	  glPushMatrix();
	gluLookAt(LOOKFROM(self).x,LOOKFROM(self).y,LOOKFROM(self).z,
		  LOOKAT(self).x, LOOKAT(self).y, LOOKAT(self).z, 
		  LOOKUP(self).x, LOOKUP(self).y, LOOKUP(self).z);
#else
	viewport (0, xsize + 1, 0, ysize + 1);
	lmcolor(LMC_COLOR);
	back = BACKGROUND(self);
	cval= ((int)((back[0]*255)+0.5))
	  | (((int)((back[1]*255)+0.5)) << 8)
	  | (((int)((back[2]*255)+0.5)) << 16)
	  | (((int)((back[3]*255)+0.5)) << 24);
	GLPROF("Clearing");
	czclear(cval,getgconfig(GC_ZMIN));
	pushmatrix();
	lookat(LOOKFROM(self).x, LOOKFROM(self).y, LOOKFROM(self).z,
	       LOOKAT(self).x, LOOKAT(self).y, LOOKAT(self).z,
	       LOOKANGLE(self));
#endif
      }
      GLPROF(thisgob->name);
      if ( thisgob->has_transform )
	ren_transform(thisgob->trans);
      
      /* There must be children to render, because if this was a primitive
       * this method would not be getting executed.
       */
      kidlist= thisgob->children;
      while (kidlist) {
#ifdef USE_OPENGL
	glPushMatrix();
#else
	pushmatrix();
#endif

	METHOD_RDY(kidlist->gob);
	(*(kidlist->gob->render_to_ren))(self, (P_Transform *)0,
					 (P_Attrib_List *)0);
	kidlist= kidlist->next;
#ifdef USE_OPENGL
	glPopMatrix();
#else
	popmatrix();
#endif
      }
      
      if (thisgob->attr) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->pop_attributes))( thisgob->attr );
      }
      
      if (thisattrlist) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->pop_attributes))( thisattrlist );
      }
      
      if (top_level_call) {
#ifdef USE_OPENGL
	glPopMatrix();

#if defined(WIREGL)
	if (NPROCS(self)>1) glBarrierExec(BARRIER(self));

	if (MANAGE(self) && (NPROCS(self)<2 || RANK(self)==0))
	  wireGLSwapBuffers();
#ifdef WIREGL_INSTRUMENT
	else if (MANAGE(self) && (NPROCS(self)>1 || RANK(self)!=0))
	  wireGLInstrumentNextFrame();
#endif

#else
	if (NPROCS(self)>1) {
	  /* If NPROCS > 1, we can assume Chromium */
	  glBarrierExecCR( BARRIER(self) );
	}
	if (MANAGE(self)) {
	  if (chromium_in_use()) {
	    /* The crserver only executes the SwapBuffers() for the 0th client.
	     * No need to test for rank==0 as we used to do.
	     */
	    glSwapBuffersCR(0, 0);
	  }
	  else {
	    glXSwapBuffers(XDISPLAY(self),XWINDOW(self));
	  }
	}
#endif
	glFlush();
#else
	popmatrix();
	if (MANAGE(self)) swapbuffers();
	gflush();
#endif
      }
      
    }
  }
  METHOD_OUT
}

static void traverse_gob( P_Void_ptr primdata, P_Transform *thistrans,
			  P_Attrib_List *thisattrlist )
     /* This routine traverses a gob looking for lights. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Gob *thisgob = (P_Gob *)primdata;
  P_Gob_List *kidlist;
  register short lupe;
  METHOD_IN

  if (RENDATA(self)->open) {
    
    if (primdata) {
      
      ger_debug("gl_ren_mthd: traverse_gob\n");
      
      /*First, check to see if this lighting gob has already been
	defined...*/
      /*If we're using the same lights as before don't bother to
	re-render them.*/
      
      if (thisgob == DLIGHTBUFFER(self)) {
	METHOD_OUT
	return;
      }
      /*Okay, *now* we need to render them. :)*/

      if (thistrans) {
	/* Top level call; initialize lighting state and traversal */

	if (MANAGE(self)) set_drawing_window(self);

	DLIGHTBUFFER(self) = thisgob;
      
	/*Clear the ambient lighting...*/
	AMBIENTCOLOR(self)[0] = 0.0;
	AMBIENTCOLOR(self)[1] = 0.0;
	AMBIENTCOLOR(self)[2] = 0.0;
	AMBIENTCOLOR(self)[3] = 0.0;
      
#ifdef USE_OPENGL
	/*First, clear all lights.*/
	for (lupe= 0; lupe < MY_GL_MAX_LIGHTS; lupe++) {
	  glDisable(GL_LIGHT0 + lupe);
	}
	
	glPushMatrix();
#else
	/*First, clear all lights.*/
	for (lupe = LIGHT0; lupe <= DLIGHTCOUNT(self); lupe++)
	  lmbind(lupe, 0);
	DLIGHTCOUNT(self) = LIGHT0;
	
	pushmatrix();
#endif
      }

      if ( thisgob->has_transform ) ren_transform(thisgob->trans);

      if (thisgob->attr) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->push_attributes))( thisgob->attr );
      }
      
      GLPROF(thisgob->name);
      /* There must be children to render, because if this was a primitive
       * this method would not be getting executed.
       */
      kidlist= thisgob->children;
      while (kidlist) {
#ifdef USE_OPENGL
	glPushMatrix();
#else
	pushmatrix();
#endif
	METHOD_RDY(kidlist->gob);
	(*(kidlist->gob->traverselights_to_ren))
	  (self,(P_Transform *)0,(P_Attrib_List *)0);
	kidlist= kidlist->next;
#ifdef USE_OPENGL
	glPopMatrix();
#else
	popmatrix();
#endif
      }

      if (thisgob->attr) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->pop_attributes))( thisgob->attr );
      }

      if (thistrans) {
#ifdef USE_OPENGL
	glPopMatrix();
	
#else
	popmatrix();
	
	/*Now the ambient lighting should be set...*/
	LM(self)[1] = AMBIENTCOLOR(self)[0];
	LM(self)[2] = AMBIENTCOLOR(self)[1];
	LM(self)[3] = AMBIENTCOLOR(self)[2];
	
	lmdef(DEFLMODEL, 3, 9, LM(self));
	lmbind(LMODEL, 3);
#endif
      }
      
    }
  }
  METHOD_OUT
}

static void destroy_gob( P_Void_ptr primdata )
     /* This routine destroys the renderer's data for the gob. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    P_Gob *gob;
    gob= (P_Gob *)primdata;
    ger_debug("gl_ren_mthd: destroy_gob: destroying gob <%s>",gob->name);
  }
  METHOD_OUT
}

static P_Void_ptr def_polyline(char *name, P_Vlist *vertices) {
    
  gl_gob *it;
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }

  ger_debug("gl_ren_mthd: def_polyline\n");

  if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
    ger_fatal("def_polyline: unable to allocate %d bytes!", 
	      sizeof(gl_gob));
    
#ifdef USE_OPENGL
  it->color_mode= 0;
  it->obj_info.obj= (GLuint)0;
#else
  it->color_mode= LMC_COLOR;
  it->obj_info.obj= NULL;
#endif
  it->cvlist= cache_vlist(self, vertices);

  METHOD_OUT
  return((P_Void_ptr)it);
}

static P_Void_ptr def_polygon(char *name, P_Vlist *vertices) {
    
  gl_gob *it;
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }
    
  ger_debug("gl_ren_mthd: def_polygon\n");

  if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
    ger_fatal("def_polygon: unable to allocate %d bytes!", sizeof(gl_gob));
    
#ifdef USE_OPENGL
  it->color_mode= 0;
  it->obj_info.obj= (GLuint)0;
#else
  it->color_mode= LMC_COLOR;
  it->obj_info.obj= NULL;
#endif
  it->cvlist= cache_vlist(self, vertices);

  METHOD_OUT
  return((P_Void_ptr)it);
}

static P_Void_ptr def_polymarker(char *name, P_Vlist *vertices) {
    
  gl_gob *it;
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }
    
  ger_debug("gl_ren_mthd: def_polymarker\n");

  if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
    ger_fatal("def_polymarker: unable to allocate %d bytes!", sizeof(gl_gob));
    
#ifdef USE_OPENGL
  it->color_mode= 0;
  it->obj_info.obj= (GLuint)0;
#else
  it->color_mode= LMC_COLOR;
  it->obj_info.obj= NULL;
#endif
  it->cvlist= cache_vlist(self, vertices);

  METHOD_OUT
  return((P_Void_ptr)it);
}

static P_Void_ptr def_tristrip(char *name, P_Vlist *vertices) {
    
  gl_gob *it;
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }
    
  ger_debug("gl_ren_mthd: def_tristrip\n");

  if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
    ger_fatal("def_tristrip: unable to allocate %d bytes!", sizeof(gl_gob));
    
#ifdef USE_OPENGL
  it->color_mode= 0;
  it->obj_info.obj= (GLuint)0;
#else
  it->color_mode= LMC_COLOR;
  it->obj_info.obj= NULL;
#endif
  it->cvlist= cache_vlist(self, vertices);
  METHOD_OUT
  return((P_Void_ptr)it);
}

static P_Void_ptr def_bezier(char *name, P_Vlist *vertices) {
  int color_flag= 0;
  int value_flag= 0;
  int value_flag_2= 0;
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  gl_gob *it;
    
  METHOD_IN

  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }
  
  ger_debug("gl_ren_mthd: def_bezier");

#ifdef AVOID_NURBS
  METHOD_RDY(ASSIST(self));
  result= (*(ASSIST(self)->def_bezier))(vertices);
  METHOD_OUT
  return( (P_Void_ptr)result );
#else

  if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
    ger_fatal("def_bezier: unable to allocate %d bytes!", sizeof(gl_gob));
  it->cvlist= NULL;
  
  switch (vertices->type) {
  case P3D_CVVVTX:
    value_flag_2 = 0;
    value_flag = 1;
    color_flag = 0;
    break;
  case P3D_CVNVTX:
    value_flag_2 = 0;
    value_flag = 1;
    color_flag = 0;
    break;
  case P3D_CVVTX:
    value_flag_2 = 0;
    value_flag = 1;
    color_flag = 0;
    break;
  case P3D_CCNVTX:
  case P3D_CCVTX:
    value_flag_2 = 0;
    value_flag = 0;
    color_flag = 1;
    break;
  case P3D_CVTX:
    value_flag_2 = 0;
    color_flag = 0;
    value_flag = 0;
    break;
  case P3D_CNVTX:
    value_flag_2 = 0;
    color_flag = 0;
    value_flag = 0;
    break;
  }
  
#ifdef USE_OPENGL
  {
    GLfloat* knot_data= NULL;
    GLfloat* runner;
    GLfloat* crunner= NULL;
    int buf_length;
    float col4[4];
    int i;

    if ((!color_flag) && (!value_flag) && (!value_flag_2)) 
      buf_length= 3*16;
    else buf_length= 3*16 + 4*16;
    if ( !(knot_data= (GLfloat*)malloc(buf_length*sizeof(GLfloat))) ) 
      ger_fatal("def_bezier: unable to allocate %d bytes!",
		buf_length*sizeof(GLfloat));

    runner= knot_data;
    if (color_flag || value_flag || value_flag_2) 
      crunner= knot_data + 3*16;
    METHOD_RDY(vertices);
    for (i=0; i<16; i++) {
      *runner++= (*vertices->x)(i);
      *runner++= (*vertices->y)(i);
      *runner++= (*vertices->z)(i);
      if (color_flag) {
	*crunner++ = (*vertices->r)(i);
	*crunner++ = (*vertices->g)(i);
	*crunner++ = (*vertices->b)(i);
	*crunner++ = (*vertices->a)(i);
      } else if (value_flag) {
	get_rgb_color(self, col4, (*vertices->v)(i));
	*crunner++ = (col4[0]);
	*crunner++ = (col4[1]);
	*crunner++ = (col4[2]);
	*crunner++ = (col4[3]);
      } else if (value_flag_2) {
	get_rgb_color(self, col4, (*vertices->v2)(i));
	*crunner++ = (col4[0]);
	*crunner++ = (col4[1]);
	*crunner++ = (col4[2]);
	*crunner++ = (col4[3]);
      }
    }

    it->obj_info.nurbs_obj.data= knot_data;
    it->obj_info.nurbs_obj.ren= gluNewNurbsRenderer();
    if (color_flag || value_flag || value_flag_2) {
      it->obj_info.nurbs_obj.flags= 1;
    }
    else {
      it->obj_info.nurbs_obj.flags= 0;
    }
    it->color_mode= 1;    
  }
#else
  {
    static double bez_knots[8]= {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
    double coords[3*16];
    double colors[4*16];
    double *coordptr, *clrptr;
    float col4[4];
    int i;

    coordptr= coords;
    clrptr= colors;
    METHOD_RDY(vertices);
    for (i=0; i<16; i++) {
      *coordptr++= (*vertices->x)(i);
      *coordptr++= (*vertices->y)(i);
      *coordptr++= (*vertices->z)(i);
      if (color_flag) {
	*clrptr++ = (*vertices->r)(i);
	*clrptr++ = (*vertices->g)(i);
	*clrptr++ = (*vertices->b)(i);
	*clrptr++ = (*vertices->a)(i);
      } else if (value_flag) {
	get_rgb_color(self, col4, (*vertices->v)(i));
	*clrptr++ = (col4[0]);
	*clrptr++ = (col4[1]);
	*clrptr++ = (col4[2]);
	*clrptr++ = (col4[3]);
      } else if (value_flag_2) {
	get_rgb_color(self, col4, (*vertices->v2)(i));
	*clrptr++ = (col4[0]);
	*clrptr++ = (col4[1]);
	*clrptr++ = (col4[2]);
	*clrptr++ = (col4[3]);
      }
    }
  
    makeobj( it->obj_info.obj=genobj() );
    /*As of now, we're defining the object*/
    bgnsurface();
    
    if (color_flag || value_flag || value_flag_2) {
      it->color_mode= LMC_AD;
      nurbssurface( 8, bez_knots, 8, bez_knots,
		    4*sizeof(double), 4*4*sizeof(double), colors,
		    4, 4, N_C4D );
    }
    else it->color_mode= LMC_COLOR;
    
    nurbssurface( 8, bez_knots, 8, bez_knots,
		  3*sizeof(double), 4*3*sizeof(double), coords,
		  4, 4, N_V3D );
    
    endsurface();
    closeobj();
  }
#endif

  METHOD_OUT
	
  return((P_Void_ptr)it);
#endif
}

static P_Void_ptr def_mesh(char *name, P_Vlist *vertices, int *indices,
			   int *facet_lengths, int nfacets) {
  P_Renderer *self= (P_Renderer *)po_this;
  gl_gob *it;
  int lupe, loope;
  int total_indices; 
  gl_mesh_type mesh_type;
  int i;
  int j;
  float vtx[3];
  float col4[4], nor[3];
  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }

  ger_debug("gl_ren_mthd: def_mesh");
  if (vertices->length < 3) {
    ger_error("gl_ren_mthd: def_mesh: %d isn't enough vertices for a mesh!", vertices->length);
    METHOD_OUT
    return((P_Void_ptr)0);
  }

  if (nfacets<1) {
    ger_error("gl_ren_mthd: def_mesh: %d isn't enough facets for a mesh!", nfacets);
    METHOD_OUT
    return((P_Void_ptr)0);
  }

  if ( !(it= (gl_gob *)malloc(sizeof(gl_gob))))
    ger_fatal("gl_ren_mthd: def_mesh: unable to allocate %d bytes!"
	      , sizeof(gl_gob));
  it->cvlist= NULL;
    
#ifdef USE_GL_OBJ
#ifdef USE_OPENGL
  if (MANAGE(self)) set_drawing_window(self);
  glNewList( it->obj_info.obj=glGenLists(1) , GL_COMPILE);
#else
  makeobj( it->obj_info.obj=genobj() );
#endif
  /*As of now, we're defining the object*/
    
  switch (vertices->type) {
  case P3D_CNVTX:  /*Fixed for std. gobs*/
  case P3D_CCNVTX:
  case P3D_CVNVTX: /*Fixed*/
  case P3D_CVTX:
#ifdef USE_OPENGL
    it->color_mode = 1;
#else
    it->color_mode = LMC_AD;
#endif
    break;
  case P3D_CCVTX:
  case P3D_CVVTX:  /*Fixed*/
  case P3D_CVVVTX:
#ifdef USE_OPENGL
    it->color_mode = 0;
#else
    it->color_mode = LMC_COLOR;
#endif
    break;
  }

  METHOD_RDY(vertices);
  for (loope=0; loope < nfacets; loope++) {
#ifdef USE_OPENGL
    if (*facet_lengths == 3)
      glBegin(GL_TRIANGLES);
    else if (*facet_lengths == 4)
      glBegin(GL_QUADS);
    else
      glBegin(GL_POLYGON);
#else
    if (*facet_lengths == 3)
      bgntmesh();
    else
      bgnpolygon();
#endif
    switch(vertices->type) {
    case P3D_CVTX:
      /*Coordinate vertex list: feed and draw.*/
      for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
	vtx[0] = (*vertices->x)(*indices);
	vtx[1] = (*vertices->y)(*indices);
	vtx[2] = (*vertices->z)(*indices);
#ifdef USE_OPENGL
	glVertex3fv(vtx);
#else
	v3f(vtx);
#endif
      }
      break;
    case P3D_CCVTX:
      /*Coordinate/color vertex list*/
      for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
	col4[0] = (*vertices->r)(*indices);
	col4[1] = (*vertices->g)(*indices);
	col4[2] = (*vertices->b)(*indices);
	col4[3] = (*vertices->a)(*indices);
	vtx[0] = (*vertices->x)(*indices);
	vtx[1] = (*vertices->y)(*indices);
	vtx[2] = (*vertices->z)(*indices);
#ifdef USE_OPENGL
	glColor4fv(col4);
	glVertex3fv(vtx);
#else
	c4f(col4);
	v3f(vtx);
#endif
      }
      break;
    case P3D_CCNVTX:
      /*Coordinate/color/normal vertex list*/
      for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
	nor[0] = (*vertices->nx)(*indices);
	nor[1] = (*vertices->ny)(*indices);
	nor[2] = (*vertices->nz)(*indices);
	col4[0] = (*vertices->r)(*indices);
	col4[1] = (*vertices->g)(*indices);
	col4[2] = (*vertices->b)(*indices);
	col4[3] = (*vertices->a)(*indices);
	vtx[0] = (*vertices->x)(*indices);
	vtx[1] = (*vertices->y)(*indices);
	vtx[2] = (*vertices->z)(*indices);
#ifdef USE_OPENGL
	glNormal3fv(nor);
	glColor4fv(col4);
	glVertex3fv(vtx);
#else
	n3f(nor);
	c4f(col4);
	v3f(vtx);
#endif
      }
      break;
    case P3D_CNVTX:
      /*Coordinate/normal vertex list*/
      for (lupe=0;lupe < *facet_lengths; lupe++, indices++) {
	nor[0] = (*vertices->nx)(*indices);
	nor[1] = (*vertices->ny)(*indices);
	nor[2] = (*vertices->nz)(*indices);
	vtx[0] = (*vertices->x)(*indices);
	vtx[1] = (*vertices->y)(*indices);
	vtx[2] = (*vertices->z)(*indices);
#ifdef USE_OPENGL
	glNormal3fv(nor);
	glVertex3fv(vtx);
#else
	n3f(nor);
	v3f(vtx);
#endif
      }
      break;
    case P3D_CVVTX:
      /*Coordinate/value vertex list*/
      for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
	get_rgb_color(self, col4,(*vertices->v)(*indices));
	vtx[0] = (*vertices->x)(*indices);
	vtx[1] = (*vertices->y)(*indices);
	vtx[2] = (*vertices->z)(*indices);
#ifdef USE_OPENGL
	glColor4fv(col4);
	glVertex3fv(vtx);
#else
	c4f(col4);
	v3f(vtx);
#endif
      }
      break;
    case P3D_CVNVTX:
      /*Coordinate/normal/value vertex list*/
      for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
	nor[0] = (*vertices->nx)(*indices);
	nor[1] = (*vertices->ny)(*indices);
	nor[2] = (*vertices->nz)(*indices);
	get_rgb_color(self, col4,(*vertices->v)(*indices));
	vtx[0] = (*vertices->x)(*indices);
	vtx[1] = (*vertices->y)(*indices);
	vtx[2] = (*vertices->z)(*indices);
#ifdef USE_OPENGL
	glNormal3fv(nor);
	glColor4fv(col4);
	glVertex3fv(vtx);
#else
	n3f(nor);
	c4f(col4);
	v3f(vtx);
#endif
      }
      break;
    case P3D_CVVVTX:
      /*Coordinate/value/value vertex list*/
      for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
	get_rgb_color(self, col4,(*vertices->v)(*indices));
	vtx[0] = (*vertices->x)(*indices);
	vtx[1] = (*vertices->y)(*indices);
	vtx[2] = (*vertices->z)(*indices);
#ifdef USE_OPENGL
	glColor4fv(col4);
	glVertex3fv(vtx);
#else
	c4f(col4);
	v3f(vtx);
#endif
      }
      break;
    default:
      printf("gl_ren_mthd: def_mesh: null vertex type.\n");
      break;
    } /*switch(vertices->type)*/
	
#ifdef USE_OPENGL
    glEnd();
#else
    if (*facet_lengths == 3)
      endtmesh();
    else
      endpolygon();
#endif
    facet_lengths++;
  }
    
#ifdef USE_OPENGL
  glEndList();
#else
  closeobj();    
#endif

#else /* ifdef USE_GL_OBJ */

#ifdef USE_OPENGL
  switch (vertices->type) {
  case P3D_CNVTX:  /*Fixed for std. gobs*/
  case P3D_CCNVTX:
  case P3D_CVNVTX: /*Fixed*/
  case P3D_CVTX:
#ifdef USE_OPENGL
    it->color_mode = 1;
#else
    it->color_mode = LMC_AD;
#endif
    break;
  case P3D_CCVTX:
  case P3D_CVVTX:  /*Fixed*/
  case P3D_CVVVTX:
#ifdef USE_OPENGL
    it->color_mode = 0;
#else
    it->color_mode = LMC_COLOR;
#endif
    break;
  }
  it->obj_info.obj= (GLuint)0;
#else
  it->color_mode= LMC_COLOR;
  it->obj_info.obj= NULL;
#endif

  /* Count total indices, and classify this mesh */
  if (facet_lengths[0] == 3) mesh_type= MESH_TRI;
  else if (facet_lengths[0] == 4) mesh_type= MESH_QUAD;
  else mesh_type= MESH_MIXED;
  total_indices= facet_lengths[0];
  for (i=1; i<nfacets; i++) {
    total_indices += facet_lengths[i];
    if (facet_lengths[i] != facet_lengths[0]) mesh_type= MESH_MIXED;
  }
#if (STRIP_MESHES != 0)
  if (mesh_type==MESH_TRI) {
    int* newIndex= NULL;
    int* mark= NULL;
    int* newLength= NULL;
    int* here;
    int odd;
    int length;
    int firstFree= 0;
    int nFree= nfacets;
    int count= 0;

    if (!(newIndex= (int*)malloc(3*nfacets*sizeof(int)))) 
      ger_fatal("def_mesh: unable to allocate %d bytes!\n",
		3*nfacets*sizeof(int));
    if (!(newLength= (int*)malloc(nfacets*sizeof(int)))) 
      ger_fatal("def_mesh: unable to allocate %d bytes!\n",
		nfacets*sizeof(int));
    if (!(mark= (int*)malloc(nfacets*sizeof(int)))) 
      ger_fatal("def_mesh: unable to allocate %d bytes!\n",
		nfacets*sizeof(int));
    for (i=0; i<nfacets; i++) mark[i]= 0;

    here= newIndex;

    *here++= indices[3*firstFree];
    *here++= indices[3*firstFree+1];
    *here++= indices[3*firstFree+2];
    mark[firstFree]= 1;
    nFree--;
    firstFree += 1;
    odd= 1;
    length= 3;

    while (nFree) {
      /* Start a new strip on firstFree */

      /* Run from here to the end, looking for a good next cell */

      int limit= firstFree+STRIP_PATIENCE;
      if (limit>nfacets) limit= nfacets;
      for (i=firstFree; i<limit; i++) {
	if (!mark[i]) {
	  if (odd) {
	    if (indices[3*i]==*(here-2)&&indices[3*i+2]==*(here-1)) {
	      *here++= indices[3*i+1];
	      break;
	    }
	    else if (indices[3*i+1]==*(here-2)&&indices[3*i]==*(here-1)) {
	      *here++= indices[3*i+2];
	      break;
	    }
	    else if (indices[3*i+2]==*(here-2)&&indices[3*i+1]==*(here-1)) {
	      *here++= indices[3*i];
	      break;
	    }
	  }
	  else { /* even case */
	    if (indices[3*i+2]==*(here-2)&&indices[3*i]==*(here-1)) {
	      *here++= indices[3*i+1];
	      break;
	    }
	    else if (indices[3*i]==*(here-2)&&indices[3*i+1]==*(here-1)) {
	      *here++= indices[3*i+2];
	      break;
	    }
	    else if (indices[3*i+1]==*(here-2)&&indices[3*i+2]==*(here-1)) {
	      *here++= indices[3*i];
	      break;
	    }
	  }
	}
      }
      if (i!=limit) {
	/* matched */
	mark[i]= 1;
	nFree--;
	odd= !odd;
	length++;
      }
      else {
	/* start a new strip */
	newLength[count]= length;
	count += 1;
	while (firstFree<nfacets && mark[firstFree]) firstFree++;
	if (firstFree==nfacets) break;
	*here++= indices[3*firstFree];
	*here++= indices[3*firstFree+1];
	*here++= indices[3*firstFree+2];
	nFree--;
	mark[firstFree]= 1;
	while (firstFree<nfacets && mark[firstFree]) firstFree++;
	odd= 1;
	length= 3;
	if (firstFree==nfacets) break;
      }
    }
    newLength[count]= length;
    count += 1;
    if (count<STRIP_BENEFIT*nfacets) {
      /* OK, let's use the stripped mesh */
      free(mark);
      it->obj_info.mesh_obj.indices= newIndex;
      it->obj_info.mesh_obj.facet_lengths= newLength;
      it->obj_info.mesh_obj.nfacets= count;
      it->obj_info.mesh_obj.type= MESH_STRIP;
    }
    else {
      /* It's not worth it; cache the original info */
      free(newIndex);
      free(newLength);
      free(mark);

      if (!(it->obj_info.mesh_obj.indices= 
	    (int*)malloc(total_indices*sizeof(int))))
	ger_fatal("def_mesh: unable to allocate %d bytes!",
		  total_indices*sizeof(int));
      for (i=0; i<total_indices; i++) 
	it->obj_info.mesh_obj.indices[i]= indices[i];
      if (!(it->obj_info.mesh_obj.facet_lengths= 
	    (int*)malloc(nfacets*sizeof(int))))
	ger_fatal("def_mesh: unable to allocate %d bytes!",
		  nfacets*sizeof(int));
      for (i=0; i<nfacets; i++)
	it->obj_info.mesh_obj.facet_lengths[i]= facet_lengths[i];
      it->obj_info.mesh_obj.nfacets= nfacets;
      it->obj_info.mesh_obj.type= MESH_TRI;
	
    }
    it->cvlist= cache_vlist(self, vertices);
  }
  else {
    /* Cache any mesh which is not triangles */
    if (!(it->obj_info.mesh_obj.indices= 
	  (int*)malloc(total_indices*sizeof(int))))
      ger_fatal("def_mesh: unable to allocate %d bytes!",
		total_indices*sizeof(int));
    for (i=0; i<total_indices; i++) 
      it->obj_info.mesh_obj.indices[i]= indices[i];
    if (!(it->obj_info.mesh_obj.facet_lengths= 
	  (int*)malloc(nfacets*sizeof(int))))
      ger_fatal("def_mesh: unable to allocate %d bytes!",
		nfacets*sizeof(int));
    for (i=0; i<nfacets; i++)
      it->obj_info.mesh_obj.facet_lengths[i]= facet_lengths[i];
    it->obj_info.mesh_obj.nfacets= nfacets;
    it->obj_info.mesh_obj.type= mesh_type;
    
    it->cvlist= cache_vlist(self, vertices);
  }
#else

  /* Cache everything */
  if (!(it->obj_info.mesh_obj.indices= 
	(int*)malloc(total_indices*sizeof(int))))
    ger_fatal("def_mesh: unable to allocate %d bytes!",
	      total_indices*sizeof(int));
  for (i=0; i<total_indices; i++) 
    it->obj_info.mesh_obj.indices[i]= indices[i];
  if (!(it->obj_info.mesh_obj.facet_lengths= 
	(int*)malloc(nfacets*sizeof(int))))
    ger_fatal("def_mesh: unable to allocate %d bytes!",
	      nfacets*sizeof(int));
  for (i=0; i<nfacets; i++)
    it->obj_info.mesh_obj.facet_lengths[i]= facet_lengths[i];
  it->obj_info.mesh_obj.nfacets= nfacets;
  it->obj_info.mesh_obj.type= mesh_type;

  it->cvlist= cache_vlist(self, vertices);

#endif /* STRIP_MESHES != 0 */
#endif /* USE_GL_OBJ */
    
  METHOD_OUT
  return( (P_Void_ptr)it );
}

#ifdef USE_OPENGL
static int pick_free_light(P_Renderer* self)
{
  int i;
  for (i=0; i<MY_GL_MAX_LIGHTS; i++) {
    if (!LIGHT_IN_USE(self)[i]) {
      LIGHT_IN_USE(self)[i]++;
      return i;
    }
  }
  ger_error("gl_ren_mthd: ran out of lights!\n");
  LIGHT_IN_USE(self)[MY_GL_MAX_LIGHTS-1]++;
  return MY_GL_MAX_LIGHTS-1;
}
#endif

static P_Void_ptr def_light(char *name, P_Point *point, P_Color *color) {
    
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return 0;
  }
    
  ger_debug("gl_ren_mthd: def_light\n");
    
#ifdef USE_OPENGL
  {
    PGL_Light* light= NULL;

    if (!(light=(PGL_Light*)malloc(sizeof(PGL_Light))))
      ger_fatal("def_light: unable to allocate %d bytes!\n",
		sizeof(PGL_Light));
    light->cvals[0]= color->r;
    light->cvals[1]= color->g;
    light->cvals[2]= color->b;
    light->cvals[3]= color->a;
    light->xvals[0]= point->x;
    light->xvals[1]= point->y;
    light->xvals[2]= point->z;
    light->xvals[3]= 0.0;
    light->light_num= pick_free_light(self);

    METHOD_OUT
    return((P_Void_ptr)light);
  }
#else
  {
    float the_light[] = {
      LCOLOR, 1, 1, 1,
      POSITION, 1, 1, 1, 0,
      LMNULL };
      
    the_light[1] = color->r;
    the_light[2] = color->g;
    the_light[3] = color->b;
      
    the_light[5] = point->x;
    the_light[6] = point->y;
    the_light[7] = point->z;
      
    /*WARNING: DOING IT THIS WAY LIMITS ONE TO USING ONLY A SHORT OF LIGHTS
      IF THIS MESSES UP IT'LL NEED TO BE REWRITTEN.*/
      
    lmdef(DEFLIGHT, light_num, 10, the_light);
      
    METHOD_OUT
    return ((P_Void_ptr)light_num++);
  }

#endif
}

void traverse_light(P_Void_ptr it, P_Transform *foo,
		    P_Attrib_List *bar) {

  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return;
  }

  ger_debug("gl_ren_mthd: traverse_light");
    
#ifdef USE_OPENGL

  {
    PGL_Light* light= (PGL_Light*)it;
      
    glLightfv( GL_LIGHT0 + light->light_num, GL_DIFFUSE, light->cvals );
    glLightfv( GL_LIGHT0 + light->light_num, GL_SPECULAR, light->cvals );
    glLightfv( GL_LIGHT0 + light->light_num, GL_POSITION, light->xvals );
    glEnable( GL_LIGHT0 + light->light_num );
  }

#else
  /*Okay... let's figure out how many lights are in use...
     and quit if we have too many...*/
  if ((LIGHT0 + MAXLIGHTS) == DLIGHTCOUNT(self))
    return;

  lmbind(DLIGHTCOUNT(self)++, (short)it);
#endif
    
  METHOD_OUT
}

void ren_light(P_Void_ptr it, P_Transform *foo,
	       P_Attrib_List *bar) {

  /*Hmm... let's just do nothing, for now...*/
}

void destroy_light(P_Void_ptr it) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return;
  }
	
  ger_debug("gl_ren_mthd: destroy_light\n");

#ifdef USE_OPENGL
  {
    PGL_Light* light= (PGL_Light*)it;
    if ((--LIGHT_IN_USE(self)[light->light_num])==0)
      glDisable( GL_LIGHT0 + light->light_num );
    free(light);
  }
#else
  /*Hmm... Maybe if we do resource allocation one of these days there'll
    actually be something to do here...*/
#endif
    
  METHOD_OUT
}

static P_Void_ptr def_ambient(char *name, P_Color *color) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }

  ger_debug("gl_ren_mthd: def_ambient\n");

#ifdef USE_OPENGL
  {
    PGL_Light *light= NULL;

    if (!(light=(PGL_Light*)malloc(sizeof(PGL_Light))))
      ger_fatal("def_light: unable to allocate %d bytes!\n",
		sizeof(PGL_Light));
    light->cvals[0]= color->r;
    light->cvals[1]= color->g;
    light->cvals[2]= color->b;
    light->cvals[3]= color->a;
    light->xvals[0]= light->xvals[1]= light->xvals[2]=
      light->xvals[3]= 0.0;
    light->light_num= pick_free_light(self);

    METHOD_OUT
    return((P_Void_ptr)light);
  }

#else
    
  /*Let's just put this somewhere we can use it later.*/

  METHOD_OUT
  return ((P_Void_ptr)color);
#endif
}

void traverse_ambient(P_Void_ptr it, P_Transform *foo,
		      P_Attrib_List *bar) {

  P_Color *color = (P_Color *)it;
  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return;
  }

  ger_debug("gl_ren_mthd: ren_ambient");

#ifdef USE_OPENGL
  {
    PGL_Light* light= (PGL_Light*)it;
      
    glLightfv( GL_LIGHT0 + light->light_num, GL_AMBIENT, light->cvals );
    glEnable( GL_LIGHT0 + light->light_num );
  }

#else
  /*Ambient lighting sums... at least, the painter renderer seems
      to think it does... so let's just add in the ambient lighting
      here...*/
    
  rgbify_color(color);
    
  AMBIENTCOLOR(self)[0] += color->r;
  AMBIENTCOLOR(self)[1] += color->g;
  AMBIENTCOLOR(self)[2] += color->b;
  AMBIENTCOLOR(self)[3] += color->a;
#endif        
    
  METHOD_OUT
}

void ren_ambient(P_Void_ptr it, P_Transform *foo,
		 P_Attrib_List *bar) {

  /*Hmm... let's just do nothing, for now...*/
}

void destroy_ambient(P_Void_ptr it) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return;
  }
	
  ger_debug("gl_ren_mthd: destroy_ambient\n");

#ifdef USE_OPENGL
  {
    PGL_Light* light= (PGL_Light*)it;
    if ((--LIGHT_IN_USE(self)[light->light_num])==0)
      glDisable( GL_LIGHT0 + light->light_num );
    free(light);
  }
#else
  /* nothing to be done */
#endif

  METHOD_OUT
}

static void hold_gob(P_Void_ptr gob) {
  /*This doesn't do anything*/

  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return;
  }

    
  ger_debug("gl_ren_mthd: hold_gob\n");

  METHOD_OUT
}

static void unhold_gob(P_Void_ptr gob) {
  /*This doesn't do anything*/

  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return;
  }
	
  ger_debug("gl_ren_mthd: unhold_gob\n");
  METHOD_OUT
}

static P_Void_ptr def_camera(struct P_Camera_struct *thecamera) {
    
  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }
	

  ger_debug("gl_ren_mthd: def_camera\n");
  /*I think all I want to do for this method is to
    stash the camera, and deal with it in the set_camera
    routine.*/
    
  METHOD_OUT
  return ((P_Void_ptr)thecamera);
}

static void set_camera(P_Void_ptr thecamera) {

  P_Renderer *self = (P_Renderer *)po_this;
  P_Camera *camera = (P_Camera *)thecamera;
  P_Vector looking, theCross;
  float norm;
  float result;
  short short_result;

  METHOD_IN	
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return;
  }
	
  ger_debug("gl_ren_mthd: set_camera\n");

  /*Well, it was stashed, and now we get to use it.
    Happy Happy Joy Joy.*/
    
  if (MANAGE(self)) set_drawing_window(self);
  if (!(RENDATA(self)->initialized)) {
    init_gl_gl(self);
    RENDATA(self)->initialized= 1;
  }

#ifdef USE_OPENGL
  {
    GLint mm;
    glGetIntegerv(GL_MATRIX_MODE, &mm);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(.1*(10*(camera->fovea)), ASPECT(self), 
		   (0-(camera->hither)),  (0-(camera->yon)));
    glMatrixMode(mm);
  };
#else
  perspective(10*(camera->fovea), ASPECT(self), 
	      (0-(camera->hither)), (0-(camera->yon)));
#endif

  LOOKFROM(self).x = camera->lookfrom.x;
  LOOKFROM(self).y = camera->lookfrom.y;
  LOOKFROM(self).z = camera->lookfrom.z;
  LOOKAT(self).x = camera->lookat.x;
  LOOKAT(self).y = camera->lookat.y;
  LOOKAT(self).z = camera->lookat.z;
  LOOKUP(self).x = camera->up.x;
  LOOKUP(self).y = camera->up.y;
  LOOKUP(self).z = camera->up.z;

  theCross.x = ((looking.y = (camera->lookat.y - camera->lookfrom.y))*camera->up.z) - ((looking.z = (camera->lookat.z - camera->lookfrom.z))*camera->up.y);
  theCross.y = (looking.z*camera->up.x) - ((looking.x = (camera->lookat.x - camera->lookfrom.x))*camera->up.z);
  theCross.z = (looking.x*camera->up.y) - (looking.y*camera->up.x);
    
  norm = sqrt((double)((theCross.x*theCross.x) 
		       + (theCross.y*theCross.y) 
		       + (theCross.z*theCross.z)));
    
  if (norm) {
    result = 10.0*RadtoDeg*asin((double)(theCross.y/norm));	
    if ( !(((looking.z <= 0.0) && (theCross.x >= 0.0))
	   || ((looking.z >= 0.0) && (theCross.x <= 0.0))) ) {
      result= 1800 - result;
    }
    short_result = (short)(result+0.5);
    LOOKANGLE(self) = short_result;

    /*Don't ask me how I came up with the angle.*/
    /*It's quite messy.*/
  }
  else
    ger_error("gl_ren_mthd: set_camera: up vector is parallel to \
                   viewing vector; setting up angle to %d.", LOOKANGLE(self));

  BACKGROUND(self)[0] = camera->background.r;
  BACKGROUND(self)[1] = camera->background.g;
  BACKGROUND(self)[2] = camera->background.b;
  BACKGROUND(self)[3] = 1.0;

  METHOD_OUT
}    
    
static void destroy_camera(P_Void_ptr thecamera) {
  ger_debug("gl_ren_mthd: destroy_camera\n");

  /*I don't have anything to deallocate... :)*/
}    
    
static P_Void_ptr def_text(char *name, char *tstring, P_Point *location,
			   P_Vector *u, P_Vector *v) {
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: def_text");
    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_text))( tstring, location, u, v );
    METHOD_OUT
    return( (P_Void_ptr)result );
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_text( P_Void_ptr rendata, P_Transform *trans,
		      P_Attrib_List *attr )
     /* This method renders a text string */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: ren_text");
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_text))(rendata, trans, attr);
  }
  METHOD_OUT
}

static void destroy_text( P_Void_ptr rendata )
     /* This method destroys a text string */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: destroy_text");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_text))(rendata);

  }
  METHOD_OUT
}

static void ren_print() {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gl_ren_mthd: ren_print");
  if ( RENDATA(self)->open ) {
    if (AUTO(self)) {
      ind_write("RENDERER: GL generator '%s', does not own window, open",
		self->name); 
    }
    else {
      ind_write("RENDERER: GL generator '%s', owns window, open",
		self->name); 
    }
  }
  else {
    if (AUTO(self)) {
      ind_write("RENDERER: GL generator '%s', does not own window, closed",
		self->name); 
    }
    else {
      ind_write("RENDERER: GL generator '%s', owns window, closed",
		self->name); 
    }
  }
  ind_eol();

  METHOD_OUT
}

static void ren_open() {
  /*opening the renderer.*/
    
  P_Renderer *self=(P_Renderer *)po_this;
    
  METHOD_IN

  ger_debug("gl_ren_mthd: ren_open\n");
  RENDATA(self)->open=1;
    
  METHOD_OUT
}

static void ren_close() {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("gl_ren_mthd: ren_close");
  RENDATA(self)->open= 0;
  METHOD_OUT
}

static void ren_destroy() {
  /*Kill everything. Everything.*/
  P_Renderer *self=(P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gl_ren_mthd: ren_destroy\n");

  if (MANAGE(self)) release_drawing_window(self);

  METHOD_RDY(ASSIST(self));
  (*(ASSIST(self)->destroy_self))();
    
  free( (P_Void_ptr)NAME(self) );
  free( (P_Void_ptr)BACKGROUND(self) );
  free( (P_Void_ptr)AMBIENTCOLOR(self) );
#ifdef USE_OPENGL
#ifdef WIREGL
  if (NPROCS(self)>1) glBarrierDestroy(BARRIER(self));
#else
  if (CRCONTEXT(self)>0) {
    glDestroyContextCR(CRCONTEXT(self));
    CRCONTEXT(self)= -1;
  }
  if (NPROCS(self)>1) glBarrierDestroyCR( BARRIER(self) );
#endif
#else
  free( (P_Void_ptr)LM(self) );
#endif
  free ((P_Void_ptr)self);
  METHOD_DESTROYED
    }

static P_Void_ptr def_cmap(char *name, double min, double max, 
			   void(*mapfun)(float *, float *, float *, float *, float *)) {
  P_Renderer *self= (P_Renderer *)po_this;
  P_Renderer_Cmap *thismap;
  METHOD_IN
  
  if (! (RENDATA(self)->open)) {
    METHOD_OUT
    return((P_Void_ptr)0);
  }

  ger_debug("gl_ren_mthd: def_cmap");

  if (max == min) {
    ger_error("gl_ren_mthd: def_cmap: max equals min (val is %f)", max );
    METHOD_OUT;
    return( (P_Void_ptr)0 );
  }

  if ( RENDATA(self)->open ) {
    if ( !(thismap= (P_Renderer_Cmap *)malloc(sizeof(P_Renderer_Cmap))) )
      ger_fatal( "gl_ren_mthd: def_cmap: unable to allocate %d bytes!",
		 sizeof(P_Renderer_Cmap) );

    strncpy(thismap->name, name, MAXSYMBOLLENGTH-1);
    (thismap->name)[MAXSYMBOLLENGTH-1]= '\0';
    thismap->min= min;
    thismap->max= max;
    thismap->mapfun= mapfun;
    METHOD_OUT
    return( (P_Void_ptr)thismap );
  }
  METHOD_OUT
  return( (P_Void_ptr)0 );
}

static void install_cmap(P_Void_ptr mapdata) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gl_ren_mthd: install_cmap");
  if ( RENDATA(self)->open ) {
    if (mapdata) CUR_MAP(self)= (P_Renderer_Cmap *)mapdata;
    else ger_error("gl_ren_mthd: install_cmap: got null color map data.");
  }
  METHOD_OUT
}

static void destroy_cmap(P_Void_ptr mapdata) {
  /* This has to be implemented without reference to "self", like a
   * class static method, because it may be called to destroy data
   * after the specific renderer which created it has been destroyed.
   */
  ger_debug("gl_ren_mthd: destroy_cmap");
  if ( mapdata ) free( mapdata );
}

static P_Void_ptr def_gob(char *name, struct P_Gob_struct *gob) {
    
  P_Renderer *self = (P_Renderer *)po_this;

  METHOD_IN
	
  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: def_gob");
    METHOD_OUT
    return( (P_Void_ptr)gob ); /* Stash the gob where we can get it later */
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static char* getTrimmedValue(char* string)
{
  /* This returns the value of a pair like 'name="foo"', using the same storage as
   * the input string.  Surrounding quotes or double quotes are removed if present.
   */
  char* result= strchr(string,'=');
  char* end;

  if (!result) return NULL;

  result++; /* skip to following char */
  if (*result == '\'' || *result == '"') result++;
  end= result;
  while (*end) end++;
  if ((end>result) && (*(end-1)=='\'' || *(end-1)=='"')) end--;
  *end= '\0';
  return result;
}

P_Renderer *po_create_gl_renderer( char *device, char *datastr )
     /* This routine creates a ptr-generating renderer object */
{
  P_Renderer *self;
  int length,i;
  P_Renderer_data *rdata;
  long xsize, ysize;
  register short lupe;
  int x,y,result; /*For prefposition*/
  unsigned int width, height; /*For prefsize*/
  char *name, *size;
  int nprocs_set= 0, rank_set= 0;
  
  ger_debug("po_create_gl_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_gl_renderer: unable to allocate %d bytes!",
	      sizeof(P_Renderer) );
#ifdef USE_OPENGL
  sprintf(self->name,"ogl%d",ren_seq_num++);
#else
  sprintf(self->name,"igl%d",ren_seq_num++);
#endif

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_gl_renderer: unable to allocate %d bytes!",
	      sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;

  /* Set some flags */
  if (strstr (device,"nomanage") != NULL) {
    MANAGE(self)= 0;
  }
  else MANAGE(self)= 1;
  if (strstr (device, "widget=") != NULL) {
    AUTO (self) = 1;
  }
  else AUTO(self)= 0;

  /*parse the datastr string*/
  name= strdup("p3d-gl");
  size = NULL;
  NPROCS(self)= 0;
  RANK(self)= 0;
  
  if (datastr) {
    char* size_start= NULL;
    char* name_start= NULL;
    char* nprocs_start= NULL;
    char* rank_start= NULL;
    char* thisTok;
    char* where;
    char* dupDatastr= strdup(datastr);
    int firstPass= 1;
    while ( thisTok= strtok_r( (firstPass ? dupDatastr : NULL), ",", &where ) ) {
      if (!strncasecmp(thisTok,"name=",strlen("name="))
	  || !strncasecmp(thisTok,"title=",strlen("title="))) {
	if (name) free(name);
	name= strdup( getTrimmedValue(thisTok) );
      }
      else if (!strncasecmp(thisTok,"size=",strlen("size="))
	       || !strncasecmp(thisTok,"geometry=",strlen("geometry="))) {
	size= strdup(getTrimmedValue(thisTok));
      }
      else if (!strncasecmp(thisTok,"nprocs=",strlen("nprocs="))) {
	NPROCS(self)= atoi(getTrimmedValue(thisTok));
	nprocs_set= 1;
      }
      else if (!strncasecmp(thisTok,"rank=",strlen("rank="))) {
	RANK(self)= atoi(getTrimmedValue(thisTok));
	rank_set= 1;
      }
      else ger_error("po_create_gl_renderer: unrecognized data string element <%s>!\n",thisTok);
      firstPass= 0;
    }
    free(dupDatastr);
  }
  
  NAME(self) = name;
  RENDATA(self)->initialized= 0;
  init_gl_structure(self);
  
  if (nprocs_set && !rank_set) 
    ger_fatal("po_create_gl_renderer: if nprocs is specified, rank must be also.\n");
  if (rank_set && !nprocs_set) 
    ger_fatal("po_create_gl_renderer: if rank is specified, nprocs must be also.\n");

  /*start by parsing the size into a geometry spec...*/
  
  if (MANAGE(self)) {
    if (AUTO (self)) {
      char *ptr;
      
      device = strstr (device, "widget=");
      ptr = strchr (device, '=');
      WIDGET(self) = (Widget) atoi (ptr + 1);
      XDISPLAY(self)= XtDisplay(WIDGET(self));
      XWINDOW(self)= XtWindow(WIDGET(self)); /* we will actually use a subwindow */
#ifdef never
      XSynchronize(XDISPLAY(self),True);
#endif
      attach_drawing_window(self);
    }
    else {
      create_drawing_window(self, size);
    }
    set_drawing_window(self);
  }

  if (size) free(size);

  return (self);
}

/* complete structure initalization (note: no actual gl calls here) */
static void init_gl_structure (P_Renderer *self)
{
  register short lupe;

#ifdef USE_OPENGL

  /* result of chromium_in_use() depends on the following call */
  if (!glCreateContextCR) LOAD( CreateContext );

  if (!glDestroyContextCR) LOAD( DestroyContext );
  if (!glDestroyContextCR) glDestroyContextCR= dummyDestroyContext;;
  if (!glMakeCurrentCR) LOAD( MakeCurrent );
  if (!glMakeCurrentCR) glMakeCurrentCR= dummyMakeCurrent;
  if (!glSwapBuffersCR) LOAD( SwapBuffers );
  if (!glSwapBuffersCR) glSwapBuffersCR= dummySwapBuffers;

  SPHERE(self)= NULL;
  SPHERE_DEFINED(self)= 0;
  CYLINDER(self)= NULL;
  CYLINDER_DEFINED(self)= 0;
  for (lupe=0; lupe<MY_GL_MAX_LIGHTS; lupe++)
    LIGHT_IN_USE(self)[lupe]= 0;
  if (NPROCS(self)>1) {
    BARRIER(self)= BARRIER_BASE+ren_seq_num;
  }
  else {
    BARRIER(self)= 0;
  }
#endif

  BACKGROUND(self) = (float *)(malloc(4*sizeof(float)));
  AMBIENTCOLOR(self) = (float *)(malloc(4*sizeof(float)));
  ASSIST(self)= po_create_assist(self);
  BACKCULLSYMBOL(self)= create_symbol("backcull");
  TEXTHEIGHTSYMBOL(self)= create_symbol("text-height");
  COLORSYMBOL(self)= create_symbol("color");
  MATERIALSYMBOL(self)= create_symbol("material");
  CURMATERIAL(self) = -1;
#ifdef USE_OPENGL
  MAXDLIGHTCOUNT(self) = 0; /* not used under OpenGL */
  DLIGHTCOUNT(self) = 0; /* not used under OpenGL */
#else
  MAXDLIGHTCOUNT(self) = MAXLIGHTS; /* not used under OpenGL */
  DLIGHTCOUNT(self) = LIGHT0; /* not used under OpenGL */
#endif
  DLIGHTBUFFER(self) = NULL;

  /* Fill in all the methods */
  self->def_sphere= def_sphere;
  self->ren_sphere= ren_sphere;
  self->destroy_sphere= destroy_sphere;

  self->def_cylinder= def_cylinder;
  self->ren_cylinder= ren_cylinder;
  self->destroy_cylinder= destroy_cylinder;

  self->def_torus= def_torus;
  self->ren_torus= ren_torus;
  self->destroy_torus= destroy_torus;

  self->def_polymarker= def_polymarker;
  self->ren_polymarker= ren_polymarker;
  self->destroy_polymarker= destroy_object;

  self->def_polyline= def_polyline;
  self->ren_polyline= ren_polyline;
  self->destroy_polyline= destroy_object;

  self->def_polygon= def_polygon;
  self->ren_polygon= ren_polygon;
  self->destroy_polygon= destroy_object;

  self->def_tristrip= def_tristrip;
  self->ren_tristrip= ren_tristrip;
  self->destroy_tristrip= destroy_object;

  self->def_mesh= def_mesh;
  self->ren_mesh= ren_mesh;
  self->destroy_mesh= destroy_mesh;

  self->def_bezier= def_bezier;
  self->ren_bezier= ren_bezier;
  self->destroy_bezier= destroy_bezier;

  self->def_text= def_text;
  self->ren_text= ren_text;
  self->destroy_text= destroy_text;

  self->def_light= def_light;
  self->ren_light= ren_light;
  self->light_traverse_light= traverse_light;
  self->destroy_light= destroy_light;

  self->def_ambient= def_ambient;
  self->ren_ambient= ren_ambient;
  self->light_traverse_ambient= traverse_ambient;
  self->destroy_ambient= destroy_ambient;

  self->def_gob= def_gob;
  self->ren_gob= ren_gob;
  self->light_traverse_gob= traverse_gob;
  self->hold_gob= hold_gob;
  self->unhold_gob= unhold_gob;
  self->destroy_gob= destroy_gob;

  self->print= ren_print;
  self->open= ren_open;
  self->close= ren_close;
  self->destroy_self= ren_destroy;

  self->def_camera= def_camera;
  self->set_camera= set_camera;
  self->destroy_camera= destroy_camera;

  self->def_cmap= def_cmap;
  self->install_cmap= install_cmap;
  self->destroy_cmap= destroy_cmap;
}

/* gl calls necessary to complete drawp3d initalization
     after the widget is realized or the window is up
*/
static void init_gl_gl (P_Renderer *self)
{
  unsigned int xsize, ysize;
  int xcorner, ycorner;

#ifdef USE_OPENGL
  glMatrixMode(GL_MODELVIEW);

  glClearColor(0.0,0.0,0.0,0.0); 

#if defined(WIREGL)
  if (NPROCS(self)>1) glBarrierExec(BARRIER(self));

  if (MANAGE(self) && (NPROCS(self)<2 || RANK(self)==0))
    wireGLSwapBuffers();
#ifdef WIREGL_INSTRUMENT
  else if (MANAGE(self) && (NPROCS(self)>1 || RANK(self)!=0))
    wireGLInstrumentNextFrame();
#endif
#endif
  if (chromium_in_use()) {
    if (NPROCS(self)>1) glBarrierExecCR( BARRIER(self) );
    if (MANAGE(self)) glSwapBuffersCR(0, 0);
  }
  else {
    if (MANAGE(self)) glXSwapBuffers(XDISPLAY(self), XWINDOW(self));
  }
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHTING);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE);

  hastransparent= 0;

#else
  mmode (MVIEWING);
  if (MANAGE(self) && !AUTO(self)) {
    doublebuffer ();
    RGBmode ();
    gconfig();
  }
  subpixel (TRUE);
  cpack (0);
  clear ();
  if (MANAGE(self)) swapbuffers ();
  zbuffer (TRUE);
  lmcolor (LMC_COLOR);

  glcompat (GLC_ZRANGEMAP, 0);
  glcompat (GLC_OLDPOLYGON, 1);

  lsetdepth (getgdesc (GD_ZMAX), getgdesc (GD_ZMIN));
  zfunction (ZF_GREATER);
  shademodel (GOURAUD);

  if (hastransparent = getgdesc (GD_BLEND))
    blendfunction (BF_SA, BF_MSA);
  else
    defpattern (GREYPATTERN, 16, greyPattern);

#ifndef AVOID_NURBS
  sphmode (SPH_TESS, SPH_BILIN);
  sphmode (SPH_DEPTH, 4);
#endif

  LM(self) = (float *)(malloc(9*sizeof(float))); /* not used under OpenGL */
  for (lupe = 0; lupe < 9; lupe++)
    LM(self)[lupe] = initiallm[lupe];

#endif

  get_drawing_area(self,&xcorner,&ycorner,&xsize,&ysize);
  ASPECT (self) = (float) (xsize - 1) / (float) (ysize - 1);

  /*Predefine all of our materials...*/
  define_materials(self);
}

  
