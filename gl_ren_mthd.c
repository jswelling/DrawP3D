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
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"
#include <X11/Intrinsic.h>
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#ifdef USE_OPENGL
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/GLwDrawA.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#else
#include <gl/glws.h>
#include <gl/gl.h>
#include <X11/Xirisw/GlxMDraw.h>
#endif

#ifndef USE_OPENGL
#define Object GL_Object
#include <gl/sphere.h>
#undef Object
#endif

#include "gl_strct.h"

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
static void new_widget_cb (Widget w, P_Renderer *self, XtPointer call_data);
static void gl_resize_cb(Widget w, P_Renderer *self, XtPointer call_data);
static P_Renderer *init_gl_normal (P_Renderer *self);
static void init_gl_widget (P_Renderer *self); 

static void get_drawing_area_size( P_Renderer* self, long* x, long* y )
{
  /* Assumes set_drawing_window has been called */
#ifdef USE_OPENGL
  Arg args[10];
  int n= 0;
  Dimension height;
  Dimension width;
  XtSetArg(args[n],XtNwidth,&width); n++;
  XtSetArg(args[n],XtNheight,&height); n++;
  XtGetValues(WIDGET(self),args,n);
  *x= width;
  *y= height;

#else
  getsize(x,y);
#endif
}

static void set_drawing_window( P_Renderer* self )
{
#ifdef USE_OPENGL
  GLwDrawingAreaMakeCurrent(WIDGET(self), GLXCONTEXT(self));
#else
  if (!AUTO (self)) {
    winset(WINDOW(self));
  }
  else {
    GLXwinset (XtDisplay (WIDGET (self)), XtWindow (WIDGET (self)));
  }
#endif
}

static void attach_drawing_window( P_Renderer* self )
{
  /* AUTO(self) is always true in here */
  if (XtIsRealized (WIDGET (self))) {
#ifdef USE_OPENGL
    XVisualInfo *vi;
    Arg args[1];
    XtSetArg(args[0], GLwNvisualInfo, &vi);
    XtGetValues(WIDGET(self), args, 1);
    GLXCONTEXT(self)= 
      glXCreateContext(XtDisplay(WIDGET(self)), vi, 0, GL_FALSE);
    if (!GLXCONTEXT(self)) {
      fprintf(stderr,"gl_ren_mthd: attach_drawing_widget: glXCreateContext failed on widget %x; is it a glX widget?\n",(int)(WIDGET(self)));
    }
    XtAddCallback(WIDGET(self), GLwNresizeCallback, 
		  (XtCallbackProc)gl_resize_cb, (XtPointer)self);
#else
    XtAddCallback (WIDGET (self), GlxNresizeCallback,
		   (XtCallbackProc) gl_resize_cb, (XtPointer) self);
#endif
    set_drawing_window(self);
    init_gl_widget(self);
    RENDATA(self)->initialized= 1;
  }
  else {
    ger_fatal("Error: Cannot initialize DrawP3D gl with an unrealized widget!\n");
    /* give up now;  calling sequence error */
  }
}

static void create_drawing_window( P_Renderer* self, char* size_info )
{
  /* AUTO(self) is always false in here */
#ifdef USE_OPENGL
  {
    Display *dpy;
    Arg args[10];
    int n;
    static char* fake_argv[]= {"drawp3d",NULL};
    int fake_argc= 1;
    Widget shell; /* Parent shell */
    Widget glw;   /* The GLwDrawingArea widget */

    n= 0;
    XtSetArg(args[n], XtNgeometry, 
	     (size_info) ? size_info : DEFAULT_WINDOW_GEOMETRY); n++;
    shell= XtOpenApplication(&APPCONTEXT(self), "DRawp3d",
			     NULL, 0, &fake_argc, fake_argv, NULL,
			     applicationShellWidgetClass, args, n);
    n = 0;
    XtSetArg(args[n], GLwNrgba, TRUE); n++;
    XtSetArg(args[n], GLwNdoublebuffer, TRUE); n++;
    XtSetArg(args[n], GLwNdepthSize, 16); n++;
    glw= XtCreateManagedWidget("glxdrawingarea", glwDrawingAreaWidgetClass, 
			       shell, args, n);
    if (!glw) {
      ger_fatal("create_drawing_window: Unable to draw to your display!\n");
    }

    WIDGET(self)= glw;

    XtAddCallback(glw, GLwNresizeCallback, 
		  (XtCallbackProc)gl_resize_cb, (XtPointer)self);
    XtAddCallback(glw, GLwNginitCallback, (XtCallbackProc)new_widget_cb, 
		  (XtPointer)self);

    XtManageChild(glw);
    XtRealizeWidget(shell);

    /* Cleverly stall until initialization is complete */
    while (!RENDATA(self)->initialized) {
      XEvent event;
      if (XtAppPending(APPCONTEXT(self))) { 
	XtAppNextEvent(APPCONTEXT(self), &event);
	XtDispatchEvent(&event);
      }
    }
    glXWaitX();
    glXWaitGL();

    /* OK, the window should be up. */
  }
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
    set_drawing_window(self);
    init_gl_widget(self);
  }
