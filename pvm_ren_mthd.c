/****************************************************************************
 * pvm_ren_mthd.c
 * Copyright 1995, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Joel Welling
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
/* This module implements the PVM-mediated distributed renderer */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <pvm3.h>
#ifdef CRAY
#include <time.h>
#endif
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "pvm_ren_mthd.h"
#include "pvm_geom.h"

/* Notes:
   -We never actually quit PVM, on the assumption that the calling 
    program will do so.
 */

/* Instance counter */
static int instance_count= 0;

/* Space for transformation buffer */
static float* transform_buf= NULL;

/* Space for default color map */
static P_Renderer_Cmap default_map;

/* Most recent instance of this renderer to call ren_gob at top level 
 * while open
 */
static P_Renderer* most_recent_self= NULL;

/* Most recently sent outgoing PVM message info, for debugging use */
static int most_recent_send_buf= 0;
int pg_pvm_most_recent_renserver_tid= 0;

void pg_pvm_ren_retransmit(); /* backdoor useful in debugging */
#ifdef CRAY
long pg_pvm_sync_clocks= 0; /* counter for total time spent in sync wait */
#endif


/* default color map function */
static void default_mapfun(float *val, float *r, float *g, float *b, float *a)
/* This routine provides a simple map from values to colors within the
 * range 0.0 to 1.0.
 */
{
  /* No debugging; called too often */
  *r= *g= *b= *a= *val;
}

static P_Void_ptr def_cmap( char *name, double min, double max,
		  void (*mapfun)(float *, float *, float *, float *, float *) )
/* This function stores a color map definition */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Renderer_Cmap *thismap;
  METHOD_IN

  ger_debug("pvm_ren_mthd: def_cmap");

  if (max == min) {
    ger_error("pvm_ren_mthd: def_cmap: max equals min (val is %f)", max );
    METHOD_OUT;
    return( (P_Void_ptr)0 );    
  }

  if ( RENDATA(self)->open ) {
    if ( !(thismap= (P_Renderer_Cmap *)malloc(sizeof(P_Renderer_Cmap))) )
      ger_fatal( "pvm_ren_mthd: def_cmap: unable to allocate %d bytes!",
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

  ger_debug("pvm_ren_mthd: install_cmap");
  if ( RENDATA(self)->open ) {
    if (mapdata) CUR_MAP(self)= (P_Renderer_Cmap *)mapdata;
    else ger_error("pvm_ren_mthd: install_cmap: got null color map data.");
  }
  METHOD_OUT
}

static void destroy_cmap( P_Void_ptr mapdata )
/* This method causes renderer data associated with the given color map
 * to be freed.  It must not refer to "self", because the renderer which
 * created the data may already have been destroyed.
 */
{
  ger_debug("pvm_ren_mthd: destroy_cmap");
  if ( mapdata ) free( mapdata );
}

static P_Void_ptr def_camera( P_Camera *cam )
/* This method defines the given camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("pvm_ren_mthd: def_camera");

  /* Just need to set up to find camera info later */

  METHOD_OUT
  return( (P_Void_ptr)cam);
}

static void set_camera( P_Void_ptr primdata )
/* This defines the given camera and makes it the current camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("pvm_ren_mthd: set_camera");
  CURRENT_CAMERA(self)= (P_Camera*)primdata;

  METHOD_OUT
}

static void pack_camera( P_Renderer* self )
/* This actually encapsulates camera data for transmission by PVM */
{
  if (CURRENT_CAMERA(self)) {
    static float buf[16];
    static int msgbuf[1]= {PVM3D_CAMERA};
    P_Color bkg;

    ger_debug("pvm_ren_mthd: pack_camera");
    
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: pack_camera: error in pvm_pkint");
    
    buf[0]= CURRENT_CAMERA(self)->lookfrom.x;
    buf[1]= CURRENT_CAMERA(self)->lookfrom.y;
    buf[2]= CURRENT_CAMERA(self)->lookfrom.z;
    buf[3]= CURRENT_CAMERA(self)->lookat.x;
    buf[4]= CURRENT_CAMERA(self)->lookat.y;
    buf[5]= CURRENT_CAMERA(self)->lookat.z;
    buf[6]= CURRENT_CAMERA(self)->up.x;
    buf[7]= CURRENT_CAMERA(self)->up.y;
    buf[8]= CURRENT_CAMERA(self)->up.z;
    buf[9]= CURRENT_CAMERA(self)->fovea;
    buf[10]= CURRENT_CAMERA(self)->hither;
    buf[11]= CURRENT_CAMERA(self)->yon;
    copy_color( &bkg, &(CURRENT_CAMERA(self)->background) );
    rgbify_color( &bkg );
    buf[12]= bkg.r;
    buf[13]= bkg.g;
    buf[14]= bkg.b;
    buf[15]= bkg.a;
      
    if (pvm_pkfloat(buf, 16, 1)<0)
      pvm_perror("pvm_ren_mthd: pack_camera: error in pvm_pkfloat");
  }
}

static void destroy_camera( P_Void_ptr primdata )
{
  /* This method must not refer to "self", because the renderer for which
   * the camera was defined may already have been destroyed.
   */
  ger_debug("pvm_ren_mthd: destroy_camera");
  /* do something */

}

