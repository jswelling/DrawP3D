/****************************************************************************
 * p3d_ren_mthd.c
 * Author Joel Welling
 * Copyright 1990, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides renderer methods.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

/* Include file containing P3D preamble text */
#include "p3d_preamble.h"

/* Maximum length of file names and generated symbols (actually includes
 * the final \0).
 */
#define MAXFILENAME 128
#define MAXSYMBOLLENGTH P3D_NAMELENGTH

/* Struct to hold info for a color map */
typedef struct renderer_cmap_struct {
  char name[MAXSYMBOLLENGTH];
  double min;
  double max;
  void (*mapfun)( float *, float *, float *, float *, float * );
} P_Renderer_Cmap;

/* Struct to hold list of free names */
typedef struct symbol_name_list_struct {
  char *name;
  struct symbol_name_list_struct *next;
} P_Renderer_Sym_List;

/* Struct for object data, and access functions for it */
typedef struct renderer_data_struct {
  char filename[MAXFILENAME];
  FILE *outfile;
  int open;
  int initialized;
  char current_lights[MAXSYMBOLLENGTH];
  char current_camera[MAXSYMBOLLENGTH];
  P_Renderer_Cmap *current_cmap;
  int name_root;
  P_Renderer_Sym_List *free_symbol_list;
} P_Renderer_data;

#define RENDATA( self ) ((P_Renderer_data *)(self->object_data))
#define FILENAME( self ) (RENDATA(self)->filename)
#define OFILE( self ) (RENDATA(self)->outfile)
#define CUR_LIGHTS( self ) (RENDATA(self)->current_lights)
#define CUR_CAMERA( self ) (RENDATA(self)->current_camera)
#define CUR_MAP( self ) (RENDATA(self)->current_cmap)
#define NAME_ROOT( self ) (RENDATA(self)->name_root)
#define MAP_NAME( self ) (CUR_MAP(self)->name)
#define MAP_MIN( self ) (CUR_MAP(self)->min)
#define MAP_MAX( self ) (CUR_MAP(self)->max)
#define MAP_FUN( self ) (CUR_MAP(self)->mapfun)
#define FREE_SYMBOL_LIST( self ) (RENDATA(self)->free_symbol_list)

/* Space for default color map */
static P_Renderer_Cmap default_map;

/* Default color map function */
static void default_mapfun(float *val, float *r, float *g, float *b, float *a)
/* This routine provides a simple map from values to colors within the
 * range 0.0 to 1.0.
 */
{
  /* No debugging; called too often */
  *r= *g= *b= *a;
}

static char *get_name(P_Renderer *self, char *inname)
/* This routine returns a currently-free symbol name, either by
 * salvaging one from the free list or by making up a new one.
 */
{
  char *result;

  if (FREE_SYMBOL_LIST(self)) {
    P_Renderer_Sym_List* cell= FREE_SYMBOL_LIST(self);
    result= cell->name;
    FREE_SYMBOL_LIST(self)= cell->next;
    free( (P_Void_ptr)cell );
  }
  else {
    if ( !(result= (char *)malloc((MAXSYMBOLLENGTH+1)*sizeof(char))) )
      ger_fatal("p3d_ren_mthd: get_name: cannot allocate %d bytes!",
		MAXSYMBOLLENGTH+1);
    sprintf(result,"s%d",NAME_ROOT(self)++);
  }
  ger_debug("p3d_ren_mthd: get_name: returning <%s>",result);
  return(result);
}

static void unget_name(P_Renderer *self, char *name)
/* This routine takes the previously allocated name string and stashes it
 * on the free symbol name list, for later recovery by get_name().
 */
{
  P_Renderer_Sym_List* 
    cell= (P_Renderer_Sym_List*)malloc(sizeof(P_Renderer_Sym_List));
  if (!cell) 
    ger_fatal("p3d_ren_mthd: unget_name: cannot allocate %d bytes!",
	      sizeof(P_Renderer_Sym_List));
  cell->name= name;
  cell->next= FREE_SYMBOL_LIST(self);
  FREE_SYMBOL_LIST(self)= cell;
}

static P_Void_ptr def_cmap( char *name, double min, double max,
		  void (*mapfun)(float *, float *, float *, float *, float *) )