#endif
}

static void release_drawing_window( P_Renderer* self )
{
  /* Destroys the window if appropriate */
#ifdef USE_OPENGL
  if (AUTO(self)) {
    glXDestroyContext(XtDisplay(WIDGET(self)),GLXCONTEXT(self));
    /* the rest is owned by the calling app */
  }
  else {
    glXDestroyContext(XtDisplay(WIDGET(self)),GLXCONTEXT(self));
    XtDestroyWidget(XtParent(WIDGET(self))); /* gets WIDGET(self) too */
    XtDestroyApplicationContext(APPCONTEXT(self));
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
    set_drawing_window(self);
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
  set_drawing_window(self);
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
      fprintf(stderr,"pvm_ren_mthd: cannot allocate %d bytes!\n",
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
#else
      if (it->obj_info.obj && isobj(it->obj_info.obj)) {
	delobj(it->obj_info.obj);	
#endif
      }
      if (it->cvlist) free_cached_vlist( it->cvlist );
      free((void *)it);
    }
    METHOD_OUT
}

static void destroy_sphere(P_Void_ptr the_thing) {
    /*Destroy a sphere.*/
    
    gl_gob *it = (gl_gob *)the_thing;
    P_Renderer *self = (P_Renderer *)po_this;

    METHOD_IN

    ger_debug("gl_ren_mthd: destroy_sphere\n");

#ifdef USE_OPENGL
    if (it == SPHERE(self)) {
	METHOD_OUT
	return;
    }
#else
    if (it == my_sphere) {
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
	set_drawing_window(self);
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
    (*(ASSIST(self)->ren_torus))(the_thing, transform, attrs);
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
      return((P_Void_ptr)SPHERE(self));
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
    SPHERE(self)= result= (*(ASSIST(self)->def_sphere))();
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
	top_level_call = 1;

	set_drawing_window(self);

#ifdef USE_OPENGL
	if (!AUTO(self)) {
	  long xsize, ysize;
	  int event_pending= 1;
	  /* run a baby event loop */
	  while (event_pending) {
	    XEvent event;
	    if (XtAppPending(APPCONTEXT(self))) { 
	      event_pending= 1;
	      XtAppNextEvent(APPCONTEXT(self), &event);
	      XtDispatchEvent(&event);
	    }
	    else event_pending= 0;
	  }
	}
	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_LIST_BIT );
	GLPROF("Clearing");
	glClearColor(BACKGROUND(self)[0], BACKGROUND(self)[1], 
		     BACKGROUND(self)[2], BACKGROUND(self)[3]);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	gluLookAt(LOOKFROM(self).x,LOOKFROM(self).y,LOOKFROM(self).z,
		  LOOKAT(self).x, LOOKAT(self).y, LOOKAT(self).z, 
		  LOOKUP(self).x, LOOKUP(self).y, LOOKUP(self).z);
#else
	if (!AUTO(self)) {
	  long xsize, ysize;
	  get_drawing_area_size(self,&xsize,&ysize);
	  ASPECT (self) = (float) (xsize - 1) / (float) (ysize - 1);
	  viewport (0, xsize + 1, 0, ysize + 1);
	}
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
	glPopAttrib();
	glXSwapBuffers(XtDisplay(WIDGET(self)),XtWindow(WIDGET(self)));
	glFlush();
#else
	popmatrix();
	swapbuffers();
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

	set_drawing_window(self);

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
	ger_fatal("def_polyline: unable to allocate %d bytes!", sizeof(gl_gob));
    
#ifdef USE_OPENGL
    it->color_mode= 0;
#else
    it->color_mode= LMC_COLOR;
#endif
    it->obj_info.obj= NULL;
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
#else
    it->color_mode= LMC_COLOR;
#endif
    it->obj_info.obj= NULL;
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
#else
    it->color_mode= LMC_COLOR;
#endif
    it->obj_info.obj= NULL;
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
#else
    it->color_mode= LMC_COLOR;
#endif
    it->obj_info.obj= NULL;
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
    
#ifdef USE_OPENGL
    set_drawing_window(self);
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
    
    METHOD_OUT
    return( (P_Void_ptr)it );
}

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
      GLfloat cvals[4];
      GLfloat xvals[4];
      GLuint lightlist;
      cvals[0]= color->r;
      cvals[1]= color->g;
      cvals[2]= color->b;
      cvals[3]= color->a;
      xvals[0]= point->x;
      xvals[1]= point->y;
      xvals[2]= point->z;
      xvals[3]= 0.0;

      set_drawing_window(self);
      glNewList( lightlist=glGenLists(1) , GL_COMPILE );
      glLightfv( LIGHT_NUM(self), GL_DIFFUSE, cvals );
      glLightfv( LIGHT_NUM(self), GL_SPECULAR, cvals );
      glLightfv( LIGHT_NUM(self), GL_POSITION, xvals );
      glEnable( LIGHT_NUM(self) );
      glEndList();

      /* Increment light number, wrapping if necessary */
      LIGHT_NUM(self) = LIGHT_NUM(self) + 1;
      if (LIGHT_NUM(self) >= GL_LIGHT0 + MY_GL_MAX_LIGHTS) {
	ger_error("gl_ren_mthd: Warning, recycling light numbers!\n");
	LIGHT_NUM(self)= GL_LIGHT0;
      }

      METHOD_OUT
      return((P_Void_ptr)lightlist);
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
    short the_light = (short)it;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }

    ger_debug("gl_ren_mthd: traverse_light");
    