static void ren_print( VOIDLIST )
/* This is the print method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("pvm_ren_mthd: ren_print");
  if ( RENDATA(self)->open ) 
    fprintf(stdout,"RENDERER: PVM renderer '%s', open", self->name); 
  else fprintf(stdout,"RENDERER: PVM renderer '%s', closed", self->name); 
  METHOD_OUT
}

static void ren_open( VOIDLIST )
/* This is the open method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("pvm_ren_mthd: ren_open");
  RENDATA(self)->open= 1;
  METHOD_OUT
}

static void ren_close( VOIDLIST )
/* This is the close method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("pvm_ren_mthd: ren_close");
  RENDATA(self)->open= 0;
  METHOD_OUT
}

static void check_attrs( P_Renderer* self )
{
  int do_backcull= 0;
  int backcull_val;
  int do_color= 0;
  P_Color* color_val;
  int do_material= 0;
  P_Material mat_val;
  int do_text_height= 0;
  float txt_ht_val;
  
  METHOD_RDY(ASSIST(self));
  backcull_val= (*(ASSIST(self)->bool_attribute))(BACKCULLSYMBOL(self));
  color_val= (*(ASSIST(self)->color_attribute))(COLORSYMBOL(self));
  mat_val.type= 
    (*(ASSIST(self)->material_attribute))(MATERIALSYMBOL(self))->type;
  txt_ht_val= (*(ASSIST(self)->float_attribute))(TEXTHEIGHTSYMBOL(self));
	       
  if (!ATTRS_SET(self))
    do_backcull= do_color= do_material= do_text_height= 1;
  else {
    if (CURRENT_BACKCULL(self) != backcull_val) do_backcull= 1;
    if ((color_val->r != CURRENT_COLOR(self).r)
	|| (color_val->g != CURRENT_COLOR(self).g)
	|| (color_val->b != CURRENT_COLOR(self).b)
	|| (color_val->a != CURRENT_COLOR(self).a)) do_color= 1;
    if (CURRENT_MATERIAL(self).type != mat_val.type) do_material= 1;
    if (CURRENT_TEXT_HEIGHT(self) != txt_ht_val) do_text_height= 1;
  }

  if (do_backcull) {
    static int msgbuf[2]= { PVM3D_BACKCULL, 0 };
    msgbuf[1]= CURRENT_BACKCULL(self)= backcull_val;
    if (pvm_pkint(msgbuf,2,1) < 0)
      pvm_perror("pvm_ren_mthd: check_attrs: error in pvm_pkint 1");
  }
	       
  if (do_color) {
    static int msgbuf[1]= { PVM3D_COLOR };
    static float fbuf[4];
    fbuf[0]= CURRENT_COLOR(self).r= color_val->r;
    fbuf[1]= CURRENT_COLOR(self).g= color_val->g;
    fbuf[2]= CURRENT_COLOR(self).b= color_val->b;
    fbuf[3]= CURRENT_COLOR(self).a= color_val->a;
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: check_attrs: error in pvm_pkint 1");
    if (pvm_pkfloat(fbuf,4,1) < 0)
      pvm_perror("pvm_ren_mthd: check_attrs: error in pvm_pkfloat 1");
  }
	       
  if (do_material) {
    static int msgbuf[2]= { PVM3D_MATERIAL, 0 };
    msgbuf[1]= CURRENT_MATERIAL(self).type= mat_val.type;
    if (pvm_pkint(msgbuf,2,1) < 0)
      pvm_perror("pvm_ren_mthd: check_attrs: error in pvm_pkint 2");
  }
	       
  if (do_text_height) {
    static int msgbuf[1]= {PVM3D_TEXT_HEIGHT};
    static float fbuf[1];
    msgbuf[0]= PVM3D_TEXT_HEIGHT;
    fbuf[0]= CURRENT_TEXT_HEIGHT(self)= txt_ht_val;
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: check_attrs: error in pvm_pkint 3");
    if (pvm_pkfloat(fbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: check_attrs: error in pvm_pkfloat 2");
  }
	       
  ATTRS_SET(self)= 1;
}

static P_Void_ptr def_sphere(char *name)
/* This routine defines a sphere */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: def_sphere");
    METHOD_OUT
    return( (P_Void_ptr)0 ); /* no prim data needed for sphere */
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_sphere(P_Void_ptr object_data, P_Transform *transform,
		       P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_SPHERE};
    P_Transform* current_trans;
    ger_debug("pvm_ren_mthd: ren_sphere");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_sphere: error in pvm_pkint");
    METHOD_RDY(ASSIST(self));
    current_trans= (*(ASSIST(self)->get_trans))();
    if (pvm_pkfloat(current_trans->d, 12, 1)<0)
      pvm_perror("pvm_ren_mthd: ren_sphere: error in pvm_pkfloat");
  }

  METHOD_OUT;
}

static void destroy_sphere( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: destroy_cylinder");
    /* Nothing to destroy */
  }

  METHOD_OUT;
}

static P_Void_ptr def_cylinder(char *name)
/* This routine defines a cylinder */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: def_cylinder");
    METHOD_OUT
    return( (P_Void_ptr)0 ); /* no prim data needed for cylinder */
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_cylinder(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_CYLINDER};
    P_Transform* current_trans;
    ger_debug("pvm_ren_mthd: ren_cylinder");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_sphere: error in pvm_pkint");
    METHOD_RDY(ASSIST(self));
    current_trans= (*(ASSIST(self)->get_trans))();
    if (pvm_pkfloat(current_trans->d, 12, 1)<0)
      pvm_perror("pvm_ren_mthd: ren_cylinder: error in pvm_pkfloat");
  }

  METHOD_OUT;
}

static void destroy_cylinder( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: destroy_cylinder");
    /* Nothing to destroy */
  }

  METHOD_OUT;
}