/* This function stores a color map definition */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Renderer_Cmap *thismap;
  METHOD_IN

  ger_debug("p3d_ren_mthd: def_cmap");

  if (max == min) {
    ger_error("p3d_ren_mthd: def_cmap: max equals min (val is %f)", max );
    METHOD_OUT;
    return( (P_Void_ptr)0 );    
  }

  if ( RENDATA(self)->open ) {
    if ( !(thismap= (P_Renderer_Cmap *)malloc(sizeof(P_Renderer_Cmap))) )
      ger_fatal( "p3d_ren_mthd: def_cmap: unable to allocate %d bytes!",
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

static void install_cmap( P_Void_ptr mapdata )
/* This method causes the given color map to become the 'current' map. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("p3d_ren_mthd: install_cmap");
  if ( RENDATA(self)->open ) {
    if (mapdata) CUR_MAP(self)= (P_Renderer_Cmap *)mapdata;
    else ger_error("p3d_ren_mthd: install_cmap: got null color map data.");
  }
  METHOD_OUT
}

static void destroy_cmap( P_Void_ptr mapdata )
/* This method causes renderer data associated with the given color map
 * to be freed.  This method must not refer to "self", because the
 * renderer which created this data may already have been destroyed.
 */
{
  ger_debug("p3d_ren_mthd: destroy_cmap");
  if ( mapdata ) free( mapdata );
}

static P_Void_ptr def_camera( P_Camera *cam )
/* This method defines the given camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  ger_debug("p3d_ren_mthd: def_camera");
  if ( RENDATA(self)->open ) {
    lclname= get_name(self,cam->name);

    /* Define the camera */
    fprintf(OFILE(self),"(setq %s (make-camera ; camera %s\n",lclname,
	    cam->name ? cam->name : "none");
    fprintf(OFILE(self),"   :lookfrom (make-point :x %g :y %g :z %g)\n",
            cam->lookfrom.x, cam->lookfrom.y, cam->lookfrom.z);
    fprintf(OFILE(self),"   :lookat (make-point :x %g :y %g :z %g)\n",
            cam->lookat.x, cam->lookat.y, cam->lookat.z);
    fprintf(OFILE(self),"   :up (make-vector :x %g :y %g :z %g)\n",
            cam->up.x, cam->up.y, cam->up.z);
    fprintf(OFILE(self),
	    "   :background (make-color :r %g :g %g :b %g :a %g)\n",
	    cam->background.r, cam->background.g, cam->background.b,
	    cam->background.a);
    fprintf(OFILE(self),"   :fovea %g :hither %g :yon %g))\n",
            cam->fovea, cam->hither, cam->yon);
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }
  METHOD_OUT
  return( (P_Void_ptr)0 );
}

static void set_camera( P_Void_ptr primdata )
/* This defines the given camera and makes it the current camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("p3d_ren_mthd: set_camera");
  if ( RENDATA(self)->open ) {
    if (primdata) {
      strncpy( CUR_CAMERA(self), (char *)primdata, MAXSYMBOLLENGTH-1 );
      CUR_CAMERA(self)[MAXSYMBOLLENGTH-1]= '\0';
    }
    else ger_error("p3d_ren_mthd: set_camera: got null primitive data.");
  }
  METHOD_OUT
}

static void destroy_camera( P_Void_ptr primdata )
{
  /* This method must not refer to "self", because the renderer for which
   * the camera was defined may already have been destroyed.
   */
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("p3d_ren_mthd: destroy_camera");
  /* Setting symbol to nil should cause camera to be garbage collected */
  /* Unfortunately we'd need OFILE(self) to know where to write, so we
   * just have to leave the camera lieing around in the lisp interpreter.
   */
  /*
    fprintf(OFILE(self),"(setq %s ())\n",(char *)primdata);
    unget_name( self, (char*)primdata );
  */
}

