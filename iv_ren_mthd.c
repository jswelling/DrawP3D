/****************************************************************************
 * iv_ren_mthd.c
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Joel Welling, Damerlin Thompson
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
/* This module implements the Open Inventor renderer */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "iv_ren_mthd.h"

/* Notes-
 * -File opening shouldn't really happen in camera setting routine
 * -background color is not handled
 */

static Def_list *d_list = NULL;
static int gob_type = LIGHTS;
static char tab_buf[(MAX_TABS*TABSPACE)+1];
static double ambi,spec,shin;
static int shape_type;
static int material_type[60];
static int depth = 0;
static const int diff= 1, trpa= 0;
static const int torus_major_divisions=32;
static const int torus_minor_divisions= 16;
static P_Color default_clr = {P3D_RGB,1,1,1,1};

static void output_torus_indices(P_Renderer *self);
static void output_torus_mesh(P_Renderer *self, float major, float minor);
static char* generate_fname();
static void set_indent(int change);
static char* remove_spaces_from_string( char* string );
static char* check_def(P_Gob *thisgob, FILE *outfile); /* returns use name */
static void del_def_list(void);
static void swap_gob_type(void);
static void renfile_startup(void);
static void output_trans(FILE *outfile,P_Transform *trans);
static void change_curr_mat(int type);
static void output_attrs(P_Renderer *self, P_Attrib_List *attr);
static void output_vlist(P_Renderer *self,P_Cached_Vlist *vlist);
static P_Transform *get_oriented(P_Vector *up,P_Vector *view,P_Vector *start);


/* Space for default color map */
static P_Renderer_Cmap default_map;

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

  ger_debug("iv_ren_mthd: def_cmap");

  if (max == min) {
    ger_error("iv_ren_mthd: def_cmap: max equals min (val is %f)", max );
    METHOD_OUT;
    return( (P_Void_ptr)0 );    
  }

  if ( RENDATA(self)->open ) {
    if ( !(thismap= (P_Renderer_Cmap *)malloc(sizeof(P_Renderer_Cmap))) )
      ger_fatal( "iv_ren_mthd: def_cmap: unable to allocate %d bytes!",
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

  ger_debug("iv_ren_mthd: install_cmap");
  if ( RENDATA(self)->open ) {

    if (mapdata) CUR_MAP(self)= (P_Renderer_Cmap *)mapdata;
    else ger_error("iv_ren_mthd: install_cmap: got null color map data.");

  }

  METHOD_OUT
}

static void destroy_cmap( P_Void_ptr mapdata )
/* This method causes renderer data associated with the given color map
 * to be freed.  It must not refer to "self", because the renderer which
 * created the cmap data may no longer exist.
 */
{
  ger_debug("iv_ren_mthd: destroy_cmap");
  if ( mapdata ) free( mapdata );
}

static P_Void_ptr def_camera( P_Camera *cam )
/* This method defines the given camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("iv_ren_mthd: def_camera");

  /* Just need to set up to find camera info later */

  METHOD_OUT
  return( (P_Void_ptr)cam);
}

static void set_camera( P_Void_ptr primdata )
/* This defines the given camera and makes it the current camera.
 * It also creates a new metafile, and adds the camera info to the file.
 */
{
  P_Renderer *self= (P_Renderer *)po_this;

  P_Camera *cam;
  float fovea;
  P_Vector up,view,start;
  P_Transform *trans = NULL;

  METHOD_IN

  ger_debug("iv_ren_mthd: set_camera");
  CURRENT_CAMERA(self)= cam = (P_Camera*)primdata;

  METHOD_RDY(self);
  renfile_startup();

  fovea = DegtoRad*(cam->fovea);
  view.x = cam->lookat.x -  cam->lookfrom.x;
  view.y = cam->lookat.y -  cam->lookfrom.y;
  view.z = cam->lookat.z -  cam->lookfrom.z;

  up.x = cam->up.x;   up.y = cam->up.y;   up.z = cam->up.z;
  start.x = 0.0;      start.y = 0.0;      start.z = -1.0;

  fprintf(OUTFILE(self),"%sPerspectiveCamera {\n",tab_buf);
  fprintf(OUTFILE(self),"%s   position %g %g %g\n",tab_buf,
	  cam->lookfrom.x,cam->lookfrom.y,cam->lookfrom.z);
  fprintf(OUTFILE(self),"%s   heightAngle %g\n",tab_buf,fovea);
  fprintf(OUTFILE(self),"%s}\n",tab_buf);
  fprintf(OUTFILE(self),"%sTranslation { translation %g %g %g }\n",tab_buf,
	 cam->lookfrom.x,cam->lookfrom.y,cam->lookfrom.z );
  trans = get_oriented(&up,&view,&start);
  if(trans) output_trans(OUTFILE(self),transpose_trans(trans));
  fprintf(OUTFILE(self),"%sTranslation { translation %g %g %g }\n",tab_buf,
	 -(cam->lookfrom.x),-(cam->lookfrom.y),-(cam->lookfrom.z));

  free(trans);

  METHOD_OUT
}