static P_Void_ptr def_torus(char *name, double major, double minor)
/* This routine defines a torus */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    float *data;
    ger_debug("pvm_ren_mthd: def_torus");
    if ( !(data= (float*)malloc(2*sizeof(float))) ) {
      fprintf(stderr,"pvm_ren_mthd: cannot allocate 2 floats!\n");
      exit(-1);
    }
    data[0]= major;
    data[1]= minor;
    METHOD_OUT
    return( (P_Void_ptr)data );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_torus(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_TORUS};
    P_Transform* current_trans;
    ger_debug("pvm_ren_mthd: ren_torus");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_torus: error in pvm_pkint");
    if (pvm_pkfloat((float*)object_data,2,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_torus: error in pvm_pkfloat");
    METHOD_RDY(ASSIST(self));
    current_trans= (*(ASSIST(self)->get_trans))();
    if (pvm_pkfloat(current_trans->d, 12, 1)<0)
      pvm_perror("pvm_ren_mthd: ren_torus: error in pvm_pkfloat");
  }

  METHOD_OUT;
}

static void destroy_torus( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: destroy_torus");
    free( object_data );
  }

  METHOD_OUT;
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

static P_Cached_Vlist* cache_vlist( P_Renderer *self, P_Vlist *vlist )
/* This routine outputs a vertex list */
{
  int i;
  float r, g, b, a;
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
	   (float*)malloc( 3*result->length*sizeof(float) )) ) {
      fprintf(stderr,"pvm_ren_mthd: cannot allocate %d bytes!\n",
	      3*result->length*sizeof(float));
      exit(-1);
    }
    if ( !(result->opacities= 
	   (float*)malloc( result->length*sizeof(float) )) ) {
      fprintf(stderr,"pvm_ren_mthd: cannot allocate %d bytes!\n",
	      result->length*sizeof(float));
      exit(-1);
    }
  }
  else {
    result->colors= NULL;
    result->opacities= NULL;
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

  result->info_word= ((result->length) << 8) | (result->type & 255);

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
      result->colors[3*i]= (*(vlist->r))(i);
      result->colors[3*i+1]= (*(vlist->g))(i);
      result->colors[3*i+2]= (*(vlist->b))(i);
      result->opacities[i]= (*(vlist->a))(i);
      break;
    case P3D_CCNVTX:
      result->colors[3*i]= (*(vlist->r))(i);
      result->colors[3*i+1]= (*(vlist->g))(i);
      result->colors[3*i+2]= (*(vlist->b))(i);
      result->opacities[i]= (*(vlist->a))(i);
      result->normals[3*i]= (*(vlist->nx))(i);
      result->normals[3*i+1]= (*(vlist->ny))(i);
      result->normals[3*i+2]= (*(vlist->nz))(i);
      break;
    case P3D_CVVTX:
    case P3D_CVVVTX:
      map_color( self, (*(vlist->v))(i), &r, &g, &b, &a );
      result->colors[3*i]= r;
      result->colors[3*i+1]= g;
      result->colors[3*i+2]= b;
      result->opacities[i]= a;
      break;
    case P3D_CVNVTX:
      map_color( self, (*(vlist->v))(i), &r, &g, &b, &a );
      result->colors[3*i]= r;
      result->colors[3*i+1]= g;
      result->colors[3*i+2]= b;
      result->opacities[i]= a;
      result->normals[3*i]= (*(vlist->nx))(i);
      result->normals[3*i+1]= (*(vlist->ny))(i);
      result->normals[3*i+2]= (*(vlist->nz))(i);
      break;
      /* We've already checked that it is a known case */
    }
  }

  return( result );
}