static void ren_print( VOIDLIST )
/* This is the print method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("p3d_ren_mthd: ren_print");
  if ( RENDATA(self)->open ) 
    ind_write("RENDERER: P3D generator '%s', file %s, open",
	      self->name, FILENAME(self)); 
  else ind_write("RENDERER: P3D generator '%s', file %s, closed",
		 self->name, FILENAME(self)); 
  ind_eol();
  METHOD_OUT
}

static void ren_open( VOIDLIST )
/* This is the open method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("p3d_ren_mthd: ren_open");
  RENDATA(self)->open= 1;
  if ( !RENDATA(self)->initialized ) {
    fprintf( OFILE(self), preamble1 );
    fprintf( OFILE(self), preamble2 );
    fprintf( OFILE(self), preamble3 );
    fprintf( OFILE(self), preamble4 );
    fprintf( OFILE(self), preamble5 );
    fprintf( OFILE(self), preamble6 );
    fprintf( OFILE(self), preamble7 );
    fprintf( OFILE(self), preamble8 );
    RENDATA(self)->initialized= 1;
  }
  METHOD_OUT
}

static void ren_close( VOIDLIST )
/* This is the close method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("p3d_ren_mthd: ren_close");
  RENDATA(self)->open= 0;
  METHOD_OUT
}

static void ren_destroy( VOIDLIST )
/* This is the destroy method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("p3d_ren_mthd: ren_destroy");
  if (RENDATA(self)->open) ren_close();
  if ( fclose(RENDATA(self)->outfile) == EOF ) {
    perror("p3d_ren_mthd: ren_destroy:");
    ger_fatal("p3d_ren_mthd: ren_destroy: Error closing file <%s>.",
	      RENDATA(self)->filename);
  }
  RENDATA(self)->initialized= 0;
  while (FREE_SYMBOL_LIST(self)) {
    P_Renderer_Sym_List* cell= FREE_SYMBOL_LIST(self);
    free( (P_Void_ptr)cell->name );
    FREE_SYMBOL_LIST(self)= cell->next;
    free( (P_Void_ptr)cell );
  }
  free( (P_Void_ptr)RENDATA(self) );
  free( (P_Void_ptr)self );
  METHOD_DESTROYED
}

static void ren_gob( P_Void_ptr primdata, P_Transform *trans, 
		    P_Attrib_List *attr )
/* This routine renders gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: ren_gob");
    if (primdata) 
      fprintf(OFILE(self),"(snap %s %s %s)\n", 
              (char *)primdata, CUR_LIGHTS(self), CUR_CAMERA(self)); 
    else ger_error("p3d_ren_mthd: ren_gob: got a null data pointer.");
  }
  METHOD_OUT
}

static void traverse_lights( P_Void_ptr primdata, P_Transform *trans, 
			    P_Attrib_List *attr )
/* This routine sets the lighting DAG to the given primitive */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: traverse_lights");
    if (primdata) {
      strncpy( CUR_LIGHTS(self),(char *)primdata, MAXSYMBOLLENGTH-1 );
      CUR_LIGHTS(self)[MAXSYMBOLLENGTH-1]= '\0';
    }
    else ger_error("p3d_ren_mthd: traverse_lights: got a null data pointer.");
  }
  METHOD_OUT
}

static void destroy_gob( P_Void_ptr primdata )
/* This routine destroys the renderer's data for the gob. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: destroy_gob");
    if (primdata) {
      fprintf(OFILE(self),"(free-gob %s)(setq %s nil)\n", 
	      (char *)primdata,(char *)primdata);
      unget_name( self, (char*)primdata );
    }
    else ger_error("p3d_ren_mthd: destroy_gob: got a null data pointer.");
  }
  METHOD_OUT
}

static P_Void_ptr def_sphere(char *name)
/* This routine defines a sphere */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_sphere");
    lclname= get_name(self,name);
    fprintf(OFILE(self), "(setq %s (sphere))\n", lclname);
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_cylinder(char *name)
/* This routine defines a cylinder */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_cylinder");
    lclname= get_name(self,name);
    fprintf(OFILE(self), "(setq %s (cylinder))\n", lclname);
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_torus(char *name, double major, double minor)
/* This routine defines a torus */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_torus");
    lclname= get_name(self,name);
    fprintf(OFILE(self), "(setq %s (torus %f %f))\n", 
            lclname, major, minor);
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void map_color( P_Renderer *self, float val, 
		      float *r, float *g, float *b, float *a )
/* This routine invokes the color map. */
{
  /* no debugging; called too often */

  /* Scale, clip, and map. */
  val= ( val-MAP_MIN(self) )/( MAP_MAX(self)-MAP_MIN(self) );
  if ( val > 1.0 ) val= 1.0;
  if ( val < 0.0 ) val= 0.0;
  (*MAP_FUN(self))( &val, r, g, b, a );
}