static void destroy_camera( P_Void_ptr primdata )
{
  /* This method must not refer to "self", because the renderer for which
   * the camera was defined may already have been destroyed.
   */
  ger_debug("iv_ren_mthd: destroy_camera");
  /* do something */
}

static void ren_print( VOIDLIST )
/* This is the print method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("iv_ren_mthd: ren_print");
  if ( RENDATA(self)->open ) 
    fprintf(stdout,"RENDERER: IV renderer '%s', open", self->name); 
  else fprintf(stdout,"RENDERER: IV renderer '%s', closed", self->name); 
  METHOD_OUT
}

static void ren_open( VOIDLIST )
/* This is the open method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("iv_ren_mthd: ren_open");
  RENDATA(self)->open= 1;
  METHOD_OUT
}

static void ren_close( VOIDLIST )
/* This is the close method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("iv_ren_mthd: ren_close");
  RENDATA(self)->open= 0;
  METHOD_OUT
}

static P_Void_ptr def_sphere(char *name)
/* This routine defines a sphere */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: def_sphere");
    METHOD_OUT
    return( (P_Void_ptr)0 ); /* no prim data needed for sphere */
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_sphere(P_Void_ptr object_data, P_Transform *transform,
		       P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_sphere");
      

    fprintf(OUTFILE(self), "%sSphere {}\n",tab_buf);

  }

  METHOD_OUT;
}

