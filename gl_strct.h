/****************************************************************************
 * gl_strct.c
 * Author Robert Earhart
 * Copyright 1991, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides the data structures for the IRIS gl renderer
*/

#define MAXFILENAME 128
#define MAXSYMBOLLENGTH P3D_NAMELENGTH

#ifdef USE_OPENGL
/* I've encountered problems using more than 8 lights, even though
 * there are supposed to be GL_MAX_LIGHTS of them.
 */
#define MY_GL_MAX_LIGHTS 8
#else
#include<gl/gl.h>
#endif

/* mimic behavior of gl.h in getting around conflict with X def of "Object"
 */
#ifdef _XtObject_h
#ifndef USE_OPENGL
#define Object GL_Object
#define OBJECT_HACK_SET
#endif
#endif

typedef long color_mode_type; /*lmcolor wants a long...*/

/* Struct to hold material info */
typedef struct gl_material_struct {
  float ambient;
  float diffuse;
  float specular;
  float shininess;
#ifdef USE_OPENGL
  GLuint obj;
#else
  int index;
#endif
} gl_material;

/* Struct to hold info for a cached vertex list */
typedef struct vlist_cache_struct {
  int type;
  int length;
  float *coords;
  float *colors;
  float *normals;
} P_Cached_Vlist;

typedef enum { MESH_MIXED, MESH_TRI, MESH_QUAD, MESH_STRIP } gl_mesh_type;

typedef struct gl_gob_struct {
#ifdef USE_OPENGL
    union {
      GLuint obj;
      GLUquadricObj* quad_obj;
      struct {
	GLfloat* data;
	GLUnurbsObj* ren;
	int flags;
      } nurbs_obj;
      struct {
	int* indices;
	int* facet_lengths;
	int nfacets;
	gl_mesh_type type;
      } mesh_obj;
    } obj_info;
#else
    union {
      Object obj;  /*Holds the identifier for this object*/
      double* nurbs_obj;
    } obj_info;
#endif
    P_Cached_Vlist* cvlist;
    color_mode_type color_mode; /*need to keep color mode...*/ 
} gl_gob, *gl_gob_ptr;

/* Struct to hold info for a color map */
typedef struct renderer_cmap_struct {
  char name[MAXSYMBOLLENGTH];
  double min;
  double max;
  void (*mapfun)( float *, float *, float *, float *, float * );
} P_Renderer_Cmap;

typedef enum boolean {false=0, true} bool;

/* Struct for object data, and access functions for it */
typedef struct renderer_data_struct {

  void *widget;
  int w_auto;
  int manage;

#ifdef USE_OPENGL
  GLXContext ctx;
  GLint cr_ctx; /* May be used by Chromium */
  Window win;
  Display *dpy;
  P_Void_ptr sphere;
  int sphere_defined;
  gl_gob* cylinder;
  int cylinder_defined;
  int light_in_use[MY_GL_MAX_LIGHTS];
  GLuint barrier; /* used in parallel geometry stream management */
#endif

  int open;
  int initialized;
  char *outfile;
  char *name;
  short DLightCount;
  short MaxDLightCount;
  short lookangle;
  short CurMaterial;
  float *Background;
  float *AmbientColor;
  float *lm; /* not used under OpenGL */
  float aspect;
  long window;
  int nprocs; /* number of partner renderers, typically for MPI applications */
  int rank;   /* rank within nprocs, typically for MPI applications */

  P_Gob *DLightBuffer;
  P_Point lookat;
  P_Point lookfrom;
  P_Vector lookup;
  P_Renderer_Cmap *current_cmap;
  P_Symbol backcull_symbol;     /* backcull symbol */
  P_Symbol text_height_symbol;  /* text height symbol */
  P_Symbol color_symbol;        /* color symbol */
  P_Symbol material_symbol;     /* material symbol */
  P_Assist *assist;             /* renderer assist object */
} P_Renderer_data;