static void emit_vlist( P_Renderer *self, P_Vlist *vlist )
/* This routine outputs a vertex list */
{
  int i, vcount, type;
  float r, g, b, a;

  ger_debug("p3d_ren_mthd: emit_vlist");
  METHOD_RDY(vlist)
  vcount= vlist->length;
  type= vlist->type;

  for (i=0; i<vcount; i++) 
    switch (type) {
    case P3D_CVTX:   
      fprintf(OFILE(self), "(%g %g %g)\n",
          (*(vlist->x))(i), (*(vlist->y))(i), (*(vlist->z))(i)
          ); 
      break;
    case P3D_CCVTX:  
      fprintf(OFILE(self), "(%g %g %g (%g %g %g %g))\n",
          (*(vlist->x))(i),(*(vlist->y))(i),(*(vlist->z))(i),
          (*(vlist->r))(i),(*(vlist->g))(i),(*(vlist->b))(i),(*(vlist->a))(i)
          ); 
      break;
    case P3D_CCNVTX: 
      fprintf(OFILE(self), "(%g %g %g (%g %g %g %g) (%g %g %g))\n",
          (*(vlist->x))(i),(*(vlist->y))(i),(*(vlist->z))(i),
          (*(vlist->r))(i),(*(vlist->g))(i),(*(vlist->b))(i),(*(vlist->a))(i),
          (*(vlist->nx))(i),(*(vlist->ny))(i),(*(vlist->nz))(i)
          ); 
      break;
    case P3D_CNVTX:
      fprintf(OFILE(self), "(%g %g %g () (%g %g %g))\n",
          (*(vlist->x))(i),(*(vlist->y))(i),(*(vlist->z))(i),
          (*(vlist->nx))(i),(*(vlist->ny))(i),(*(vlist->nz))(i)
          );
      break;
    case P3D_CVVTX:
    case P3D_CVVVTX: /* ignore second value */
      map_color( self, (*(vlist->v))(i), &r, &g, &b, &a );
      fprintf(OFILE(self), "(%g %g %g (%g %g %g %g))\n",
	  (*(vlist->x))(i),(*(vlist->y))(i),(*(vlist->z))(i),
	  r, g, b, a
          ); 
      break;
    case P3D_CVNVTX: 
      map_color( self, (*(vlist->v))(i), &r, &g, &b, &a );
      fprintf(OFILE(self), "(%g %g %g (%g %g %g %g) (%g %g %g))\n",
	  (*(vlist->x))(i),(*(vlist->y))(i),(*(vlist->z))(i),
	  r, g, b, a,
          (*(vlist->nx))(i),(*(vlist->ny))(i),(*(vlist->nz))(i)
          ); 
      break;
    }
}