static void pack_cached_vlist( P_Renderer* self, P_Cached_Vlist* cache )
{
  int coords_sent, coords_this_block;
  P_Transform* current_trans;
  int identity_trans_flag;
  float i;
  float *runner;
  float *trunner;

  current_trans= (*(ASSIST(self)->get_trans))();

  if (pvm_pkint(&(cache->info_word),1,1) < 0)
    pvm_perror("pvm_ren_mthd: pack_cached_vlist: error in pvm_pkint");

  /* Send colors, then normals, then coords, for convenience of display
   * server.
   */
  if (cache->colors) {
    if (pvm_pkfloat( cache->colors, 3*cache->length, 1) < 0)
      pvm_perror("pvm_ren_mthd: pack_cached_vlist: error in pvm_pkfloat 1");
    if (pvm_pkfloat( cache->opacities, cache->length, 1) < 0)
      pvm_perror("pvm_ren_mthd: pack_cached_vlist: error in pvm_pkfloat 2");
  }

  identity_trans_flag= 1;
  runner= current_trans->d;
  trunner= Identity_trans->d;
  for (i=0; i<16; i++) 
    if (*runner++ != *trunner++) {
      identity_trans_flag= 0;
      break;
    }
  if (identity_trans_flag) {
    if (cache->normals) {
      if (pvm_pkfloat( cache->normals, 3*cache->length, 1) < 0)
      pvm_perror("pvm_ren_mthd: pack_cached_vlist: error in pvm_pkfloat 3");
    }
    if (pvm_pkfloat( cache->coords, 3*cache->length, 1) < 0)
      pvm_perror("pvm_ren_mthd: pack_cached_vlist: error in pvm_pkfloat 4");
  }
  else {
    if (cache->normals) {
      coords_sent= 0;
      runner= cache->normals;
      while (coords_sent < cache->length) {
	coords_this_block= 
	  (TRANSFORM_BUF_TRIPLES > (cache->length - coords_sent)) ?
	  cache->length - coords_sent : TRANSFORM_BUF_TRIPLES;
	trunner= transform_buf;
	for (i=0; i<coords_this_block; i++) {
	  *trunner++= 
	    (current_trans->d[0] * *runner) + (current_trans->d[4] * 
					       *(runner+1))
	    + (current_trans->d[8] * *(runner+2));  
	  *trunner++= 
	    (current_trans->d[1] * *runner) + (current_trans->d[5] * 
					       *(runner+1))
	    + (current_trans->d[9] * *(runner+2));  
	  *trunner++= 
	    (current_trans->d[2] * *runner) + (current_trans->d[6] * 
					       *(runner+1))
	    + (current_trans->d[10] * *(runner+2));  
	  runner += 3;
	}
	if (pvm_pkfloat( transform_buf, 3*coords_this_block, 1) < 0)
	  pvm_perror(
		 "pvm_ren_mthd: pack_cached_vlist: error in pvm_pkfloat 5");
	coords_sent += coords_this_block;
      }
    }
    
    coords_sent= 0;
    runner= cache->coords;
    while (coords_sent < cache->length) {
      coords_this_block= 
	(TRANSFORM_BUF_TRIPLES > (cache->length - coords_sent)) ?
	cache->length - coords_sent : TRANSFORM_BUF_TRIPLES;
      trunner= transform_buf;
      for (i=0; i<coords_this_block; i++) {
	*trunner++= 
	  (current_trans->d[0] * *runner) + (current_trans->d[1] * *(runner+1))
	  + (current_trans->d[2] * *(runner+2)) + (current_trans->d[3]);  
	*trunner++= 
	  (current_trans->d[4] * *runner) + (current_trans->d[5] * *(runner+1))
	  + (current_trans->d[6] * *(runner+2)) + (current_trans->d[7]);  
	*trunner++= 
	  (current_trans->d[8] * *runner) + (current_trans->d[9] * *(runner+1))
	  + (current_trans->d[10] * *(runner+2)) + (current_trans->d[11]);  
	runner += 3;
      }
      if (pvm_pkfloat( transform_buf, 3*coords_this_block, 1) < 0)
	pvm_perror("pvm_ren_mthd: pack_cached_vlist: error in pvm_pkfloat 6");
      coords_sent += coords_this_block;
    }
  }
}

static void free_cached_vlist( P_Cached_Vlist* cache )
{
  if (cache->normals) free( (P_Void_ptr)(cache->normals) );
  if (cache->colors) free( (P_Void_ptr)(cache->colors) );
  if (cache->opacities) free( (P_Void_ptr)(cache->opacities) );
  free( (P_Void_ptr)cache->coords );
  free( (P_Void_ptr)cache );
}

static P_Void_ptr def_polything(char *name, P_Vlist *vlist)
/* This routine defines a polymarker */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr cache_data;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: def_polything");
    METHOD_RDY(vlist)
    cache_data= cache_vlist(self, vlist);
    METHOD_OUT
    return( cache_data );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void destroy_polything( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: destroy_polything");
    free_cached_vlist( (P_Cached_Vlist*)object_data );
    free( object_data );
  }

  METHOD_OUT;
}

static void ren_polymarker(P_Void_ptr object_data, P_Transform *transform,
			   P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_POLYMARKER};
    ger_debug("pvm_ren_mthd: ren_polymarker");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_polymarker: error in pvm_pkint");
    pack_cached_vlist( self, (P_Cached_Vlist*)object_data );
  }

  METHOD_OUT;
}

static void ren_polyline(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_POLYLINE};
    ger_debug("pvm_ren_mthd: ren_polyline");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_polyline: error in pvm_pkint");
    pack_cached_vlist( self, (P_Cached_Vlist*)object_data );
  }

  METHOD_OUT;
}

static void ren_polygon(P_Void_ptr object_data, P_Transform *transform,
			P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_POLYGON};
    ger_debug("pvm_ren_mthd: ren_polygon");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_polygon: error in pvm_pkint");
    pack_cached_vlist( self, (P_Cached_Vlist*)object_data );
  }

  METHOD_OUT;
}

static void ren_tristrip(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_TRISTRIP};
    ger_debug("pvm_ren_mthd: ren_tristrip");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_tristrip: error in pvm_pkint");
    pack_cached_vlist( self, (P_Cached_Vlist*)object_data );
  }

  METHOD_OUT;
}