#define AUTO( self ) (RENDATA(self)->w_auto)
#define MANAGE( self ) (RENDATA(self)->manage)
#define WIDGET( self ) (RENDATA(self)->widget)
#ifdef USE_OPENGL
#define GLXCONTEXT( self ) (RENDATA(self)->ctx)
#define CRCONTEXT( self ) (RENDATA(self)->cr_ctx)
#define XWINDOW( self ) (RENDATA(self)->win)
#define XDISPLAY( self ) (RENDATA(self)->dpy)
#define SPHERE(self) (RENDATA(self)->sphere)
#define SPHERE_DEFINED(self) (RENDATA(self)->sphere_defined)
#define CYLINDER(self) (RENDATA(self)->cylinder)
#define CYLINDER_DEFINED(self) (RENDATA(self)->cylinder_defined)
#define LIGHT_IN_USE(self) (RENDATA(self)->light_in_use)
#define BARRIER(self) (RENDATA(self)->barrier)
#endif
#define RENDATA( self ) ((P_Renderer_data *)(self->object_data))
#define OUTFILE( self ) (RENDATA(self)->outfile)
#define LM( self ) (RENDATA(self)->lm) /* not used under OpenGL */
#define LOOKAT( self ) (RENDATA(self)->lookat)
#define LOOKFROM( self ) (RENDATA(self)->lookfrom)
#define LOOKANGLE( self ) (RENDATA(self)->lookangle)
#define LOOKUP( self ) (RENDATA(self)->lookup)
#define WINDOW( self ) (RENDATA(self)->window)
#define NAME( self ) (RENDATA(self)->name)
#define CUR_MAP( self ) (RENDATA(self)->current_cmap)
#define MAP_NAME( self ) (CUR_MAP(self)->map_name)
#define MAP_MIN( self ) (CUR_MAP(self)->min)
#define MAP_MAX( self ) (CUR_MAP(self)->max)
#define MAP_FUN( self ) (CUR_MAP(self)->mapfun)
#define BACKGROUND( self ) (RENDATA(self)->Background)
#define CURMATERIAL( self ) (RENDATA(self)->CurMaterial)
#define ASPECT( self ) (RENDATA(self)->aspect)
#define RECENTTRANS( self ) (RENDATA(self)->RecentTrans)
#define DEPTHBUFFER( self ) (RENDATA(self)->DepthBuffer)
#define DEPTHCOUNT( self ) (RENDATA(self)->DepthCount)
#define MAXPOLYCOUNT( self ) (RENDATA(self)->MaxPolyCount)
#define DPOLYBUFFER( self ) (RENDATA(self)->DPolyBuffer)
#define MAXDEPTHPOLY( self ) (RENDATA(self)->MaxDepthPoly)
#define DEPTHPOLYCOUNT( self ) (RENDATA(self)->DepthPolyCount)
#define DCOLORBUFFER( self ) (RENDATA(self)->DColorBuffer)
#define DCOLORCOUNT( self ) (RENDATA(self)->DColorCount)
#define MAXDCOLORCOUNT( self ) (RENDATA(self)->MaxDColorCount)
#define DCOORDBUFFER( self ) (RENDATA(self)->DCoordBuffer)
#define DCOORDINDEX( self ) (RENDATA(self)->DCoordIndex)
#define MAXDCOORDINDEX( self ) (RENDATA(self)->MaxDCoordIndex)
#define DLIGHTBUFFER( self ) (RENDATA(self)->DLightBuffer)
#define DLIGHTCOUNT( self ) (RENDATA(self)->DLightCount)
#define MAXDLIGHTCOUNT( self ) (RENDATA(self)->MaxDLightCount)
#define AMBIENTCOLOR( self ) (RENDATA(self)->AmbientColor)
#define TEMPCOORDBUFFSZ( self ) (RENDATA(self)->TempCoordBuffSz)
#define VIEWMATRIX( self ) (RENDATA(self)->ViewMatrix)
#define EYEMATRIX( self ) (RENDATA(self)->EyeMatrix)
#define ZMAX( self ) (RENDATA(self)->Zmax)
#define ZMIN( self ) (RENDATA(self)->Zmin)
#define BACKCULLSYMBOL(self) (RENDATA(self)->backcull_symbol)
#define TEXTHEIGHTSYMBOL(self) (RENDATA(self)->text_height_symbol)
#define COLORSYMBOL(self) (RENDATA(self)->color_symbol)
#define MATERIALSYMBOL(self) (RENDATA(self)->material_symbol)
#define ASSIST(self) (RENDATA(self)->assist)
#define NPROCS(self) (RENDATA(self)->nprocs)
#define RANK(self) (RENDATA(self)->rank)

/* clean up from mimicing behavior of gl.h in getting around conflict 
 *  with X def of "Object"
 */
#ifdef OBJECT_HACK_SET
#undef Object
#undef OBJECT_HACK_SET
#endif