static P_Void_ptr def_polymarker(char *name, P_Vlist *vlist)
/* This routine defines a polymarker */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_polymarker");
    METHOD_RDY(vlist)
    lclname= get_name(self,name);
    switch (vlist->type) {
    case P3D_CVTX:   fprintf(OFILE(self),"(setq %s (pm '(\n", lclname); 
      break;
    case P3D_CCVTX:  fprintf(OFILE(self),"(setq %s (pmc '(\n", lclname); 
      break;
    case P3D_CCNVTX: fprintf(OFILE(self),"(setq %s (pmcn '(\n", lclname); 
      break;
    case P3D_CNVTX:  fprintf(OFILE(self), "(setq %s (pmn '(\n", lclname); 
      break;
    case P3D_CVVTX:  
    case P3D_CVVVTX:  /* ignore second value */
      fprintf(OFILE(self), "(setq %s (pmc '(\n", lclname); 
      break;
    case P3D_CVNVTX: fprintf(OFILE(self), "(setq %s (pmcn '(\n", lclname); 
      break;
    }
    emit_vlist(self, vlist);
    fprintf(OFILE(self), ")))\n");
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_polyline(char *name, P_Vlist *vlist)
/* This routine defines a polyline */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_polyline");
    METHOD_RDY(vlist)
    lclname= get_name(self,name);
    switch (vlist->type) {
    case P3D_CVTX:   fprintf(OFILE(self),"(setq %s (pl '(\n", lclname); 
      break;
    case P3D_CCVTX:  fprintf(OFILE(self),"(setq %s (plc '(\n", lclname); 
      break;
    case P3D_CCNVTX: fprintf(OFILE(self),"(setq %s (plcn '(\n", lclname); 
      break;
    case P3D_CNVTX:  fprintf(OFILE(self), "(setq %s (pln '(\n", lclname); 
      break;
    case P3D_CVVTX:  
    case P3D_CVVVTX:  /* ignore second value */
      fprintf(OFILE(self), "(setq %s (plc '(\n", lclname); 
      break;
    case P3D_CVNVTX: fprintf(OFILE(self), "(setq %s (plcn '(\n", lclname); 
      break;
    }
    emit_vlist(self, vlist);
    fprintf(OFILE(self), ")))\n");
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_polygon(char *name, P_Vlist *vlist)
/* This routine defines a polygon */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_polygon");
    METHOD_RDY(vlist)
    lclname= get_name(self,name);
    switch (vlist->type) {
    case P3D_CVTX:   fprintf(OFILE(self),"(setq %s (pg '(\n", lclname); 
      break;
    case P3D_CCVTX:  fprintf(OFILE(self),"(setq %s (pgc '(\n", lclname); 
      break;
    case P3D_CCNVTX: fprintf(OFILE(self),"(setq %s (pgcn '(\n", lclname); 
      break;
    case P3D_CNVTX:  fprintf(OFILE(self), "(setq %s (pgn '(\n", lclname); 
      break;
    case P3D_CVVTX:  
    case P3D_CVVVTX:  /* ignore second value */
      fprintf(OFILE(self), "(setq %s (pgc '(\n", lclname); 
      break;
    case P3D_CVNVTX: fprintf(OFILE(self), "(setq %s (pgcn '(\n", lclname); 
      break;
    }
    emit_vlist(self, vlist);
    fprintf(OFILE(self), ")))\n");
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_tristrip(char *name, P_Vlist *vlist)
/* This routine defines a triangle strip */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_tristrip");
    METHOD_RDY(vlist)
    lclname= get_name(self,name);
    switch (vlist->type) {
    case P3D_CVTX:   fprintf(OFILE(self),"(setq %s (tri '(\n", lclname); 
      break;
    case P3D_CCVTX:  fprintf(OFILE(self),"(setq %s (tric '(\n", lclname); 
      break;
    case P3D_CCNVTX: fprintf(OFILE(self),"(setq %s (tricn '(\n", lclname); 
      break;
    case P3D_CNVTX:  fprintf(OFILE(self), "(setq %s (trin '(\n", lclname); 
      break;
    case P3D_CVVTX:  
    case P3D_CVVVTX:  /* ignore second value */
      fprintf(OFILE(self), "(setq %s (tric '(\n", lclname); 
      break;
    case P3D_CVNVTX: fprintf(OFILE(self), "(setq %s (tricn '(\n", lclname); 
      break;
    }
    emit_vlist(self, vlist);
    fprintf(OFILE(self), ")))\n");
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_bezier(char *name, P_Vlist *vlist)
/* This routine defines a 4-by-4 bicubic Bezier patch */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_bezier");
    METHOD_RDY(vlist)
    if (vlist->length != 16) {
      ger_error(
         "p3d_ren_mthd: def_bezier: need 16 vertices for patch, found %d.",
         vlist->length);
      METHOD_OUT
      return((P_Void_ptr)0);
    }
    lclname= get_name(self,name);
    switch (vlist->type) {
    case P3D_CVTX:   fprintf(OFILE(self),"(setq %s (bp '(\n", lclname); 
      break;
    case P3D_CCVTX:  fprintf(OFILE(self),"(setq %s (bpc '(\n", lclname); 
      break;
    case P3D_CCNVTX: fprintf(OFILE(self),"(setq %s (bpcn '(\n", lclname); 
      break;
    case P3D_CNVTX:  fprintf(OFILE(self), "(setq %s (bpn '(\n", lclname); 
      break;
    case P3D_CVVTX:  
    case P3D_CVVVTX:  /* ignore second value */
      fprintf(OFILE(self), "(setq %s (bpc '(\n", lclname); 
      break;
    case P3D_CVNVTX: fprintf(OFILE(self), "(setq %s (bpcn '(\n", lclname); 
      break;
    }
    emit_vlist(self, vlist);
    fprintf(OFILE(self), ")))\n");
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_mesh(char *name, P_Vlist *vlist, int *indices,
                           int *facet_lengths, int nfacets)