static void ren_bezier(P_Void_ptr object_data, P_Transform *transform,
		       P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    static int msgbuf[1]= {PVM3D_BEZIER};
    ger_debug("pvm_ren_mthd: ren_bezier");
    check_attrs(self);
    if (pvm_pkint(msgbuf,1,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_bezier: error in pvm_pkint");
    pack_cached_vlist( self, (P_Cached_Vlist*)object_data );
  }

  METHOD_OUT;
}

static P_Void_ptr def_mesh(char *name, P_Vlist *vlist, int *indices,
                           int *facet_lengths, int nfacets)
/* This routine defines a general mesh */
{
  P_Renderer *self= (P_Renderer *)po_this;
  int ifacet, iindex;
  METHOD_IN

  if (RENDATA(self)->open) {
    P_Cached_Mesh* result;
    int index_count= 0;
    ger_debug("pvm_ren_mthd: def_mesh");

    if ( !(result= (P_Cached_Mesh*)malloc(sizeof(P_Cached_Mesh))) ) {
      fprintf(stderr,"pvm_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
	      sizeof(P_Cached_Mesh));
      exit(-1);
    }
    if ( !(result->facet_lengths=(int*)malloc(nfacets*sizeof(int))) ) {
      fprintf(stderr,"pvm_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
	      nfacets*sizeof(int));
      exit(-1);
    }

    result->nfacets= nfacets;
    for (ifacet=0; ifacet<nfacets; ifacet++) {
      index_count += facet_lengths[ifacet];
      result->facet_lengths[ifacet]= facet_lengths[ifacet];
    }

    result->nindices= index_count;
    if (!(result->indices=(int*)malloc(index_count*sizeof(int))) ) {
      fprintf(stderr,"pvm_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
	      nfacets*sizeof(int));
      exit(-1);
    }

    for (iindex=0; iindex<index_count; iindex++)
      result->indices[iindex]= indices[iindex];

    METHOD_RDY(vlist)
    result->cached_vlist= cache_vlist(self,vlist);

    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_mesh(P_Void_ptr object_data, P_Transform *transform,
		       P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Mesh* data= (P_Cached_Mesh*)object_data;
    static int msgbuf[3]= {PVM3D_MESH, 0, 0};
    ger_debug("pvm_ren_mthd: ren_mesh");
    check_attrs(self);
    msgbuf[1]= data->nfacets;
    msgbuf[2]= data->nindices;
    if (pvm_pkint(msgbuf,3,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_mesh: error in pvm_pkint");
    if (pvm_pkint(data->facet_lengths, data->nfacets, 1) < 0)
      pvm_perror("pvm_ren_mthd: ren_mesh: error in pvm_pkint");
    if (pvm_pkint(data->indices, data->nindices, 1) < 0)
      pvm_perror("pvm_ren_mthd: ren_mesh: error in pvm_pkint");
    pack_cached_vlist( self, data->cached_vlist );
  }

  METHOD_OUT;
}

static void destroy_mesh( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Mesh* data= (P_Cached_Mesh*)object_data;
    ger_debug("pvm_ren_mthd: destroy_mesh");
    free( (P_Void_ptr)(data->facet_lengths) );
    free( (P_Void_ptr)(data->indices) );
    free_cached_vlist( (P_Void_ptr)(data->cached_vlist) );
    free( (P_Void_ptr)data );
  }

  METHOD_OUT;
}

static P_Void_ptr def_text( char *name, char *tstring, P_Point *location, 
                           P_Vector *u, P_Vector *v )
/* This method defines a text string */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    P_Cached_Text* result;
    ger_debug("pvm_ren_mthd: def_text");

    if ( !(result= (P_Cached_Text*)malloc(sizeof(P_Cached_Text))) ) {
      fprintf(stderr,"pvm_ren_mthd: def_text: unable to allocate %d bytes!\n",
	      sizeof(P_Cached_Text));
      exit(-1);
    }

    if ( !(result->tstring= (char*)malloc(strlen(tstring)+1)) ) {
      fprintf(stderr,"pvm_ren_mthd: def_text: unable to allocate %d bytes!\n",
	      strlen(tstring)+1);
      exit(-1);
    }
    strcpy(result->tstring,tstring);

    result->coords[0]= location->x;
    result->coords[1]= location->y;
    result->coords[2]= location->z;
    result->coords[3]= u->x;
    result->coords[4]= u->y;
    result->coords[5]= u->z;
    result->coords[6]= v->x;
    result->coords[7]= v->y;
    result->coords[8]= v->z;

    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_text(P_Void_ptr object_data, P_Transform *transform,
		       P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Text* data= (P_Cached_Text*)object_data;
    P_Transform* current_trans;
    static int msgbuf[2]= {PVM3D_TEXT, 0};
    ger_debug("pvm_ren_mthd: ren_text");
    check_attrs(self);
    msgbuf[1]= strlen( data->tstring );
    if (pvm_pkint(msgbuf,2,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_text: error in pvm_pkint");
    if (pvm_pkstr(data->tstring) < 0)
      pvm_perror("pvm_ren_mthd: ren_text: error in pvm_pkstr");
    if (pvm_pkfloat(data->coords,9,1) < 0)
      pvm_perror("pvm_ren_mthd: ren_text: error in pvm_pkfloat");
    current_trans= (*(ASSIST(self)->get_trans))();
    if (pvm_pkfloat(current_trans->d, 12, 1)<0)
      pvm_perror("pvm_ren_mthd: ren_cylinder: error in pvm_pkfloat");
  }

  METHOD_OUT;
}

static void destroy_text( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Text* data= (P_Cached_Text*)object_data;
    ger_debug("pvm_ren_mthd: destroy_text");
    free( (P_Void_ptr)(data->tstring) );
    free( (P_Void_ptr)data );
  }

  METHOD_OUT;
}

static P_Void_ptr def_light( char *name, P_Point *location, P_Color *color )
/* This method defines a positional light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: def_light");
    rgbify_color(color);
    /* do something */
    METHOD_OUT
    return( (P_Void_ptr)0 );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_light(P_Void_ptr object_data, P_Transform *transform,
		       P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: ren_light");
    check_attrs(self);
    /* do something */
  }

  METHOD_OUT;
}

static void traverse_light(P_Void_ptr object_data, P_Transform *transform,
			   P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: traverse_light");
    /* do something */
  }

  METHOD_OUT
}

static void destroy_light( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: destroy_text");
    /* do something */
  }

  METHOD_OUT;
}

static P_Void_ptr def_ambient( char *name, P_Color *color )
/* This method defines an ambient light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: def_ambient");
    rgbify_color(color);
    /* do something */
    METHOD_OUT
    return( (P_Void_ptr)0 );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_ambient(P_Void_ptr object_data, P_Transform *transform,
			P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: ren_ambient");
    check_attrs(self);
    /* do something */
  }

  METHOD_OUT;
}