static void destroy_sphere( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: destroy_cylinder");
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
    ger_debug("iv_ren_mthd: def_cylinder");
    METHOD_OUT
    return( (P_Void_ptr)0 ); /* no prim data needed for cylinder */
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_cylinder(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_cylinder");
      

    fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sTransform {\n",tab_buf);
    fprintf(OUTFILE(self),"%s   translation 0 0 0.5\n",tab_buf);
    fprintf(OUTFILE(self),"%s   rotation 1 0 0 1.5707963\n",tab_buf);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
    fprintf(OUTFILE(self),"%sCylinder { height 1}\n",tab_buf);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

  }

  METHOD_OUT;
}

static void destroy_cylinder( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: destroy_cylinder");
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
    ger_debug("iv_ren_mthd: def_torus");
    if ( !(data= (float*)malloc(2*sizeof(float))) ) {
      fprintf(stderr,"iv_ren_mthd: cannot allocate 2 floats!\n");
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

static void output_torus_indices(P_Renderer *self)
{
  /* This routine outputs the torus indices in the correct order. */

  int ord[4],i,j;
  int rows = torus_major_divisions +1,ele_per_row = torus_minor_divisions +1;

  ger_debug("iv_ren_mthd: output_torus_indices");

  for(i=0;i<rows-1;i++) {
    for(j=0;j<ele_per_row-1;j++) {
      ord[0] = j+ (ele_per_row*(i+1));
      ord[1] = j+ (ele_per_row*(i+1)) +1;
      ord[2] = j+(ele_per_row*i)+1;
      ord[3] = j+(ele_per_row*i);

      fprintf(OUTFILE(self),"%s   %d,%d,%d,%d,-1,\n",tab_buf,ord[0],ord[1],
	      ord[2],ord[3]);
    }
    fprintf(OUTFILE(self),"\n");
  }
}

static void output_torus_mesh(P_Renderer *self, float major, float minor)
{
  /* This routine calculates and outputs the coordinates and normals 
   * to create a mesh in the shape of a torus. */

  int torus_vertices= (torus_major_divisions+1) * (torus_minor_divisions+1);
  float theta= 0.0;
  float dt= 2.0*M_PI/torus_minor_divisions;
  float phi=0.0;
  float dp= 2.0*M_PI/torus_major_divisions;
  int i,j;
  int vcount= 0;
  float coords[561][3],norms[561][3];

  ger_debug("iv_ren_mthd: output_torus_mesh");

  /*  Calculate vertex positions */
  for (i=0; i<=torus_major_divisions; i++) {
    for (j=0; j<=torus_minor_divisions; j++) {
      
      coords[ vcount ][0]=
        ( major + minor*cos(theta) ) * cos(phi);
      coords[ vcount ][1]=
        ( major + minor*cos(theta) ) * sin(phi);
      coords[ vcount ][2]= minor * sin(theta);
      norms[ vcount ][0]= cos(theta)*cos(phi);
      norms[ vcount ][1]= cos(theta)*sin(phi);
      norms[ vcount ][2]= sin(theta);
      vcount++;
      theta += dt;
    }
    phi += dp;
  }
  
  fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
  set_indent(INCREASE);
  fprintf(OUTFILE(self),"%sNormal { vector [\n",tab_buf);
  for(i=0;i < torus_vertices;i++) {
    
    fprintf(OUTFILE(self),"%s   %g %g %g,\n",tab_buf,norms[i][0],
	    norms[i][1],norms[i][2]);
  }
  fprintf(OUTFILE(self),"%s]}\n",tab_buf);
  
  fprintf(OUTFILE(self),"%sCoordinate3 { point [\n",tab_buf);
  for(i=0;i < torus_vertices;i++) {
    
    fprintf(OUTFILE(self),"%s   %g %g %g,\n",tab_buf,coords[i][0],
	    coords[i][1],coords[i][2]);
  }
  fprintf(OUTFILE(self),"%s]}\n",tab_buf);
  fprintf(OUTFILE(self),"%sIndexedFaceSet { coordIndex [\n",tab_buf);
  output_torus_indices(self);
  fprintf(OUTFILE(self),"%s]}\n",tab_buf);
    set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s}\n",tab_buf);
}

static void ren_torus(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  P_Cached_Torus *torus = (P_Cached_Torus *)object_data;

  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_torus");
      

    output_torus_mesh(self,torus->major,torus->minor);

  }

  METHOD_OUT;
}

static void destroy_torus( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: destroy_torus");
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

  ger_debug("iv_ren_mthd: cache_vlist");

  if ( !(result=(P_Cached_Vlist*)malloc(sizeof(P_Cached_Vlist))) ) {
    fprintf(stderr,"iv_ren_mthd: cache_vlist: unable to allocate %d bytes!\n",
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
    fprintf(stderr,"iv_ren_mthd: cache_vlist: got unknown vertex type %d!\n",
	    vlist->type);
  }

  /* Allocate memory as needed */
  if ( !(result->coords= 
	 (float*)malloc( 3*result->length*sizeof(float) )) ) {
    fprintf(stderr,"iv_ren_mthd: cannot allocate %d bytes!\n",
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
      fprintf(stderr,"iv_ren_mthd: cannot allocate %d bytes!\n",
	      3*result->length*sizeof(float));
      exit(-1);
    }
    if ( !(result->opacities= 
	   (float*)malloc( result->length*sizeof(float) )) ) {
      fprintf(stderr,"iv_ren_mthd: cannot allocate %d bytes!\n",
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
      fprintf(stderr,"iv_ren_mthd: cannot allocate %d bytes!\n",
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
    ger_debug("iv_ren_mthd: def_polything");

    METHOD_RDY(vlist)
    cache_data= (P_Void_ptr)cache_vlist(self, vlist);
    METHOD_OUT
    return( cache_data );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_bezier(char *name, P_Vlist *vlist)
/* This routine defines a bezier */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr cache_data;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: def_bezier");

    METHOD_RDY(ASSIST(self))
      cache_data = (*(ASSIST(self)->def_bezier))(vlist);
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
    ger_debug("iv_ren_mthd: destroy_polything");
    free_cached_vlist( (P_Cached_Vlist*)object_data );
  }

  METHOD_OUT;
}

static void ren_polymarker(P_Void_ptr object_data, P_Transform *transform,
			   P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  P_Cached_Vlist *vlist = (P_Cached_Vlist *)object_data;

  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_polymarker");

    fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
    set_indent(INCREASE);
    shape_type = NON_INDEXED;
    output_vlist(self,vlist);
    fprintf(OUTFILE(self),"%sPointSet { numPoints %d }\n",tab_buf,
	    vlist->length);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

  }

  METHOD_OUT;
}

static void ren_polyline(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  P_Cached_Vlist *vlist = (P_Cached_Vlist *)object_data;
  int i;

  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_polyline");

    fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
    set_indent(INCREASE);
    shape_type = INDEXED;
    output_vlist(self,vlist);
    fprintf(OUTFILE(self),"%sIndexedLineSet { coordIndex[\n%s   ",
	    tab_buf,tab_buf);
    for(i=0;i<vlist->length;i++) {
      fprintf(OUTFILE(self),"%d,",i);
    }
    fprintf(OUTFILE(self),"-1,\n%s]}\n",tab_buf);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

  }
  METHOD_OUT;
}

static void ren_polygon(P_Void_ptr object_data, P_Transform *transform,
			P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  P_Cached_Vlist *vlist = (P_Cached_Vlist *)object_data;
  int i;

  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_polygon");     

    fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
    set_indent(INCREASE);
    shape_type = INDEXED;
    output_vlist(self,vlist);
    fprintf(OUTFILE(self),"%sIndexedFaceSet { coordIndex[\n%s   ",
	    tab_buf,tab_buf);
    for(i=0;i<vlist->length;i++) {
      fprintf(OUTFILE(self),"%d,",i);
    }
    fprintf(OUTFILE(self),"-1,\n%s]}\n",tab_buf);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

  }

  METHOD_OUT;
}

static void ren_tristrip(P_Void_ptr object_data, P_Transform *transform,
			 P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  P_Cached_Vlist *vlist = (P_Cached_Vlist *)object_data;
  int i;

  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_tristrip");
     

    fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
    set_indent(INCREASE);
    shape_type = INDEXED;
    output_vlist(self,vlist);
    fprintf(OUTFILE(self),"%sIndexedFaceSet { coordIndex[\n",tab_buf);
    for(i=0;i< vlist->length-2;i++) {
      if(i % 2 == 0) {
	fprintf(OUTFILE(self),"%s   %d,%d,%d,-1,\n",tab_buf,i,i+1,i+2);
      } else fprintf(OUTFILE(self),"%s   %d,%d,%d,-1,\n",tab_buf,i,i+2,i+1);
    }
    fprintf(OUTFILE(self),"%s]}\n",tab_buf);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

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
    ger_debug("iv_ren_mthd: def_mesh");

    if ( !(result= (P_Cached_Mesh*)malloc(sizeof(P_Cached_Mesh))) ) {
      fprintf(stderr,"iv_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
	      sizeof(P_Cached_Mesh));
      exit(-1);
    }
    if ( !(result->facet_lengths=(int*)malloc(nfacets*sizeof(int))) ) {
      fprintf(stderr,"iv_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
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
      fprintf(stderr,"iv_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
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
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Mesh* data= (P_Cached_Mesh*)object_data;

    P_Cached_Vlist *vlist = data->cached_vlist;
    int i,j,k=0;

    ger_debug("iv_ren_mthd: ren_mesh");
     

    fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
    set_indent(INCREASE);
    shape_type = INDEXED;
    output_vlist(self,vlist);
    fprintf(OUTFILE(self),"%sIndexedFaceSet { coordIndex [\n",tab_buf);
    for(i=0;i < data->nfacets && k < data->nindices;i++) {
      fprintf(OUTFILE(self),"%s   ",tab_buf);
      for(j=0;j < data->facet_lengths[i];j++) {
	fprintf(OUTFILE(self),"%d, ",data->indices[k]);   
	k++;
      }
      fprintf(OUTFILE(self),"-1,\n");
    }
    fprintf(OUTFILE(self),"%s]}\n",tab_buf);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

  }
  METHOD_OUT;
}

static void destroy_mesh( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Mesh* data= (P_Cached_Mesh*)object_data;
    ger_debug("iv_ren_mthd: destroy_mesh");
    free( (P_Void_ptr)(data->facet_lengths) );
    free( (P_Void_ptr)(data->indices) );
    free_cached_vlist( (P_Cached_Vlist*)(data->cached_vlist) );
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
    ger_debug("iv_ren_mthd: def_text");

    if ( !(result= (P_Cached_Text*)malloc(sizeof(P_Cached_Text))) ) {
      fprintf(stderr,"iv_ren_mthd: def_text: unable to allocate %d bytes!\n",
	      sizeof(P_Cached_Text));
      exit(-1);
    }

    if ( !(result->tstring= (char*)malloc(strlen(tstring)+1)) ) {
      fprintf(stderr,"iv_ren_mthd: def_text: unable to allocate %d bytes!\n",
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
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  P_Vector up,view,start;

  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Text* data= (P_Cached_Text *)object_data;
    ger_debug("iv_ren_mthd: ren_text");
     

    start.x = 1.0; start.y = 0.0; start.z = 0.0;
    view.x = data->coords[3]; view.y = data->coords[4];
    view.z = data->coords[5];
    up.x = data->coords[6];  up.y = data->coords[7]; up.z = data->coords[8];
    transform = get_oriented(&up,&view,&start); 
    transform->d[3] = data->coords[0]; transform->d[7] = data->coords[1];
    transform->d[11] = data->coords[2];

    fprintf(OUTFILE(self),"%sSeparator {\n",tab_buf);
    set_indent(INCREASE);
    if(transform) output_trans(OUTFILE(self),transform); 
    fprintf(OUTFILE(self),"%sText3 {\n",tab_buf);
    fprintf(OUTFILE(self),"%s   parts FRONT\n",tab_buf);
    fprintf(OUTFILE(self),"%s   string \"%s\" }\n",tab_buf,data->tstring);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

  }
  METHOD_OUT;
}

static void destroy_text( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    P_Cached_Text* data= (P_Cached_Text*)object_data;
    ger_debug("iv_ren_mthd: destroy_text");
    free( (P_Void_ptr)(data->tstring) );
    free( (P_Void_ptr)data );
  }

  METHOD_OUT;
}

static P_Void_ptr def_light( char *name, P_Point *location, P_Color *color )
/* This method defines a positional light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  
  P_Cached_Light *result;
  
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: def_light");
    rgbify_color(color);
    /* do something */
    
    if ( !(result= (P_Cached_Light*)malloc(sizeof(P_Cached_Light))) ) {
      fprintf(stderr,"iv_ren_mthd: def_light: unable to allocate %d bytes!\n",
	      sizeof(P_Cached_Light));
      exit(-1);
    }
    result->location.x = location->x;
    result->location.y = location->y;
    result->location.z = location->z;

    result->color.r = color->r;
    result->color.g = color->g;
    result->color.b = color->b;
    
    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_light(P_Void_ptr object_data, P_Transform *transform,
		       P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  P_Cached_Light *data = (P_Cached_Light *)object_data;


  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_light");

    fprintf(OUTFILE(self),"%sPointLight {\n",tab_buf);
    fprintf(OUTFILE(self),"%s   location %g %g %g\n",
	    tab_buf,data->location.x,data->location.y,data->location.z);
    fprintf(OUTFILE(self),"%s   color %g %g %g\n",tab_buf,data->color.r,
	    data->color.g,data->color.b);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);

  }
  METHOD_OUT;
}

static void traverse_light(P_Void_ptr object_data, P_Transform *transform,
			   P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;

  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: traverse_light");
    /* do something */
  }
  METHOD_OUT
}

static void destroy_light( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: destroy_light");
    free( object_data );
  }
  METHOD_OUT;
}