/* This routine defines a general mesh */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  int ifacet, iindex;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_mesh");
    METHOD_RDY(vlist)
    lclname= get_name(self,name);

    /* Begin definition and emit vertex list */
    switch (vlist->type) {
    case P3D_CVTX:   fprintf(OFILE(self),"(setq %s (msh %d '(\n", 
                             lclname,vlist->length);
      break;
    case P3D_CCVTX:  fprintf(OFILE(self),"(setq %s (mshc %d '(\n", 
                             lclname,vlist->length);
      break;
    case P3D_CCNVTX: fprintf(OFILE(self),"(setq %s (mshcn %d '(\n",
                             lclname,vlist->length);
      break;
    case P3D_CNVTX:  fprintf(OFILE(self), "(setq %s (mshn %d '(\n",
                             lclname,vlist->length);
      break;
    case P3D_CVVTX:  
    case P3D_CVVVTX:  /* ignore second value */
      fprintf(OFILE(self), "(setq %s (mshc %d '(\n",
                             lclname,vlist->length);
      break;
    case P3D_CVNVTX: fprintf(OFILE(self), "(setq %s (mshcn %d '(\n",
                             lclname,vlist->length);
      break;
    }
    emit_vlist(self, vlist);

    /* Close vertex list and emit facet index sublists */
    fprintf(OFILE(self), ") '( \n");
    for (ifacet=0; ifacet<nfacets; ifacet++) {
      fprintf(OFILE(self), "( \n");
      for (iindex=0; iindex<facet_lengths[ifacet]; iindex++)
        fprintf(OFILE(self), "%d\n", *indices++);
      fprintf(OFILE(self), ") ");
    }

    /* Close the whole thing */
    fprintf(OFILE(self), ")))\n");

    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_text( char *name, char *tstring, P_Point *location, 
                           P_Vector *u, P_Vector *v )
/* This method defines a text string */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_text");
    lclname= get_name(self,name);
    fprintf(OFILE(self),
            "(setq %s (tx \"%s\" '(%g %g %g)\n '(%g %g %g) '(%g %g %g)))\n",
            lclname, tstring, location->x, location->y, location->z,        
            u->x, u->y, u->z, v->x, v->y, v->z);
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_light( char *name, P_Point *location, P_Color *color )
/* This method defines a positional light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_light");
    lclname= get_name(self,name);
    rgbify_color(color);
    fprintf(OFILE(self),
            "(setq %s (lt '(%g %g %g) '(%g %g %g)))\n",
            lclname, location->x, location->y, location->z,
            color->r, color->g, color->b);
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_ambient( char *name, P_Color *color )
/* This method defines an ambient light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_ambient");
    lclname= get_name(self,name);
    rgbify_color(color);
    fprintf(OFILE(self),
            "(setq %s (amb '(%g %g %g)))\n",
            lclname, color->r, color->g, color->b);
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void def_trans(FILE *ofile, P_Transform *trans)
/* This routine emits a transform definition */
{
  int i;
  float *val;

  ger_debug("p3d_ren_mthd: def_trans");

  fprintf(ofile,"(trns '(\n");
  val= trans->d;
  for (i=0; i<16; i++) {
    if ( !( i%4 ) ) fprintf(ofile,"(");
    fprintf(ofile,"%f ",*val++);
    if ( !( (i+1) % 4 ) ) fprintf(ofile,")\n");
  }
  fprintf(ofile,"))");
}