static void traverse_ambient(P_Void_ptr object_data, P_Transform *transform,
			     P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null
   * and PVM is already in the right state to pack data.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: traverse_ambient");
    /* do something */
  }

  METHOD_OUT
}

static void destroy_ambient( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: destroy_ambient");
    /* do something */
  }

  METHOD_OUT;
}

static P_Void_ptr def_gob( char *name, P_Gob *gob )
/* This method defines a gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  char *kidname;
  P_Gob_List *kids;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: def_gob");
    METHOD_OUT
    return( (P_Void_ptr)gob ); /* Stash the gob where we can get it later */
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

void pg_pvm_ren_retransmit() /* backdoor useful in debugging */
{
  int oldbuf;
  
  /* Resend the most recent PVM message */
  if (!most_recent_send_buf) {
    fprintf(stderr,"pg_pvm_ren_retransmit: no most recent buffer!\n");
    return;
  }
  
  /* get synched if necessary */
  if (!INSTANCE(most_recent_self) && FRAME_NUMBER(most_recent_self)) {
    int old_rbuf;
    int sync_rbuf;
#ifdef CRAY
    long wc_sync_start;
#endif
    if ((old_rbuf= pvm_setrbuf(0))<0)
      pvm_perror("pg_pvm_ren_retransmit: error in pvm_setrbuf 1");
#ifdef CRAY
    wc_sync_start= rtclock();
#endif
    if (pvm_recv(-1, CLIENT_SYNC_MSG)<0)
      pvm_perror("pg_pvm_ren_retransmit: error in pvm_recv");
#ifdef CRAY
    pg_pvm_sync_clocks += (rtclock() - wc_sync_start);
#endif
    if ((sync_rbuf= pvm_setrbuf(old_rbuf))<0)
      pvm_perror("pg_pvm_ren_retransmit: error in pvm_setrbuf 2");
    if (pvm_freebuf(sync_rbuf)<0)
      pvm_perror("pg_pvm_ren_retransmit: error in pvm_freebuf");
  }
  
  /* resend last message */
  if ((oldbuf= pvm_setsbuf(most_recent_send_buf)) < 0) {
    pvm_perror("pg_pvm_ren_retransmit: error in pvm_setsbuf");
    return;
  }
  if ( pvm_send(RENSERVER_TID(most_recent_self), PVM3D_DRAW) < 0 )
    pvm_perror("pg_pvm_ren_retransmit: error on send");
  if (pvm_setsbuf(oldbuf) < 0)
    pvm_perror("pg_pvm_ren_retransmit: error resetting in pvm_setsbuf");
  
  FRAME_NUMBER(most_recent_self)++;
}

static void ren_gob( P_Void_ptr primdata, P_Transform *trans, 
		    P_Attrib_List *attr )
{
  P_Renderer *self= (P_Renderer *)po_this;
  int newbuf, oldbuf;
  METHOD_IN

  if (RENDATA(self)->open) {
    P_Gob* thisgob= (P_Gob*)primdata;
    P_Gob_List* kidlist;
    ger_debug("pvm_ren_mthd: ren_gob:");

    /* if transform is non-null, this is a top-level call and we have
     * to do transform and attribute management.
     */

    if (trans) {
      static int strlenbuf;
      static int fmnumbuf;

      most_recent_self= self;
      if (most_recent_send_buf && (pvm_freebuf(most_recent_send_buf) < 0))
	pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_freebuf");
      if ((newbuf=pvm_mkbuf(PvmDataDefault)) < 0)
	pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_mkbuf");
      if ((oldbuf= pvm_setsbuf(newbuf)) < 0)
	pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_setsbuf");
      most_recent_send_buf= newbuf; /* global, for debugging use */

      strlenbuf= strlen(NAME(self));
      if (pvm_pkint(&strlenbuf,1,1) < 0)
	pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_pkint");
      if (pvm_pkstr(NAME(self)) < 0) {
	pvm_perror("pvm_ren_renderer: error in pvm_pkstr");
	exit(-1);
      }
      fmnumbuf= FRAME_NUMBER(self);
      if (pvm_pkint(&fmnumbuf,1,1) < 0)
	pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_pkint 2");

      /* Send the camera, if there is one */
      if (CURRENT_CAMERA(self)) pack_camera(self);

      ATTRS_SET(self)= 0;
      METHOD_RDY( ASSIST(self) );
      (*(ASSIST(self)->push_attributes))( attr );
      (void)(*(ASSIST(self)->push_trans))( trans );
    }

    /* Traverse the children */
    if (thisgob->attr) {
      METHOD_RDY( ASSIST(self) );
      (*(ASSIST(self)->push_attributes))( thisgob->attr );
    }
    if (thisgob->has_transform) {
      METHOD_RDY( ASSIST(self) );
      (void)(*(ASSIST(self)->push_trans))( &(thisgob->trans) );
    }
    kidlist= thisgob->children;
    while (kidlist) {
      METHOD_RDY(kidlist->gob);
      (*(kidlist->gob->render_to_ren))(self, (P_Transform *)0,
				       (P_Attrib_List *)0 );
      kidlist= kidlist->next;
    }
    if (thisgob->attr) {
      METHOD_RDY( ASSIST(self) );
      (*(ASSIST(self)->pop_attributes))( thisgob->attr );
    }
    if (thisgob->has_transform) {
      METHOD_RDY( ASSIST(self) );
      (void)(*(ASSIST(self)->pop_trans))();
    }

    /* Do clean-up for top-level gobs.  If this is not the first
     * frame of geometry sent, and this is the first instance of
     * the renderer, wait for an incoming sync message from the
     * server.
     */
    if (trans) {
      static int endfmbuf= PVM3D_ENDFRAME;

      if (!INSTANCE(self) && FRAME_NUMBER(self)) {
#ifdef CRAY
	long wc_sync_start;
#endif
	/* Get synchronized */
	int old_rbuf;
	int sync_rbuf;
	if ((old_rbuf= pvm_setrbuf(0))<0)
	  pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_setrbuf 1");
#ifdef CRAY
	wc_sync_start= rtclock();
#endif
	if (pvm_recv(-1, CLIENT_SYNC_MSG)<0)
	  pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_recv");
#ifdef CRAY
	pg_pvm_sync_clocks += (rtclock() - wc_sync_start);
#endif
	if ((sync_rbuf= pvm_setrbuf(old_rbuf))<0)
	  pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_setrbuf 2");
	if (pvm_freebuf(sync_rbuf)<0)
	  pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_freebuf");
      }

      if (pvm_pkint(&endfmbuf,1,1) < 0)
	pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_pkint");
      if ( pvm_send(RENSERVER_TID(self), PVM3D_DRAW) < 0 )
	pvm_perror("pvm_ren_mthd: ren_gob: error on send");
      pg_pvm_most_recent_renserver_tid= RENSERVER_TID(self);
      if (pvm_setsbuf(oldbuf) < 0)
	pvm_perror("pvm_ren_mthd: ren_gob: error resetting in pvm_setsbuf");

      METHOD_RDY( ASSIST(self) );
      (*(ASSIST(self)->pop_attributes))( attr );
      (void)(*(ASSIST(self)->pop_trans))();

      FRAME_NUMBER(self)++;
    }
  }

  METHOD_OUT
}