#ifdef USE_OPENGL
    /* Turn on the given light */
    glCallList((GLuint)it);
#else
    /*Okay... let's figure out how many lights are in use...
     and quit if we have too many...*/
    if ((LIGHT0 + MAXLIGHTS) == DLIGHTCOUNT(self))
	return;

    lmbind(DLIGHTCOUNT(self)++, the_light);
#endif
    
    METHOD_OUT
}

void ren_light(P_Void_ptr it, P_Transform *foo,
	       P_Attrib_List *bar) {

    /*Hmm... let's just do nothing, for now...*/
}

void destroy_light(P_Void_ptr it) {
    
    P_Renderer *self = (P_Renderer *)po_this;
    short the_light = (short)it;
    METHOD_IN
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
	
    ger_debug("gl_ren_mthd: destroy_light\n");

#ifdef USE_OPENGL
    glDeleteLists((GLuint)it,1);
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
      GLfloat cvals[4];
      GLuint lightlist;
      cvals[0]= color->r;
      cvals[1]= color->g;
      cvals[2]= color->b;
      cvals[3]= color->a;

      set_drawing_window(self);
      glNewList( lightlist=glGenLists(1) , GL_COMPILE );
      glLightfv( LIGHT_NUM(self), GL_AMBIENT, cvals );
      glEnable( LIGHT_NUM(self) );
      glEndList();

      /* Increment light number, wrapping if necessary */
      LIGHT_NUM(self)= LIGHT_NUM(self) + 1;
      if (LIGHT_NUM(self) >= GL_LIGHT0 + MY_GL_MAX_LIGHTS) {
	ger_error("gl_ren_mthd: Warning, recycling light numbers!\n");
	LIGHT_NUM(self)= GL_LIGHT0;
      }

      METHOD_OUT
      return((P_Void_ptr)lightlist);
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

    /* Turn on the given light */
    glCallList((GLuint)it);

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
    glDeleteLists((GLuint)it,1);
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
    
    set_drawing_window(self);

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

    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
	
    ger_debug("gl_ren_mthd: destroy_camera\n");

    /*I don't have anything to deallocate... :)*/
    
    METHOD_OUT
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

    release_drawing_window(self);

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_self))();
    
    free( (P_Void_ptr)BACKGROUND(self) );
    free( (P_Void_ptr)AMBIENTCOLOR(self) );