static void def_attr( FILE *ofile, P_Attrib_List *attr )
/* This routine emits definitions for attributes */
{
  P_Color *clr;
  P_Point *pt;
  P_Vector *vec;
  P_Material *mat;

  ger_debug("p3d_ren_mthd: def_attr");

  fprintf(ofile,"(list \n");
  while (attr) {
    fprintf(ofile,"(cons '%s ",attr->attribute);
    switch (attr->type) {
    case P3D_INT: fprintf(ofile,"%d",*(int *)attr->value); break;
    case P3D_BOOLEAN: 
      if (*(int *)(attr->value)) fprintf(ofile,"T");else fprintf(ofile,"nil");
      break;
    case P3D_FLOAT: fprintf(ofile,"%f",*(float *)attr->value); break;
    case P3D_STRING: fprintf(ofile,"\"%s\"",(char *)(attr->value)); break;
    case P3D_COLOR: 
      clr= (P_Color *)attr->value;
      rgbify_color(clr);
      fprintf(ofile,"(clr %g %g %g %g)", clr->r, clr->g, clr->b, clr->a);
      break;
    case P3D_POINT: 
      pt= (P_Point *)attr->value;
      fprintf(ofile,"(pt %g %g %g)", pt->x, pt->y, pt->z );
      break;
    case P3D_VECTOR: 
      vec= (P_Vector *)attr->value;
      fprintf(ofile,"(vec %g %g %g)", vec->x, vec->y, vec->z );
      break;
    case P3D_TRANSFORM: 
      def_trans( ofile, (P_Transform *)attr->value );
      break;
    case P3D_MATERIAL:
      mat= (P_Material *)attr->value;
      switch (mat->type) {
      case P3D_DEFAULT_MATERIAL:
	fprintf(ofile,"default-material");
	break;
      case P3D_DULL_MATERIAL:
	fprintf(ofile,"dull-material");
	break;
      case P3D_SHINY_MATERIAL:
	fprintf(ofile,"shiny-material");
	break;
      case P3D_METALLIC_MATERIAL:
	fprintf(ofile,"metallic-material");
	break;
      case P3D_MATTE_MATERIAL:
	fprintf(ofile,"matte-material");
	break;
      case P3D_ALUMINUM_MATERIAL:
	fprintf(ofile,"aluminum-material");
	break;
      default:
	fprintf(stderr,"p3d_ren_mthd: def_attr: unknown material type %d!\n",
		mat->type);
	fprintf(ofile,"default-material");
      }
      break;
    case P3D_OTHER: 
      fprintf(ofile,"nil"); /* Can't define it if we don't know what it is */
      break;
    }
    fprintf(ofile,")\n");
    attr= attr->next;
  }
  fprintf(ofile,")\n");
}

static P_Void_ptr def_gob( char *name, P_Gob *gob )
/* This method defines a gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *lclname, *kidname;
  P_Gob_List *kids;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: def_gob");
    lclname= get_name(self,name);
    fprintf(OFILE(self),
            "(setq %s (def-gob :children (list ; gob %s\n",lclname,
	    name ? name : "none");
    kids= gob->children;
    while (kids) {
      METHOD_RDY(kids->gob);
      kidname= (char *)((*(kids->gob->get_ren_data))(self));
      /* null kidname means it wasn't defined for this renderer */
      if (kidname) fprintf(OFILE(self),"%s\n",kidname); 
      else fprintf(OFILE(self),";not defined\n");
      kids= kids->next;
    }
    fprintf(OFILE(self),")");
    if (gob->has_transform) {
      fprintf(OFILE(self)," :transform ");
      def_trans(OFILE(self),&(gob->trans));
    }
    if (gob->attr) {
      fprintf(OFILE(self)," :attr ");
      def_attr(OFILE(self), gob->attr);
    }
    fprintf(OFILE(self),"))\n");
    METHOD_OUT
    return( (P_Void_ptr)lclname );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void hold_gob( P_Void_ptr primdata )