static void traverse_gob( P_Void_ptr primdata, P_Transform *trans, 
			    P_Attrib_List *attr )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: traverse_gob:");
    /* do something */
  }

  METHOD_OUT
}

static void destroy_gob( P_Void_ptr primdata )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: destroy_gob");
    /* Nothing to destroy */
  }
  METHOD_OUT
}

static void hold_gob( P_Void_ptr primdata )
/* This routine renders gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: hold_gob");
    /* Nothing to do */
  }
  METHOD_OUT
}

static void unhold_gob( P_Void_ptr primdata )
/* This routine renders gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("pvm_ren_mthd: unhold_gob");
    /* Nothing to do */
  }
  METHOD_OUT
}

static void pvm_init_renderer(P_Renderer *self)
{
  int newbuf, oldbuf;
  int old_route_policy;
  int databuf[2];
  
  TID(self)= pvm_mytid();
  
  /* get tid of rendering server, looping and sleeping if necessary */
  while ((RENSERVER_TID(self)=pvm_gettid(SERVER_GROUP_NAME,0)) <= 0) {
    if (RENSERVER_TID(self)==PvmNoGroup) {
      fprintf(stderr,"pvm_init_renderer: awaiting server startup.\n");
      (void)sleep(10);
    }
    else {
      pvm_perror("pvm_init_renderer: pvm_gettid error");
      exit(-1);
    }
  }

  /* Join the client group, if we are the first instance in this process */
  if (!INSTANCE(self)) {
    if (pvm_joingroup(CLIENT_GROUP_NAME) < 0) {
      pvm_perror("pvm_init_renderer: error in pvm_joingroup");
      exit(-1);
    }
  }
  
  /* send rendering server message, connecting up.  Want to do it
   * using direct routing, and then reset all values to those expected
   * by host program */
  if ((newbuf=pvm_mkbuf( PvmDataDefault )) < 0) {
    pvm_perror("pvm_init_renderer: error in pvm_mkbuf");
    exit(-1);
  }
  if ((oldbuf= pvm_setsbuf(newbuf)) < 0) {
    pvm_perror("pvm_init_renderer: error in pvm_setsbuf");
    exit(-1);
  }
  if ((old_route_policy= pvm_getopt( PvmRoute )) < 0) 
    pvm_perror("pvm_init_renderer: error in pvm_getopt");
  if ( pvm_setopt(PvmRoute, PvmRouteDirect) < 0 ) 
    pvm_perror("pvm_init_renderer: error in pvm_setopt");
  
  databuf[0]= TID(self);
  databuf[1]= strlen(NAME(self));
  if (pvm_pkint( databuf,2,1 ) < 0) {
    pvm_perror("pvm_init_renderer: error in pvm_pkint");
    exit(-1);
  }
  if (pvm_pkstr(NAME(self)) < 0) {
    pvm_perror("pvm_init_renderer: error in pvm_pkstr");
    exit(-1);
  }
  
  if (pvm_send(RENSERVER_TID(self),PVM3D_CONNECT) < 0) {
    pvm_perror("pvm_init_renderer: error in pvm_send");
    exit(-1);
  }
  
  if ( pvm_setopt(PvmRoute, old_route_policy) < 0 )
    pvm_perror("pvm_init_renderer: error resetting in pvm_getopt");
  if (pvm_setsbuf(oldbuf) < 0) {
    pvm_perror("pvm_init_renderer: error resetting in pvm_setsbuf");
    exit(-1);
  }
  if (pvm_freebuf(newbuf) < 0) {
    pvm_perror("pvm_init_renderer: error in pvm_freebuf");
    exit(-1);
  }
}