#ifndef USE_OPENGL
    free( (P_Void_ptr)LM(self) );
#endif
    free ((P_Void_ptr)self);
    METHOD_DESTROYED
}

static P_Void_ptr def_cmap(char *name, double min, double max, void(*mapfun)(float *, float *, float *, float *, float *)) {
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
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_cmap");
  if ( RENDATA(self)->open ) free( mapdata );
  METHOD_OUT
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

P_Renderer *po_create_gl_renderer( char *device, char *datastr )
/* This routine creates a ptr-generating renderer object */
{
  P_Renderer *self;
  int length,i;
  P_Renderer_data *rdata;
  long xsize, ysize;
  register short lupe;
  register char *where;
  int x,y,result; /*For prefposition*/
  unsigned int width, height; /*For prefsize*/
  char *name, *size;
  
  ger_debug("po_create_gl_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_gl_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );
#ifdef USE_OPENGL
  sprintf(self->name,"ogl%d",ren_seq_num++);
#else
  sprintf(self->name,"ogl%d",ren_seq_num++);
#endif

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_gl_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;

  if (strstr (device, "widget=") != NULL) {
    AUTO (self) = 1;
  }
  else AUTO(self)= 0;
  
  /*parse the datastr string*/
  /*At present, because we only take "name" and "size" arguments,
    we're just looking forward for a "n" or a "s", searching forward for the 
    ", getting the data up to the next ", and repeating.
    We'll also support "p" for "position" and "g" for "geometry"
   in place of "s", and "t" for "title" in place of "n".
   */

  name = "p3d-gl";
  size = NULL;
  
  if (datastr) {
      where = (char *)malloc(strlen(datastr)+1);
      strcpy(where, datastr);
      for (;*where != '\0';where++)
	  switch (*where) {
	  case 's':
	  case 'S':
	  case 'g':
	  case 'G':
	  case 'p':
	  case 'P':
	      for (;(*where != '"') && (*where != '\0');where++);
	      if (*where != '\0') {
		  size = ++where;
		  for (;(*where != '"') && (*where != '\0');where++);
		  if (*where == '"')
		      *where='\0';
		  else
		      where--;
	      }
	      break;
	  case 'n':
	  case 'N':
	  case 't':
	  case 'T':
	      for (;(*where != '"') && (*where != '\0');where++);
	      if (*where != '\0') {
		  name = ++where;
		  for (;(*where != '"') && (*where != '\0');where++);
		  if (*where == '"')
		      *where='\0';
		  else
		      where--;
	      }
	      break;
	  default:
	      break;
	  }
  }
  
  NAME(self) = name;
  RENDATA(self)->initialized= 0;
  
  /*start by parsing the size into a geometry spec...*/
  
  if (AUTO (self)) {
    char *ptr;
    
    device = strstr (device, "widget=");
    ptr = strchr (device, '=');
    WIDGET(self) = (Widget) atoi (ptr + 1);
    attach_drawing_window(self);
  }
  else {
    create_drawing_window(self, size);
  }
  
  return (init_gl_normal (self));
}

/* complete gl initalization (note: no actual gl calls here) */
P_Renderer *init_gl_normal (P_Renderer *self)
{
     register short lupe;

#ifdef USE_OPENGL
     SPHERE(self)= NULL;
     SPHERE_DEFINED(self)= 0;
     CYLINDER(self)= NULL;
     CYLINDER_DEFINED(self)= 0;
     LIGHT_NUM(self)= GL_LIGHT0;
#endif

     BACKGROUND(self) = (float *)(malloc(4*sizeof(float)));
     AMBIENTCOLOR(self) = (float *)(malloc(4*sizeof(float)));
     ASSIST(self)= po_create_assist(self);
     BACKCULLSYMBOL(self)= create_symbol("backcull");
     TEXTHEIGHTSYMBOL(self)= create_symbol("text-height");
     COLORSYMBOL(self)= create_symbol("color");
     MATERIALSYMBOL(self)= create_symbol("material");
     CURMATERIAL(self) = -1;
     MAXDLIGHTCOUNT(self) = MAXLIGHTS; /* not used under OpenGL */
     DLIGHTCOUNT(self) = LIGHT0; /* not used under OpenGL */
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
     self->ren_mesh= ren_object;
     self->destroy_mesh= destroy_object;

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

     return (self);
}


static void new_widget_cb (Widget w, P_Renderer *self, XtPointer call_data)
{
  if (!RENDATA (self)->initialized) {
#ifdef USE_OPENGL
    XVisualInfo *vi;
    Arg args[1];
    XtSetArg(args[0], GLwNvisualInfo, &vi);
    XtGetValues(w, args, 1);
    GLXCONTEXT(self)= glXCreateContext(XtDisplay(w), vi, 0, GL_FALSE);
#endif
    set_drawing_window(self);
    init_gl_widget (self);
    RENDATA (self)->initialized = 1;
  }
}

static void gl_resize_cb (Widget w, P_Renderer *self, XtPointer call_data)
{
#ifdef USE_OPENGL
     long xsize, ysize;
     GLwDrawingAreaCallbackStruct *glx = 
       (GLwDrawingAreaCallbackStruct *) call_data;

     set_drawing_window(self);

     get_drawing_area_size(self,&xsize,&ysize);
     ASPECT (self) = (float) (xsize - 1) / (float) (ysize - 1);
     glViewport(0,  0, ( glx->width + 1)-(0)+1, 
		( glx->height + 1)-( 0)+1); 
     glScissor(0,  0, ( glx->width + 1)-(0)+1, ( glx->height + 1)-( 0)+1);
#else
     long xsize, ysize;
     GlxDrawCallbackStruct *glx = (GlxDrawCallbackStruct *) call_data;

     set_drawing_window(self);

     getsize (&xsize, &ysize);
     ASPECT (self) = (float) (xsize - 1) / (float) (ysize - 1);
     viewport (0, glx->width + 1, 0, glx->height + 1);
#endif
}

/* gl calls necessary to complete drawp3d initalization
     after the widget is realized or the window is up
*/
void init_gl_widget (P_Renderer *self)
{
     register short lupe;
     long xsize, ysize;

     RENDATA (self)->initialized = 1;
#ifdef USE_OPENGL
     glMatrixMode(GL_MODELVIEW);

     glClearColor(0.0,0.0,0.0,0.0); 
     glClear(GL_COLOR_BUFFER_BIT);

     glXSwapBuffers(XtDisplay(WIDGET(self)), XtWindow(WIDGET(self)));
     glEnable(GL_DEPTH_TEST);
     glShadeModel(GL_SMOOTH);
     glEnable(GL_LIGHTING);
     glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
     glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glEnable(GL_COLOR_MATERIAL);
     glEnable(GL_AUTO_NORMAL);
     glEnable(GL_NORMALIZE);

     hastransparent= 0;
#ifdef never
     if (hastransparent = (glGetIntegerv(GL_BLEND, &gdtmp), gdtmp)) {
       glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA); 
       if((GL_SRC_ALPHA) == GL_ONE 
	  && ( GL_ONE_MINUS_SRC_ALPHA) == GL_ZERO) 
	 glDisable(GL_BLEND) else glEnable(GL_BLEND);
     }
     else {
       /* screen_door_transp() does what's needed */
     }
#endif

#else
     mmode (MVIEWING);
     doublebuffer ();
     RGBmode ();
     gconfig();
     subpixel (TRUE);
     cpack (0);
     clear ();
     swapbuffers ();
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

     get_drawing_area_size(self,&xsize,&ysize);
     ASPECT (self)= (float) xsize / (float) ysize;

     /*Predefine all of our materials...*/
     define_materials(self);
}