static P_Void_ptr def_ambient( char *name, P_Color *color )
/* This method defines an ambient light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: def_ambient");
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
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: ren_ambient");
      
    /* do something */
  }
  METHOD_OUT;
}

static void traverse_ambient(P_Void_ptr object_data, P_Transform *transform,
			     P_Attrib_List *attrs)
{
  /* This is a primitive, so transform and attrs are guaranteed null.
   */
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: traverse_ambient");
    /* do something */
  }

  METHOD_OUT
}

static void destroy_ambient( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: destroy_ambient");
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
    ger_debug("iv_ren_mthd: def_gob");
    METHOD_OUT
    return( (P_Void_ptr)gob ); /* Stash the gob where we can get it later */
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_gob( P_Void_ptr primdata, P_Transform *trans, 
		    P_Attrib_List *attr )
{
  P_Renderer *self= (P_Renderer *)po_this;
  int newbuf, oldbuf;
  char* use_name;

  METHOD_IN

  if (RENDATA(self)->open) {
    P_Gob* thisgob= (P_Gob*)primdata;
    P_Gob_List* kidlist;
    ger_debug("iv_ren_mthd: ren_gob:");

    if( use_name= check_def(thisgob,OUTFILE(self)) ) {
      fprintf(OUTFILE(self), "%sUSE %s\n",tab_buf,use_name);
      return;
    }
    depth++;
    material_type[depth] = material_type[depth-1];

    /* Last call to check_def defined gob, so now get its use name */
    use_name= check_def(thisgob,OUTFILE(self));
    
    /*  I should print out the attribute and transform info before I reach the 
     *  children. If this is a light gob, then I don't want to use a Separator
     *  and I don't want to indent. 
     */
    if(gob_type == MODEL) {
      if (thisgob->name && strcmp(thisgob->name,"") != 0) {
	fprintf(OUTFILE(self), "%sDEF %s Separator {\n", tab_buf,
		use_name);
      } else {
	fprintf(OUTFILE(self), "%sSeparator {\n",tab_buf);
      }
      set_indent(INCREASE);
    }
    output_attrs(self,thisgob->attr);

    /* Traverse the children */

    if (thisgob->has_transform) {
      output_trans(OUTFILE(self),&(thisgob->trans));
    }

    kidlist= thisgob->children;
    if(kidlist) {
      while(kidlist->next) {
	kidlist = kidlist->next;
      }
    }
    /* kidlist is now at the end of the list */

    while (kidlist) {
      METHOD_RDY(kidlist->gob);
      (*(kidlist->gob->render_to_ren))(self, (P_Transform *)0,
				       (P_Attrib_List *)0 );
      kidlist= kidlist->prev;
    }
    /* All the kids are done. It's time to un-indent and add closing
     * bracket.
     */
    if(gob_type == MODEL) {
      set_indent(DECREASE);
      fprintf(OUTFILE(self), "%s}\n", tab_buf);
    }

    /* Do clean-up for top-level gobs. */
    if (trans) {

      if(gob_type == MODEL) {

	if ( fclose(OUTFILE(self)) == EOF ) {
	  perror("iv_ren_mthd: ren_gob:");
	  ger_fatal("iv_ren_mthd: ren_gob: Error closing file <%s>.",
		  OUTFILE(self) );
	}
	del_def_list();
      }
      swap_gob_type();
    }
    depth--;
  }

  METHOD_OUT
}

static void traverse_gob( P_Void_ptr primdata, P_Transform *trans, 
			    P_Attrib_List *attr )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: traverse_gob:");

    METHOD_RDY(self);
    gob_type = LIGHTS;
    self->ren_gob(primdata,trans,attr);

  }
  METHOD_OUT
}