static void ren_destroy( VOIDLIST )
{
  P_Renderer *self= (P_Renderer *)po_this;
  int newbuf, oldbuf;
  int databuf[2];
  METHOD_IN

  ger_debug("pvm_ren_mthd: ren_destroy");
  if (RENDATA(self)->open) ren_close();

  /* send rendering server message, disconnecting. */
  if ((newbuf=pvm_mkbuf( PvmDataDefault )) < 0)
    pvm_perror("pvm_ren_mthd: ren_destroy: error in pvm_mkbuf");
  if ((oldbuf= pvm_setsbuf(newbuf)) < 0)
    pvm_perror("pvm_ren_mthd: ren_destroy: error in pvm_setsbuf");

  databuf[0]= TID(self);
  databuf[1]= strlen(NAME(self));
  if (pvm_pkint( databuf,2,1 ) < 0)
    pvm_perror("pvm_ren_mthd: ren_destroy: error in pvm_pkint");
  if (pvm_pkstr(NAME(self)) < 0) {
    pvm_perror("pvm_ren_mthd: ren_destroy: error in pvm_pkstr");
    exit(-1);
  }

  /* Get synchronized if necessary */
  if (!INSTANCE(self) && FRAME_NUMBER(self)) {
    int old_rbuf;
    int sync_rbuf;
    if ((old_rbuf= pvm_setrbuf(0))<0)
      pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_setrbuf 1");
    if (pvm_recv(-1, CLIENT_SYNC_MSG)<0)
      pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_recv");
    if ((sync_rbuf= pvm_setrbuf(old_rbuf))<0)
      pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_setrbuf 2");
    if (pvm_freebuf(sync_rbuf)<0)
      pvm_perror("pvm_ren_mthd: ren_gob: error in pvm_freebuf");
  }

  if (pvm_send(RENSERVER_TID(self),PVM3D_DISCONNECT) < 0)
    pvm_perror("pvm_ren_mthd: ren_destroy: error in pvm_send to server");

  if (pvm_setsbuf(oldbuf) < 0)
    pvm_perror("pvm_ren_mthd: ren_destroy: error resetting in pvm_setsbuf");

  if (pvm_freebuf(newbuf) < 0)
    pvm_perror("pvm_ren_mthd: ren_destroy: error in pvm_freebuf");

  /* Leave the client group, if we are the first instance in this process */
  if (!INSTANCE(self)) {
    if (pvm_lvgroup(CLIENT_GROUP_NAME) < 0) {
      pvm_perror("pvm_init_renderer: error in pvm_joingroup");
      exit(-1);
    }
  }
  
  RENDATA(self)->initialized= 0;
  instance_count--;

  free( (P_Void_ptr)RENDATA(self) );
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

P_Renderer *po_create_pvm_renderer( char *device, char *datastr )
/* This routine creates a pvm renderer object */
{
  P_Renderer *self;
  P_Renderer_data *rdata;
  static int sequence_number = 0;

  ger_debug("po_create_pvm_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_pvm_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_pvm_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;

  /* Build the transformation buffer (need only 1) */
  if (!transform_buf) {
    if (!(transform_buf=(float*)malloc(3*TRANSFORM_BUF_TRIPLES*sizeof(float))))
      ger_fatal("po_create_pvm_renderer: unable to allocate %d floats!",
		3*TRANSFORM_BUF_TRIPLES);
  }

  /* Fill out default color map */
  strcpy(default_map.name,"default-map");
  default_map.min= 0.0;
  default_map.max= 1.0;
  default_map.mapfun= default_mapfun;

  /* Fill out public and private object data */
  sprintf(self->name,"pvm%d",sequence_number++);
  rdata->open= 0;  /* renderer created in closed state */
  CUR_MAP(self)= &default_map;

  INSTANCE(self)= instance_count++;
  FRAME_NUMBER(self)= 0;
  if (strlen(device)) {
    if ( !(NAME(self)= (char*)malloc(strlen(device)+1)) ) {
      ger_fatal("po_create_pvm_renderer: unable to allocate %d chars!",
		strlen(device)+1);
    }
    strcpy(NAME(self),device);
  }
  else {
    if ( !(NAME(self)= (char*)malloc(32)) ) {
      ger_fatal("po_create_pvm_renderer: unable to allocate 32 chars!");
    }
    strcpy(NAME(self),"PVM-DrawP3D");
  }

  ATTRS_SET(self)= 0;
  ASSIST(self)= po_create_assist(self);

  CURRENT_CAMERA(self)= 0;

  BACKCULLSYMBOL(self)= create_symbol("backcull");
  TEXTHEIGHTSYMBOL(self)= create_symbol("text-height");
  COLORSYMBOL(self)= create_symbol("color");
  MATERIALSYMBOL(self)= create_symbol("material");
  
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

  self->def_polymarker= def_polything;
  self->ren_polymarker= ren_polymarker;
  self->destroy_polymarker= destroy_polything;

  self->def_polyline= def_polything;
  self->ren_polyline= ren_polyline;
  self->destroy_polyline= destroy_polything;

  self->def_polygon= def_polything;
  self->ren_polygon= ren_polygon;
  self->destroy_polygon= destroy_polything;

  self->def_tristrip= def_polything;
  self->ren_tristrip= ren_tristrip;
  self->destroy_tristrip= destroy_polything;

  self->def_bezier= def_polything;
  self->ren_bezier= ren_bezier;
  self->destroy_bezier= destroy_polything;

  self->def_mesh= def_mesh;
  self->ren_mesh= ren_mesh;
  self->destroy_mesh= destroy_mesh;

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

  /* do pvm stuff */
  pvm_init_renderer(self);
  
  RENDATA(self)->initialized= 1;

  return( self );
}