/* This routine renders gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: hold_gob");
    if (primdata) 
      fprintf(OFILE(self),"(hold-gob %s)\n", (char *)primdata);
    else ger_error("p3d_ren_mthd: hold_gob: got a null data pointer.");
  }
  METHOD_OUT
}

static void unhold_gob( P_Void_ptr primdata )
/* This routine renders gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("p3d_ren_mthd: unhold_gob");
    if (primdata) 
      fprintf(OFILE(self),"(unhold-gob %s)\n", (char *)primdata);
    else ger_error("p3d_ren_mthd: unhold_gob: got a null data pointer.");
  }
  METHOD_OUT
}

P_Renderer *po_create_p3d_renderer( char *device, char *datastr )
/* This routine creates a p3d-generating renderer object */
{
  P_Renderer *thisrenderer;
  P_Renderer_data *rdata;
  static int sequence_number= 0;

  ger_debug(
	    "po_create_p3d_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(thisrenderer= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_p3d_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_p3d_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  thisrenderer->object_data= (P_Void_ptr)rdata;

  /* output file handling */
  if ( !strcmp(device,"-") ) rdata->outfile= stdout;
  else if ( !(rdata->outfile= fopen(device,"w")) ) {
    perror("po_create_p3d_renderer");
    ger_fatal("po_create_p3d_renderer: Error opening file <%s> for writing.",
	      device);
  }

  /* Fill out default color map */
  strcpy(default_map.name,"default-map");
  default_map.min= 0.0;
  default_map.max= 1.0;
  default_map.mapfun= default_mapfun;

  /* Fill out public and private object data */
  sprintf(thisrenderer->name,"p3d%d",sequence_number++);
  strncpy( rdata->filename, device, MAXFILENAME-1 );
  rdata->filename[MAXFILENAME-1]= '\0';
  rdata->open= 0;  /* renderer created in closed state */
  rdata->initialized= 0;
  strcpy(CUR_LIGHTS(thisrenderer),"default-lights");
  strcpy(CUR_CAMERA(thisrenderer),"default-camera");
  CUR_MAP(thisrenderer)= &default_map;
  NAME_ROOT(thisrenderer)= 0;
  FREE_SYMBOL_LIST(thisrenderer)= NULL;

  /* Fill in all the methods */
  thisrenderer->def_sphere= def_sphere;
  thisrenderer->ren_sphere= ren_gob;
  thisrenderer->destroy_sphere= destroy_gob;

  thisrenderer->def_cylinder= def_cylinder;
  thisrenderer->ren_cylinder= ren_gob;
  thisrenderer->destroy_cylinder= destroy_gob;

  thisrenderer->def_torus= def_torus;
  thisrenderer->ren_torus= ren_gob;
  thisrenderer->destroy_torus= destroy_gob;

  thisrenderer->def_polymarker= def_polymarker;
  thisrenderer->ren_polymarker= ren_gob;
  thisrenderer->destroy_polymarker= destroy_gob;

  thisrenderer->def_polyline= def_polyline;
  thisrenderer->ren_polyline= ren_gob;
  thisrenderer->destroy_polyline= destroy_gob;

  thisrenderer->def_polygon= def_polygon;
  thisrenderer->ren_polygon= ren_gob;
  thisrenderer->destroy_polygon= destroy_gob;

  thisrenderer->def_tristrip= def_tristrip;
  thisrenderer->ren_tristrip= ren_gob;
  thisrenderer->destroy_tristrip= destroy_gob;

  thisrenderer->def_bezier= def_bezier;
  thisrenderer->ren_bezier= ren_gob;
  thisrenderer->destroy_bezier= destroy_gob;

  thisrenderer->def_mesh= def_mesh;
  thisrenderer->ren_mesh= ren_gob;
  thisrenderer->destroy_mesh= destroy_gob;

  thisrenderer->def_text= def_text;
  thisrenderer->ren_text= ren_gob;
  thisrenderer->destroy_text= destroy_gob;

  thisrenderer->def_light= def_light;
  thisrenderer->ren_light= ren_gob;
  thisrenderer->light_traverse_light= traverse_lights;
  thisrenderer->destroy_light= destroy_gob;

  thisrenderer->def_ambient= def_ambient;
  thisrenderer->ren_ambient= ren_gob;
  thisrenderer->light_traverse_ambient= traverse_lights;
  thisrenderer->destroy_ambient= destroy_gob;

  thisrenderer->def_gob= def_gob;
  thisrenderer->ren_gob= ren_gob;
  thisrenderer->light_traverse_gob= traverse_lights;
  thisrenderer->hold_gob= hold_gob;
  thisrenderer->unhold_gob= unhold_gob;
  thisrenderer->destroy_gob= destroy_gob;

  thisrenderer->print= ren_print;
  thisrenderer->open= ren_open;
  thisrenderer->close= ren_close;
  thisrenderer->destroy_self= ren_destroy;

  thisrenderer->def_camera= def_camera;
  thisrenderer->set_camera= set_camera;
  thisrenderer->destroy_camera= destroy_camera;

  thisrenderer->def_cmap= def_cmap;
  thisrenderer->install_cmap= install_cmap;
  thisrenderer->destroy_cmap= destroy_cmap;

  return( thisrenderer );
}