static void destroy_gob( P_Void_ptr primdata )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("iv_ren_mthd: destroy_gob");
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
    ger_debug("iv_ren_mthd: hold_gob");
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
    ger_debug("iv_ren_mthd: unhold_gob");
    /* Nothing to do */
  }
  METHOD_OUT
}

static void ren_destroy( VOIDLIST )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("iv_ren_mthd: ren_destroy");
  if (RENDATA(self)->open) ren_close();
  RENDATA(self)->initialized= 0;

  METHOD_RDY(ASSIST(self));
  (*(ASSIST(self)->destroy_self))();

  free( (P_Void_ptr)NAME(self) );
  free( (P_Void_ptr)RENDATA(self) );
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

P_Renderer *po_create_iv_renderer( char *device, char *datastr )
/* This routine creates a iv renderer object */
{
  P_Renderer *self;
  P_Renderer_data *rdata;
  static int sequence_number = 0;

  ger_debug("po_create_iv_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_iv_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_iv_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;

  /* Fill out default color map */
  strcpy(default_map.name,"default-map");
  default_map.min= 0.0;
  default_map.max= 1.0;
  default_map.mapfun= default_mapfun;

  /* Fill out public and private object data */
  sprintf(self->name,"iv%d",sequence_number++);
  rdata->open= 0;  /* renderer created in closed state */

  CUR_MAP(self)= &default_map;

  if (strlen(device)) {
    if ( !(NAME(self)= (char*)malloc(strlen(device)+1)) ) {
      ger_fatal("po_create_iv_renderer: unable to allocate %d chars!",
		strlen(device)+1);
    }
    strcpy(NAME(self),device);
  }
  else {
    if ( !(NAME(self)= (char*)malloc(32)) ) {
      ger_fatal("po_create_iv_renderer: unable to allocate 32 chars!");
    }
    strcpy(NAME(self),"IV-DrawP3D");
  }

  if ( !strcmp(device,"-") ) OUTFILE(self)= stdout;
  else OUTFILE(self)= NULL;
  FILENUM(self) = 0;
 
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

  self->def_bezier= def_bezier;
  self->ren_bezier= ren_mesh;
  self->destroy_bezier= destroy_mesh;

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

  RENDATA(self)->initialized= 1;
  material_type[0] = P3D_DEFAULT_MATERIAL;

  return( self );
}

static void set_indent(int change)
{
  /* This routine sets the tab indentation. */

  static int t_steps = 0;
  int i;

  if(change == INCREASE) {
    for(i=0;i<TABSPACE;i++) tab_buf[t_steps*TABSPACE + i] = ' ';
    tab_buf[(t_steps+1)*TABSPACE] = '\0';
    t_steps++;
  } else {
    t_steps--;
    tab_buf[(t_steps)*TABSPACE] = '\0';
  }
}

static char* remove_spaces_from_string( char* string )
{
  char* result= (char*)malloc(strlen(string)+1);
  char* irunner= string;
  char* orunner= result;
  while (*irunner) {
    if (isspace(*irunner)) *orunner++= '_';
    else *orunner++= *irunner;
    irunner++;
  }
  *orunner= 0;
  return result;
}

static char* check_def(P_Gob *thisgob, FILE *outfile)
{
  /* This routine checks to see if 'thisgob' has been used before in
   * this file. If so, its use name is returned;  otherwise returns NULL */

  Def_list *temp_list = d_list, *new_def;
  
  ger_debug("iv_ren_mthd: check_def");
  while(temp_list) {

    if( thisgob == temp_list->gob ) {
      return temp_list->use_name;
    }
    temp_list = temp_list->next;
  }
  /* If you've made it here, it's not in the list */

  if ( !(new_def= (Def_list *)malloc(sizeof(Def_list))) )
    ger_fatal( "iv_ren_mthd: check_def: unable to allocate %d bytes!",
	       sizeof(Def_list) );
  new_def->next = d_list;
  new_def->gob = thisgob;
  new_def->use_name= remove_spaces_from_string(thisgob->name);
  d_list = new_def;
  
  return NULL;
}

static void del_def_list(void)
{
  /* This routine deletes the list of defined gobs just before the
     current file is closed. */

  Def_list *temp_list;
 
  ger_debug("iv_ren_mthd: del_def_list");

  while(d_list) {
    temp_list = d_list->next;
    free(d_list->use_name);
    free(d_list);
    d_list = temp_list;
  }
}

static void swap_gob_type(void)
{
  /* Self-explanatory. */

  if(gob_type == LIGHTS) gob_type = MODEL;
  else gob_type = LIGHTS;
}

static char* generate_fname()
{
  /* This routine generates numbered fnames */

  P_Renderer *self = (P_Renderer *)po_this;
  char* result;
  int len;
  int has_index_field= 0;
  int index_field_start= 0;
  int index_field_length= 0;
  char* runner;

  len = strlen(NAME(self));
  result= (char*)malloc(len+32);

  /* Scan for a field like "####" to put the model index in */
  for (runner= NAME(self); *runner; runner++) {
    if (*runner=='#') {
      if (!has_index_field) index_field_start= runner - NAME(self);
      has_index_field= 1;
      index_field_length++;
    }
    else if (has_index_field) break;
  }
  if (index_field_length>10) index_field_length= 10;

  if (has_index_field) {
    char format[32];
    if (index_field_start) strncpy(result,NAME(self),index_field_start);
    sprintf(format,"%%.%dd",index_field_length);
    sprintf(result+index_field_start,format,FILENUM(self));
    strcat(result+index_field_start+index_field_length,
	   NAME(self)+index_field_start+index_field_length);
  }
  else if (FILENUM(self)) {
    /* Find the file extension */
    int has_extension= 0;
    int ext_offset= 0;
    
    runner= NAME(self)+len-1; /* end of string */
    while ((runner > NAME(self)) && (*runner != '/')) {
      if (*runner=='.') {
	has_extension= 1;
	ext_offset= runner-NAME(self);
	break;
      }
      runner--;
    }
    if (has_extension) {
      strncpy(result, NAME(self), ext_offset+1);
      sprintf(result+ext_offset+1,"%.4d",FILENUM(self));
      strncat(result, NAME(self)+ext_offset);
    }
    else {
      strcpy(result, NAME(self));
      sprintf(result+len,".%.4d",FILENUM(self));
    }
  }
  else strcpy(result, NAME(self));

  return result;
}

static void renfile_startup(void)
{
  /* This routine opens up a new file for a snap and includes some 
   * necessary info. */

  P_Renderer *self = (P_Renderer *)po_this;

  ger_debug("iv_ren_mthd: renfile_startup");

  if(OUTFILE(self) != stdout) {
    char* fname;
    fname= generate_fname();
    if ( !(OUTFILE(self)= fopen(fname,"w")) ) {
      perror("set_camera");
      ger_fatal("set_camera: Error opening file <%s> for writing.",
		fname);
    }
    free(fname);
    FILENUM(self)++;
  }

  fprintf(OUTFILE(self),"#Inventor V2.0 ascii\n\n\n");
  fprintf(OUTFILE(self),"ShapeHints {\n");
  fprintf(OUTFILE(self),"   vertexOrdering COUNTERCLOCKWISE\n");
  fprintf(OUTFILE(self),"   shapeType UNKNOWN_SHAPE_TYPE\n");
  fprintf(OUTFILE(self),"}\n");
  fprintf(OUTFILE(self),"Font { size 1 }\n");
  fprintf(OUTFILE(self),"%sMaterial {\n",tab_buf);
  fprintf(OUTFILE(self),"   ambientColor 0.5 0.5 0.5\n");
  fprintf(OUTFILE(self),"   diffuseColor 1 1 1\n");
  fprintf(OUTFILE(self),"   specularColor 0.5 0.5 0.5\n");
  fprintf(OUTFILE(self),"   shininess 0.3\n");
  fprintf(OUTFILE(self),"   transparency 0\n");
  fprintf(OUTFILE(self),"}\n");
}

static void output_trans(FILE *outfile,P_Transform *trans)
{
  /* This routine outputs the transformation matrix, altering it's
   * output to comply with Open Inventor. */

  fprintf(outfile,"%sMatrixTransform {\n",tab_buf);
  fprintf(outfile,"%s   matrix %g %g %g %g\n",tab_buf,trans->d[0],
	  trans->d[4],trans->d[8],trans->d[12]);
  fprintf(outfile,"%s          %g %g %g %g\n",tab_buf,trans->d[1],
	  trans->d[5],trans->d[9],trans->d[13]);
  fprintf(outfile,"%s          %g %g %g %g\n",tab_buf,trans->d[2],
	  trans->d[6],trans->d[10],trans->d[14]);
  fprintf(outfile,"%s          %g %g %g %g\n",tab_buf,trans->d[3],
	  trans->d[7],trans->d[11],trans->d[15]);
  fprintf(outfile,"%s}\n",tab_buf);
}

static void change_curr_mat(int type)
{
  /* This routine is called to set the global variables that
   * specify a material type to the current material type. */

  material_type[depth] = type;

  switch(type) {

  case P3D_DEFAULT_MATERIAL:
    ambi = 0.5;  spec = 0.5;  shin = 0.3;
    break;
  case P3D_DULL_MATERIAL:
    ambi = 0.9;  spec = 0.5;  shin = 0.1;
    break;
  case P3D_SHINY_MATERIAL:
    ambi = 0.5;  spec = 0.5;  shin = 0.5;
    break;
  case P3D_METALLIC_MATERIAL:
    ambi = 0.5;  spec = 0.5;  shin = 0.5;
    break;
  case P3D_MATTE_MATERIAL:
    ambi = 0.1;  spec = 0.9;  shin = 1;
    break;
  case P3D_ALUMINUM_MATERIAL:
    ambi = 1;  spec = 0;  shin = 1;
    break;
  default:
    ambi = 0.5;  spec = 0.5;  shin = 0.3;
    break;
  }
}

static void output_attrs(P_Renderer *self, P_Attrib_List *attr)
{
  /* This routine outputs the attribute list associated with the current gob */

  int do_color = 0;
  P_Color *clr;
  P_Material *mat;
  P_Attrib_List *tmp=attr;

  ger_debug("iv_ren_mthd: output_attrs");
  
  while (tmp) {
    clr = &default_clr;
    switch (attr->type) {
    case P3D_INT: break;
    case P3D_BOOLEAN:
      if(attr->attribute == BACKCULLSYMBOL(self)) {
	fprintf(OUTFILE(self),"%sShapeHints {\n",tab_buf);
	fprintf(OUTFILE(self),"%s   vertexOrdering COUNTERCLOCKWISE\n"
		,tab_buf);
	if(0 == (int *)attr->value)
	fprintf(OUTFILE(self),"%s   shapeType UNKNOWN\n",tab_buf);
	else
	fprintf(OUTFILE(self),"%s   shapeType SOLID\n",tab_buf);
	fprintf(OUTFILE(self),"%s}\n",tab_buf);
      }
      break;
    case P3D_FLOAT: 
      if(attr->attribute == TEXTHEIGHTSYMBOL(self)) {
	fprintf(OUTFILE(self),"%sFont  { size %g }\n",tab_buf,
		*(float *)attr->value);
      }
      break;
    case P3D_STRING: break;
    case P3D_COLOR: 
      clr= (P_Color *)attr->value;
      do_color = 1;
      break;
    case P3D_POINT: break;
    case P3D_VECTOR: break;
    case P3D_TRANSFORM: break;
    case P3D_MATERIAL:
      if(attr->attribute == MATERIALSYMBOL(self)) {
	mat= (P_Material *)attr->value;
	change_curr_mat(mat->type);
      }
      break;
    case P3D_OTHER: break;
    }
    tmp= tmp->next;
  }

  if(do_color) {
    fprintf(OUTFILE(self),"%sMaterial {\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sambientColor %g %g %g\n",tab_buf,
	   ambi,ambi,ambi);
    fprintf(OUTFILE(self),"%sdiffuseColor %g %g %g\n",tab_buf,
	   clr->r,clr->g,clr->b);
    fprintf(OUTFILE(self),"%sspecularColor %g %g %g\n",tab_buf,
	    spec,spec,spec);
    fprintf(OUTFILE(self),"%sshininess %g\n",tab_buf,shin);
    fprintf(OUTFILE(self),"%stransparency %g\n",tab_buf,(1 - clr->a));
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
  }
}

static void output_vlist(P_Renderer *self,P_Cached_Vlist *vlist)
{
  /* This routine outputs all the information stored in a cached
   * vertex list. */

  int do_color = 0,do_normal = 0,i = 0;

  ger_debug("iv_ren_mthd: output_vlist");

  fprintf(OUTFILE(self),"%sCoordinate3 {\n",tab_buf);
  fprintf(OUTFILE(self),"%s   point [ \n",tab_buf);
  for(i=0;i < vlist->length;i++) {
    fprintf(OUTFILE(self),"%s   %g %g %g,\n",tab_buf,
	    vlist->coords[i*3],vlist->coords[(i*3)+1],
	    vlist->coords[(i*3)+2]);
  }
  fprintf(OUTFILE(self),"%s]} \n",tab_buf);

  switch(vlist->type) {
  case P3D_CVTX:
    break;
  case P3D_CCVTX:
    do_color = 1;
    break;
  case P3D_CNVTX:
    do_normal = 1;
    break;
  case P3D_CCNVTX:
    do_normal = 1;
    do_color = 1;
    break;
  }

  if(do_color) {
    
    if(shape_type == NON_INDEXED) {
      fprintf(OUTFILE(self),"%sMaterialBinding { value PER_VERTEX }\n",
	    tab_buf);
    } else {
      fprintf(OUTFILE(self),
	      "%sMaterialBinding { value PER_VERTEX_INDEXED }\n",tab_buf);
    }

    fprintf(OUTFILE(self),"%sMaterial {\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sambientColor %g %g %g\n",tab_buf,
	    ambi,ambi,ambi);
    fprintf(OUTFILE(self),"%sdiffuseColor [\n",tab_buf);
    
    for(i=0;i < vlist->length;i++) {
    fprintf(OUTFILE(self),"%s   %g %g %g,\n",tab_buf,
	    vlist->colors[i*3],vlist->colors[(i*3)+1],vlist->colors[(i*3)+2]);
    }
    fprintf(OUTFILE(self),"%s   ]\n",tab_buf);
    fprintf(OUTFILE(self),"%sspecularColor %g %g %g\n",tab_buf,
	    spec,spec,spec);
    fprintf(OUTFILE(self),"%sshininess %g\n",tab_buf,shin);
    fprintf(OUTFILE(self),"%stransparency %g\n",tab_buf,trpa);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
  }

  if(do_normal) {
    if(shape_type == NON_INDEXED) {
      fprintf(OUTFILE(self),"%sNormalBinding { value PER_VERTEX }\n",
	    tab_buf);
    } else {
      fprintf(OUTFILE(self),
	      "%sNormalBinding { value PER_VERTEX_INDEXED }\n",tab_buf);
    }
    
    fprintf(OUTFILE(self),"%sNormal { vector [\n",tab_buf);
    for(i=0;i < vlist->length;i++) {
      fprintf(OUTFILE(self),"%s   %g %g %g,\n",tab_buf,
	      vlist->normals[3*i],vlist->normals[3*i+1],
	      vlist->normals[3*i+2]);
    }
    fprintf(OUTFILE(self),"%s]}\n",tab_buf);
  }
}

static P_Transform *get_oriented(P_Vector *up,P_Vector *view,P_Vector *start)
{
  /* This routine returns the transformation necessary to go from
   * the start direction with up as the y-axis, to 'view' direction with 
   * the up direction as 'up'. */

  P_Vector *normup,*rup,y_axis;
  P_Transform *rot,*urot;

  y_axis.x = 0; y_axis.y = 1; y_axis.z = 0;

  ger_debug("iv_ren_mthd: get_oriented");

  rot = make_aligning_rotation(start,view);
  normup = normal_component_wrt(up,view);
  rup = matrix_vector_mult(rot,&y_axis);
  urot = make_aligning_rotation(rup,normup);
  rot = premult_trans(urot,rot);

  free(normup); free(rup); free(urot);
  
  return rot;
}
