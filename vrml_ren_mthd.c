/****************************************************************************
 * vrml_ren_mthd.c
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
/* This module implements the VRML renderer */

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
 */

static char tab_buf[(MAX_TABS*TABSPACE)+1], i_string[40], *fname,
  viewer[30]="\"EXAMINE\" \"WALK\" \"FLY\" \"NONE\"",
  *nogzip="nogzip", *walk="walk", *fly="fly", *none="none";
static int gzip=P3D_TRUE, gob_type=LIGHTS, isFace=P3D_FALSE,
  useEmissive=P3D_FALSE, depth=0, newGob=P3D_TRUE, last_material=0,
  curr_material[MAX_DEPTH], backcull[MAX_DEPTH], transforms[MAX_DEPTH],
  geomIndex[MAX_DEPTH];
static float text_height[MAX_DEPTH];
static double ambi, spec, shin;
static P_Vector y_axis = {0.0, 1.0, 0.0};
static P_Color default_clr = {P3D_RGB,1,1,1,1}, *last_color,
  *curr_color[MAX_DEPTH];
static Def_list *d_list;
static P_Transform_type *combo;

static const int torus_major_divisions=32;
static const int torus_minor_divisions= 16;

static void output_torus_indices(P_Renderer *self);
static void output_torus_mesh(P_Renderer *self, float major, float minor);
static int contains(char *string, char *substring);
static void read_options(char *datastr);
static char* generate_fname();
static void renfile_startup(void);
static void swap_gob_type(void);
static void set_indent(int change);
static void set_index_string(int i);
static char* remove_spaces_from_string( char* string );
static Def_list *check_def(P_Gob *thisgob);
static void del_def_list(void);
static void change_curr_mat(int type);
static void output_curr_appear(P_Renderer *self);
static void output_curr_attrs(P_Renderer *self);
static void set_curr_attrs(P_Renderer *self,P_Attrib_List *attr);
static void output_vlist(P_Renderer *self,P_Cached_Vlist *vlist);
static void get_oriented(P_Vector *up,P_Vector *view,P_Vector *start);
static void print_matrix(float d[]);
static P_Transform_type *get_translation(float d[],P_Transform_type *t_list);
static P_Transform_type *get_rotation(float d[],P_Transform_type *t_list);
static P_Transform_type *get_scale(float d[],P_Transform_type *t_list);
static void remove_neg_scale(float m[3][3], int choice);
static P_Transform_type *get_rot_and_scale(float d[],
					   P_Transform_type *t_list);
static P_Transform_type *reduce_matrix(float d[]);
static void output_trans(FILE *outfile,P_Transform *trans);


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

  ger_debug("vrml_ren_mthd: def_cmap");

  if (max == min) {
    ger_error("vrml_ren_mthd: def_cmap: max equals min (val is %f)", max );
    METHOD_OUT;
    return( (P_Void_ptr)0 );    
  }

  if ( RENDATA(self)->open ) {
    if ( !(thismap= (P_Renderer_Cmap *)malloc(sizeof(P_Renderer_Cmap))) )
      ger_fatal( "vrml_ren_mthd: def_cmap: unable to allocate %d bytes!",
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

  ger_debug("vrml_ren_mthd: install_cmap");
  if ( RENDATA(self)->open ) {

    if (mapdata) CUR_MAP(self)= (P_Renderer_Cmap *)mapdata;
    else ger_error("vrml_ren_mthd: install_cmap: got null color map data.");

  }

  METHOD_OUT
}

static void destroy_cmap( P_Void_ptr mapdata )
/* This method causes renderer data associated with the given color map
 * to be freed.  It must not refer to "self", because the renderer which
 * created the data may already have been destroyed.
 */
{
  ger_debug("vrml_ren_mthd: destroy_cmap");
  if ( mapdata ) free( mapdata );
}

static P_Void_ptr def_camera( P_Camera *cam )
/* This method defines the given camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("vrml_ren_mthd: def_camera");

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
  float fovea,angle;
  P_Vector up,view,start;

  METHOD_IN

  ger_debug("vrml_ren_mthd: set_camera");
  CURRENT_CAMERA(self)= cam = (P_Camera*)primdata;

  METHOD_RDY(self);
  renfile_startup();

  fovea = DegtoRad*(cam->fovea);
  view.x = cam->lookat.x -  cam->lookfrom.x;
  view.y = cam->lookat.y -  cam->lookfrom.y;
  view.z = cam->lookat.z -  cam->lookfrom.z;

  up.x = cam->up.x;   up.y = cam->up.y;   up.z = cam->up.z;
  start.x = 0.0;      start.y = 0.0;      start.z = -1.0;

  fprintf(OUTFILE(self),"%sBackground{skyColor [%g %g %g]}\n",tab_buf,
	  cam->background.r,cam->background.g,cam->background.b);
  
  get_oriented(&up,&view,&start);

  fprintf(OUTFILE(self),"%sViewpoint{#VIEWPOINT\n",tab_buf);
  set_indent(INCREASE);
  fprintf(OUTFILE(self),"%sposition %g %g %g\n",tab_buf,
	  cam->lookfrom.x,cam->lookfrom.y,cam->lookfrom.z);
  if(combo) { 
    fprintf(OUTFILE(self),"%sorientation %g %g %g  %g\n",tab_buf,
	    combo->generators[0],combo->generators[1],
	    combo->generators[2],combo->generators[3]*DegtoRad);
    free(combo);
    combo = NULL;
  }
  fprintf(OUTFILE(self),"%sfieldOfView %g\n",tab_buf,fovea);
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s}\n",tab_buf);

  METHOD_OUT
}

static void destroy_camera( P_Void_ptr primdata )
{
  /* This method must not refer to "self", because the renderer for which
   * the camera was defined may already have been destroyed.
   */
  ger_debug("vrml_ren_mthd: destroy_camera");
  /* do something */
}

static void ren_print( VOIDLIST )
/* This is the print method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("vrml_ren_mthd: ren_print");
  if ( RENDATA(self)->open ) 
    printf("RENDERER: VRML renderer '%s', open", self->name); 
  else printf("RENDERER: VRML renderer '%s', closed", self->name); 
  METHOD_OUT
}

static void ren_open( VOIDLIST )
/* This is the open method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("vrml_ren_mthd: ren_open");
  RENDATA(self)->open= 1;
  METHOD_OUT
}

static void ren_close( VOIDLIST )
/* This is the close method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("vrml_ren_mthd: ren_close");
  RENDATA(self)->open= 0;
  METHOD_OUT
}

static P_Void_ptr def_sphere(char *name)
/* This routine defines a sphere */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("vrml_ren_mthd: def_sphere");
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
    ger_debug("vrml_ren_mthd: ren_sphere");
    
    fprintf(OUTFILE(self),"%sShape{#SPHERE\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sappearance USE APP\n",tab_buf);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE SPH%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF SPH%d Sphere{}\n",tab_buf,
	      geomIndex[depth]);
    }

    geomIndex[depth]++;
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
  }
  
  METHOD_OUT;
}

static void destroy_sphere( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("vrml_ren_mthd: destroy_cylinder");
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
    ger_debug("vrml_ren_mthd: def_cylinder");
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
    ger_debug("vrml_ren_mthd: ren_cylinder");
      
    fprintf(OUTFILE(self),"%sTransform{#CYLINDER\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%stranslation 0 0 0.5\n",tab_buf);
    fprintf(OUTFILE(self),"%srotation 1 0 0 1.5707963\n",tab_buf);
    fprintf(OUTFILE(self),"%schildren [Shape{\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sappearance USE APP\n",tab_buf);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE CYL%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF CYL%d Cylinder{height 1}\n",
	      tab_buf,geomIndex[depth]);
    }

    geomIndex[depth]++;
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}]\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: destroy_cylinder");
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
    ger_debug("vrml_ren_mthd: def_torus");
    if ( !(data= (float*)malloc(2*sizeof(float))) ) {
      fprintf(stderr,"vrml_ren_mthd: cannot allocate 2 floats!\n");
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

  int ord[4],i,j,k;
  int rows = torus_major_divisions +1,ele_per_row = torus_minor_divisions +1;

  ger_debug("vrml_ren_mthd: output_torus_indices");

  k = 0;
  for(i=0;i<rows-1;i++) {
    for(j=0;j<ele_per_row-1;j++) {
      ord[0] = j+ (ele_per_row*(i+1));
      ord[1] = j+ (ele_per_row*(i+1)) +1;
      ord[2] = j+(ele_per_row*i)+1;
      ord[3] = j+(ele_per_row*i);

      if(k % 5==0) set_index_string(k);
      else sprintf(i_string,"");

      fprintf(OUTFILE(self),"%s%d,%d,%d,%d,-1,%s\n",tab_buf,ord[0],ord[1],
	      ord[2],ord[3],i_string);
      k++;
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

  ger_debug("vrml_ren_mthd: output_torus_mesh");

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
  
  fprintf(OUTFILE(self),"%sShape{#TORUS\n",tab_buf);
  set_indent(INCREASE);
  fprintf(OUTFILE(self),"%sappearance USE APP\n",tab_buf);

  if(geomIndex[depth] < geomIndex[depth-1]) {
    fprintf(OUTFILE(self),"%sgeometry USE TOR%d\n",tab_buf,
	    geomIndex[depth]);
    
  } else {
    fprintf(OUTFILE(self),"%sgeometry DEF TOR%d IndexedFaceSet{\n",
	    tab_buf,geomIndex[depth]);
  }
  
  geomIndex[depth]++;
  set_indent(INCREASE);
  fprintf(OUTFILE(self),"%screaseAngle 0.5\n",tab_buf);
  fprintf(OUTFILE(self),"%scoordIndex[\n",tab_buf);
  set_indent(INCREASE);
  output_torus_indices(self);
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s]\n",tab_buf);
  fprintf(OUTFILE(self),"%scoord Coordinate{point[\n",tab_buf);
  set_indent(INCREASE);
  for(i=0;i < torus_vertices;i++) {
    if(i % 5==0) set_index_string(i);
    else sprintf(i_string,"");

    fprintf(OUTFILE(self),"%s%g %g %g,%s\n",tab_buf,coords[i][0],
	    coords[i][1],coords[i][2],i_string);
  }
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s]}\n",tab_buf);
  fprintf(OUTFILE(self),"%snormal Normal{vector[\n",tab_buf);
  set_indent(INCREASE);
  for(i=0;i < torus_vertices;i++) {
    if(i % 5==0) set_index_string(i);
    else sprintf(i_string,"");

    fprintf(OUTFILE(self),"%s%g %g %g,%s\n",tab_buf,norms[i][0],
	    norms[i][1],norms[i][2],i_string);
  }
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s]}\n",tab_buf);
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s}\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: ren_torus");
      

    output_torus_mesh(self,torus->major,torus->minor);

  }

  METHOD_OUT;
}

static void destroy_torus( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("vrml_ren_mthd: destroy_torus");
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

  ger_debug("vrml_ren_mthd: cache_vlist");

  if ( !(result=(P_Cached_Vlist*)malloc(sizeof(P_Cached_Vlist))) ) {
    fprintf(stderr,"vrml_ren_mthd: cache_vlist: unable to allocate %d bytes!\n",
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
    fprintf(stderr,"vrml_ren_mthd: cache_vlist: got unknown vertex type %d!\n",
	    vlist->type);
  }

  /* Allocate memory as needed */
  if ( !(result->coords= 
	 (float*)malloc( 3*result->length*sizeof(float) )) ) {
    fprintf(stderr,"vrml_ren_mthd: cannot allocate %d bytes!\n",
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
      fprintf(stderr,"vrml_ren_mthd: cannot allocate %d bytes!\n",
	      3*result->length*sizeof(float));
      exit(-1);
    }
    if ( !(result->opacities= 
	   (float*)malloc( result->length*sizeof(float) )) ) {
      fprintf(stderr,"vrml_ren_mthd: cannot allocate %d bytes!\n",
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
      fprintf(stderr,"vrml_ren_mthd: cannot allocate %d bytes!\n",
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
    ger_debug("vrml_ren_mthd: def_polything");

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
    ger_debug("vrml_ren_mthd: def_bezier");

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
    ger_debug("vrml_ren_mthd: destroy_polything");
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
    ger_debug("vrml_ren_mthd: ren_polymarker");
    fprintf(OUTFILE(self),"%sShape{#POLYMARKER\n",tab_buf);
    set_indent(INCREASE);
    useEmissive=P3D_TRUE;
    output_curr_appear(self);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE PM%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF PM%d PointSet{\n",
	      tab_buf,geomIndex[depth]);
    }
    
    geomIndex[depth]++;
    set_indent(INCREASE);
    output_vlist(self,vlist);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: ren_polyline");
    
    fprintf(OUTFILE(self),"%sShape{#POLYLINE\n",tab_buf);
    set_indent(INCREASE);
    useEmissive=P3D_TRUE;
    output_curr_appear(self);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE PL%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF PL%d IndexedLineSet{\n",
	      tab_buf,geomIndex[depth]);
    }
    
    geomIndex[depth]++;
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%scoordIndex[",tab_buf);
    for(i=0;i<vlist->length;i++) {
      fprintf(OUTFILE(self),"%d,",i);
    }
    fprintf(OUTFILE(self),"]\n");
    output_vlist(self,vlist);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: ren_polygon");     
    
    fprintf(OUTFILE(self),"%sShape{#POLYGON\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sappearance USE APP\n",tab_buf);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE PG%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF PG%d IndexedFaceSet{\n",
	      tab_buf,geomIndex[depth]);
    }
    
    geomIndex[depth]++;
    set_indent(INCREASE);

    if(backcull[depth]==P3D_FALSE)
      fprintf(OUTFILE(self),"%ssolid FALSE\n",tab_buf);

    fprintf(OUTFILE(self),"%screaseAngle 0.5\n",tab_buf);
    fprintf(OUTFILE(self),"%scoordIndex[",tab_buf);
    for(i=0;i<vlist->length;i++) {
      fprintf(OUTFILE(self),"%d,",i);
    }
    fprintf(OUTFILE(self),"]\n");
    isFace=P3D_TRUE;
    output_vlist(self,vlist);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: ren_tristrip");
    
    fprintf(OUTFILE(self),"%sShape{#TRISTRIP\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sappearance USE APP\n",tab_buf);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE TRI%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF TRI%d IndexedFaceSet{\n",
	      tab_buf,geomIndex[depth]);
    }
    
    geomIndex[depth]++;
    set_indent(INCREASE);

    if(backcull[depth]==P3D_FALSE)
      fprintf(OUTFILE(self),"%ssolid FALSE\n",tab_buf);

    fprintf(OUTFILE(self),"%screaseAngle 0.5\n",tab_buf);
    fprintf(OUTFILE(self),"%scoordIndex[\n",tab_buf);
    set_indent(INCREASE);
    for(i=0;i< vlist->length-2;i++) {
      if(i % 2 == 0) {
	if(i % 5==0) set_index_string(i);
	else sprintf(i_string,"");

	fprintf(OUTFILE(self),"%s%d,%d,%d,-1,%s\n",tab_buf,
		i,i+1,i+2,i_string);
      } else {
	if(i % 5==0) set_index_string(i);
	else sprintf(i_string,"");
	
	fprintf(OUTFILE(self),"%s%d,%d,%d,-1,%s\n",tab_buf,
		i,i+2,i+1,i_string);
      }
    }
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s]\n",tab_buf);
    isFace=P3D_TRUE;
    output_vlist(self,vlist);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: def_mesh");

    if ( !(result= (P_Cached_Mesh*)malloc(sizeof(P_Cached_Mesh))) ) {
      fprintf(stderr,"vrml_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
	      sizeof(P_Cached_Mesh));
      exit(-1);
    }
    if ( !(result->facet_lengths=(int*)malloc(nfacets*sizeof(int))) ) {
      fprintf(stderr,"vrml_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
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
      fprintf(stderr,"vrml_ren_mthd: def_mesh: unable to allocate %d bytes!\n",
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
    int i,j,k,m;

    ger_debug("vrml_ren_mthd: ren_mesh");
     
    fprintf(OUTFILE(self),"%sShape{#MESH\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sappearance USE APP\n",tab_buf);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE MESH%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF MESH%d IndexedFaceSet{\n",
	      tab_buf,geomIndex[depth]);
    }
    
    geomIndex[depth]++;
    set_indent(INCREASE);

    if(backcull[depth]==P3D_FALSE)
      fprintf(OUTFILE(self),"%ssolid FALSE\n",tab_buf);

    fprintf(OUTFILE(self),"%screaseAngle 0.5\n",tab_buf);
    fprintf(OUTFILE(self),"%scoordIndex[\n",tab_buf);
    set_indent(INCREASE);
    k=0; m=0;
    for(i=0;i < data->nfacets && k < data->nindices;i++) {
      fprintf(OUTFILE(self),"%s",tab_buf);
      for(j=0;j < data->facet_lengths[i];j++) {
	fprintf(OUTFILE(self),"%d, ",data->indices[k]);   
	k++;
      }
      if(m % 5==0) set_index_string(m);
      else sprintf(i_string,"");

      fprintf(OUTFILE(self),"-1,%s\n",i_string);
      m++;
    }
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s]\n",tab_buf);
    isFace=P3D_TRUE;
    output_vlist(self,vlist);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: destroy_mesh");
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
    ger_debug("vrml_ren_mthd: def_text");

    if ( !(result= (P_Cached_Text*)malloc(sizeof(P_Cached_Text))) ) {
      fprintf(stderr,"vrml_ren_mthd: def_text: unable to allocate %d bytes!\n",
	      sizeof(P_Cached_Text));
      exit(-1);
    }

    if ( !(result->tstring= (char*)malloc(strlen(tstring)+1)) ) {
      fprintf(stderr,"vrml_ren_mthd: def_text: unable to allocate %d bytes!\n",
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
    double angle;

    ger_debug("vrml_ren_mthd: ren_text");
     
    start.x = 1.0; start.y = 0.0; start.z = 0.0;
    view.x = data->coords[3]; view.y = data->coords[4];
    view.z = data->coords[5];
    up.x = data->coords[6];  up.y = data->coords[7]; up.z = data->coords[8];

    get_oriented(&up,&view,&start);

    fprintf(OUTFILE(self),"%sTransform{#TEXT\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%stranslation %g %g %g\n",tab_buf,
	    data->coords[0],data->coords[1],data->coords[2]);
    if(combo) {
      fprintf(OUTFILE(self),"%srotation %g %g %g  %g\n",tab_buf,
	      combo->generators[0],combo->generators[1],
	      combo->generators[2],combo->generators[3]*DegtoRad);
      free(combo);
      combo = NULL;
    }

    fprintf(OUTFILE(self),"%schildren[Shape{\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sappearance USE APP\n",tab_buf);

    if(geomIndex[depth] < geomIndex[depth-1]) {
      fprintf(OUTFILE(self),"%sgeometry USE TEXT%d\n",tab_buf,
	      geomIndex[depth]);
      
    } else {
      fprintf(OUTFILE(self),"%sgeometry DEF TEXT%d Text{\n",
	      tab_buf,geomIndex[depth]);
    }
    
    geomIndex[depth]++;
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%sfontStyle FontStyle{size %g}\n",
	    tab_buf,text_height[depth]);
    fprintf(OUTFILE(self),"%sstring \"%s\"\n",tab_buf,data->tstring);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}\n",tab_buf);
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s}]\n",tab_buf);
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
    ger_debug("vrml_ren_mthd: destroy_text");
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
    ger_debug("vrml_ren_mthd: def_light");
    rgbify_color(color);
    /* do something */
    
    if ( !(result= (P_Cached_Light*)malloc(sizeof(P_Cached_Light))) ) {
      fprintf(stderr,
	      "vrml_ren_mthd: def_light: unable to allocate %d bytes!\n",
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
    ger_debug("vrml_ren_mthd: ren_light");

    fprintf(OUTFILE(self),"%sPointLight{#LIGHT\n",tab_buf);
    set_indent(INCREASE);
    fprintf(OUTFILE(self),"%slocation %g %g %g\n",tab_buf,
	    data->location.x,data->location.y,data->location.z);
    fprintf(OUTFILE(self),"%scolor %g %g %g\n",tab_buf,data->color.r,
	    data->color.g,data->color.b);
    set_indent(DECREASE);
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
    ger_debug("vrml_ren_mthd: traverse_light");
    /* do something */
  }
  METHOD_OUT
}

static void destroy_light( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("vrml_ren_mthd: destroy_light");
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
    ger_debug("vrml_ren_mthd: def_ambient");
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
    ger_debug("vrml_ren_mthd: ren_ambient");
      
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
    ger_debug("vrml_ren_mthd: traverse_ambient");
    /* do something */
  }

  METHOD_OUT
}

static void destroy_ambient( P_Void_ptr object_data )
{
  P_Renderer *self= (P_Renderer*)po_this;
  METHOD_IN;

  if (RENDATA(self)->open) {
    ger_debug("vrml_ren_mthd: destroy_ambient");
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
    ger_debug("vrml_ren_mthd: def_gob");
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

  METHOD_IN

  if (RENDATA(self)->open) {
    int i,doAPP,len;
    P_Gob* thisgob= (P_Gob*)primdata;
    P_Gob_List* kidlist;
    Def_list *gob_def;
    char remark[P3D_NAMELENGTH+50]="", *command;

    ger_debug("vrml_ren_mthd: ren_gob:");

    gob_def= check_def(thisgob);
        
    /*  I should print out the attribute and transform info before I reach the 
     *  children. If this is a light gob, then I don't want to use a Separator
     *  and I don't want to indent. 
     */
    
    if(gob_type == MODEL) {
      depth++;
      if (depth>=MAX_DEPTH) ger_fatal("vrml_ren_mthd: scene graph too deep!");
      curr_material[depth] = curr_material[depth-1];
      curr_color[depth] = curr_color[depth-1];
      text_height[depth] = text_height[depth-1];
      backcull[depth] = backcull[depth-1];
      transforms[depth]=0;

      if(newGob==P3D_TRUE) {
	geomIndex[depth]=geomIndex[depth-1];
	thisgob->geomIndex=geomIndex[depth];

      } else {
	geomIndex[depth]=thisgob->geomIndex;
      }

      if(newGob==P3D_TRUE) {
	if(strcmp(gob_def->use_name,"") == 0) sprintf(remark,"GOB");
	else sprintf(remark,"GOB %s",gob_def->use_name);

      } else sprintf(remark,"USE GOB %s",gob_def->use_name);

      fprintf(OUTFILE(self),"%sTransform{#%s\n",tab_buf,remark);
      set_indent(INCREASE);

      if (thisgob->has_transform) {
	output_trans(OUTFILE(self),&(thisgob->trans));
      }

      fprintf(OUTFILE(self),"%schildren[\n",tab_buf);
      set_indent(INCREASE);
      
      set_curr_attrs(self,thisgob->attr);

      doAPP=P3D_FALSE;

      if(curr_material[depth] != last_material) {
	last_material=curr_material[depth];
	doAPP=P3D_TRUE;
      }
      
      if(curr_color[depth]->r != last_color->r ||
	 curr_color[depth]->g != last_color->g ||
	 curr_color[depth]->b != last_color->b ||
	 curr_color[depth]->a != last_color->a) {
	last_color=curr_color[depth];
	doAPP=P3D_TRUE;
      }

      if(doAPP) output_curr_attrs(self);
    }
        
    /* Traverse the children */

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
      fprintf(OUTFILE(self), "%s]\n", tab_buf);
      
      for(i=transforms[depth];i>0;i--) {
	set_indent(DECREASE);
	fprintf(OUTFILE(self), "%s}]#Transform\n",tab_buf);
      }
      
      set_indent(DECREASE);
      fprintf(OUTFILE(self), "%s}#End of %s\n",tab_buf,remark);

      if(newGob==P3D_TRUE) geomIndex[depth-1]=geomIndex[depth];
      
      depth--;
    }
    

    /* Do clean-up for top-level gobs. */
    if (trans) {

      if(gob_type == MODEL) {
	int i;
	set_indent(DECREASE);
	fprintf(OUTFILE(self), "%s]}\n", tab_buf);	  
	
	if ( fclose(OUTFILE(self)) == EOF ) {
	  perror("vrml_ren_mthd: ren_gob:");
	  ger_fatal("vrml_ren_mthd: ren_gob: Error closing file <%s>.",
		  OUTFILE(self) );
	}
	
	if(gzip==P3D_TRUE) {
	  len= (strlen(fname)*3)+15;
	  command= (char*)malloc((len)*sizeof(char));
	  sprintf(command,"gzip %s; mv %s.gz %s",fname,fname,fname);
	  if(system(command) != 0)
	    printf("vrml_ren_mthd: ren_gob: %s returned nonzero status",
		   command);
	  free(command);
	}
	
	free(fname);
	del_def_list();
      }
      swap_gob_type();
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
    ger_debug("vrml_ren_mthd: traverse_gob:");

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
    ger_debug("vrml_ren_mthd: destroy_gob");
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
    ger_debug("vrml_ren_mthd: hold_gob");
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
    ger_debug("vrml_ren_mthd: unhold_gob");
    /* Nothing to do */
  }
  METHOD_OUT
}

static void ren_destroy( VOIDLIST )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("vrml_ren_mthd: ren_destroy");
  if (RENDATA(self)->open) ren_close();
  RENDATA(self)->initialized= 0;

  METHOD_RDY(ASSIST(self));
  (*(ASSIST(self)->destroy_self))();

  free( (P_Void_ptr)NAME(self) );
  free( (P_Void_ptr)RENDATA(self) );
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

P_Renderer *po_create_vrml_renderer( char *device, char *datastr )
/* This routine creates a vrml renderer object */
{
  P_Renderer *self;
  P_Renderer_data *rdata;
  static int sequence_number = 0;

  ger_debug("po_create_vrml_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_vrml_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_vrml_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;

  /* Fill out default color map */
  strcpy(default_map.name,"default-map");
  default_map.min= 0.0;
  default_map.max= 1.0;
  default_map.mapfun= default_mapfun;

  /* Fill out public and private object data */
  sprintf(self->name,"vrml%d",sequence_number++);
  rdata->open= 0;  /* renderer created in closed state */

  CUR_MAP(self)= &default_map;

  if (strlen(device)) {
    if ( !(NAME(self)= (char*)malloc(strlen(device)+1)) ) {
      ger_fatal("po_create_vrml_renderer: unable to allocate %d chars!",
		strlen(device)+1);
    }
    strcpy(NAME(self),device);
  }
  else {
    if ( !(NAME(self)= (char*)malloc(32)) ) {
      ger_fatal("po_create_vrml_renderer: unable to allocate 32 chars!");
    }
    strcpy(NAME(self),"VRML-DrawP3D.wrl");
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

  curr_material[0] = P3D_DEFAULT_MATERIAL;
  curr_color[0] = &default_clr;
  text_height[0] = 1;
  backcull[0] = P3D_FALSE;
  transforms[0] = 0;
  geomIndex[0]=0;
  read_options(datastr);

  RENDATA(self)->initialized= 1;
  
  return( self );
}

static int contains(char *string, char *substring)
     /* Returns P3D_TRUE if string contains substring */
{
  char *str;
  int len;
  
  if(substring==NULL || string==NULL) return( P3D_FALSE );

  len= strlen(substring);

  /* substring is bigger than string */
  if(len > strlen(string)) return( P3D_FALSE );

  str=string;
  while(*str) {
    if(strncmp(str++,substring,len) == 0) return( P3D_TRUE );
  }
  
  return( P3D_FALSE );
}

static void read_options(char *datastr)
     /* Sets gzip and viewer options based on datastr */
{
  if(contains(datastr,nogzip) == P3D_TRUE) gzip=P3D_FALSE;
  else gzip=P3D_TRUE;

  if(contains(datastr,walk) == P3D_TRUE)
    sprintf(viewer,"\"WALK\" \"EXAMINE\" \"FLY\" \"NONE\"");
  else if(contains(datastr,fly) == P3D_TRUE)
    sprintf(viewer,"\"FLY\" \"EXAMINE\" \"WALK\" \"NONE\"");
  else if(contains(datastr,none) == P3D_TRUE)
    sprintf(viewer,"\"NONE\" \"EXAMINE\" \"WALK\" \"FLY\"");
  else
    sprintf(viewer,"\"EXAMINE\" \"WALK\" \"FLY\" \"NONE\"");
}

static char* generate_fname()
{
  /* This routine generates numbered fnames */

  P_Renderer *self = (P_Renderer *)po_this;
  char* result;
  int len;
  int has_index_field= P3D_FALSE;
  int index_field_start= 0;
  int index_field_length= 0;
  char* runner;

  ger_debug("vrml_ren_mthd: generate_fname");

  len = strlen(NAME(self));
  result= (char*)malloc(len+32);

  /* Scan for a field like "####" to put the model index in */
  for (runner= NAME(self); *runner; runner++) {
    if (*runner=='#') {
      if (!has_index_field) index_field_start= runner - NAME(self);
      has_index_field= P3D_TRUE;
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
    int has_extension= P3D_FALSE;
    int ext_offset= 0;
    int ext_len= 0;
    
    runner= NAME(self)+len-1; /* end of string */
    while ((runner > NAME(self)) && (*runner != '/')) {
      if (*runner=='.') {
	has_extension= P3D_TRUE;
	ext_offset= runner-NAME(self);
	ext_len= len - ext_offset;
	break;
      }
      runner--;
    }
    if (has_extension) {
      strncpy(result, NAME(self), ext_offset+1);
      sprintf(result+ext_offset+1,"%.4d",FILENUM(self));
      strncat(result, NAME(self)+ext_offset,ext_len);
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

  ger_debug("vrml_ren_mthd: renfile_startup: file #%d",FILENUM(self));

  if(OUTFILE(self) != stdout) {
    fname= generate_fname();
    if ( !(OUTFILE(self)= fopen(fname,"w")) ) {
      perror("set_camera");
      ger_fatal("set_camera: Error opening file <%s> for writing.",
		fname);
    }
    FILENUM(self)++;
  }
  
  last_material=P3D_DEFAULT_MATERIAL;
  last_color=&default_clr;
  geomIndex[0]=0;

  fprintf(OUTFILE(self),"%s#VRML V2.0 utf8\n\n\n",tab_buf);
  fprintf(OUTFILE(self),"%sGroup{children[\n",tab_buf);
  set_indent(INCREASE);
  fprintf(OUTFILE(self),"%sNavigationInfo{speed 2.0 type [%s]}\n",
	  tab_buf,viewer);
  output_curr_attrs(self);
}

static void swap_gob_type(void)
{
  /* Self-explanatory. */

  if(gob_type == LIGHTS) gob_type = MODEL;
  else gob_type = LIGHTS;
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

static void set_index_string(int i)
{
  sprintf(i_string,"#%d",i);
}

static char* remove_spaces_from_string( char* string )
{
  /* Replaces ' ' with '_' in string and returns result */

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

static Def_list *check_def(P_Gob *thisgob)
{
  /* This routine returns the def for thisgob.
   * If it's already in the list, it returns it,
   * otherwise it creates a new def and returns that.
   * It set's newGob to true/false when appropriate. */

  Def_list *temp_list = d_list, *new_def;
  
  ger_debug("vrml_ren_mthd: check_def");

  while(temp_list) {

    if( thisgob == temp_list->gob ) {
      newGob=P3D_FALSE;
      return( temp_list );
    }
    temp_list = temp_list->next;
  }
  /* If you've made it here, it's a new def */

  newGob=P3D_TRUE;
  if ( !(new_def= (Def_list *)malloc(sizeof(Def_list))) )
    ger_fatal( "vrml_ren_mthd: check_def: unable to allocate %d bytes!",
	       sizeof(Def_list) );
  new_def->next = d_list;
  new_def->gob = thisgob;
  new_def->use_name= remove_spaces_from_string(thisgob->name);
  d_list = new_def;
  
  return( new_def );
}

static void del_def_list(void)
{
  /* This routine deletes the list of defined gobs just before the
     current file is closed. */

  Def_list *temp_list;
 
  ger_debug("vrml_ren_mthd: del_def_list");

  while(d_list) {
    temp_list = d_list->next;
    free(d_list->use_name);
    free(d_list);
    d_list = temp_list;
  }
}

static void change_curr_mat(int type)
{
  /* This routine is called to set the global variables that
   * specify a material type to the current material type. */

  ger_debug("vrml_ren_mthd: change_curr_mat");

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

static void output_curr_appear(P_Renderer *self)
  /* This routine outputs the current appearance. */
{
  ger_debug("vrml_ren_mthd: output_curr_appear");

  change_curr_mat(curr_material[depth]);

  fprintf(OUTFILE(self),"%sappearance DEF APP Appearance{\n",tab_buf);
  set_indent(INCREASE);
  fprintf(OUTFILE(self),"%smaterial Material{\n",tab_buf);
  set_indent(INCREASE);
  fprintf(OUTFILE(self),"%sambientIntensity %g\n",tab_buf,ambi);

  if(useEmissive==P3D_TRUE) {
    fprintf(OUTFILE(self),"%semissiveColor %g %g %g\n",tab_buf,
	    curr_color[depth]->r,curr_color[depth]->g,
	    curr_color[depth]->b);

  } else {
    fprintf(OUTFILE(self),"%sdiffuseColor %g %g %g\n",tab_buf,
	    curr_color[depth]->r,curr_color[depth]->g,
	    curr_color[depth]->b);
  }

  fprintf(OUTFILE(self),"%sspecularColor %g %g %g\n",tab_buf,
	  spec,spec,spec);
  fprintf(OUTFILE(self),"%sshininess %g\n",tab_buf,shin); 
  fprintf(OUTFILE(self),"%stransparency %g\n",tab_buf,
	  (1-curr_color[depth]->a));
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s}\n",tab_buf);
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s}\n",tab_buf);
  useEmissive=P3D_FALSE;
}

static void output_curr_attrs(P_Renderer *self)
  /* This routine outputs the current attributes. */
{
  ger_debug("vrml_ren_mthd: output_curr_attrs");

  fprintf(OUTFILE(self),"%sShape{#ATTRUBUTES\n",tab_buf,ambi);
  set_indent(INCREASE);
  output_curr_appear(self);
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s}\n",tab_buf);
}

static void set_curr_attrs(P_Renderer *self, P_Attrib_List *attr)
{
  /* This routine reads the attributes into the appropriate arrays. */

  P_Material *mat;
  P_Attrib_List *tmp = attr;

  ger_debug("vrml_ren_mthd: set_curr_attrs");
  
  while (tmp) {
    switch (tmp->type) {
    case P3D_INT: break;
    case P3D_BOOLEAN:
      if(tmp->attribute == BACKCULLSYMBOL(self)) {
	if(*((int*)tmp->value)==0) backcull[depth]=P3D_FALSE;
	else backcull[depth]=P3D_TRUE;
      }
      break;
    case P3D_FLOAT: 
      if(tmp->attribute == TEXTHEIGHTSYMBOL(self)) {
	text_height[depth] = *((float*)tmp->value);
      }
      break;
    case P3D_STRING: break;
    case P3D_COLOR:
      curr_color[depth] = (P_Color *)tmp->value;
      break;
    case P3D_POINT: break;
    case P3D_VECTOR: break;
    case P3D_TRANSFORM: break;
    case P3D_MATERIAL:
      if(tmp->attribute == MATERIALSYMBOL(self)) {
	mat= (P_Material *)tmp->value;
	curr_material[depth] = mat->type;
      }
      break;
    case P3D_OTHER: break;
    }
    tmp= tmp->next;
  }
}

static void output_vlist(P_Renderer *self,P_Cached_Vlist *vlist)
{
  /* This routine outputs all the information stored in a cached
   * vertex list. */

  int do_color = P3D_FALSE,do_normal = P3D_FALSE,i = 0;

  ger_debug("vrml_ren_mthd: output_vlist");

  fprintf(OUTFILE(self),"%scoord Coordinate{point[\n",tab_buf);
  set_indent(INCREASE);
  for(i=0;i < vlist->length;i++) {
    if(i % 5==0) set_index_string(i);
    else sprintf(i_string,"");

    fprintf(OUTFILE(self),"%s%g %g %g,%s\n",tab_buf,
	    vlist->coords[i*3],vlist->coords[(i*3)+1],
	    vlist->coords[(i*3)+2],i_string);
  }
  set_indent(DECREASE);
  fprintf(OUTFILE(self),"%s]} \n",tab_buf);

  switch(vlist->type) {
  case P3D_CVTX:
    break;
  case P3D_CCVTX:
    do_color = P3D_TRUE;
    break;
  case P3D_CNVTX:
    do_normal = P3D_TRUE;
    break;
  case P3D_CCNVTX:
    do_normal = P3D_TRUE;
    do_color = P3D_TRUE;
    break;
  }

  if(do_color == P3D_TRUE) {
    fprintf(OUTFILE(self),"%scolor Color{color[\n",tab_buf);
    set_indent(INCREASE);
    for(i=0;i < vlist->length;i++) {
      if(i % 5==0) set_index_string(i);
      else sprintf(i_string,"");
      
      fprintf(OUTFILE(self),"%s%g %g %g,%s\n",tab_buf,
	      vlist->colors[i*3],vlist->colors[(i*3)+1],
	      vlist->colors[(i*3)+2],i_string);
    }
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s]}\n",tab_buf);
  }
  
  if(do_normal == P3D_TRUE && isFace == P3D_TRUE) {
    fprintf(OUTFILE(self),"%snormal Normal{vector[\n",tab_buf);
    set_indent(INCREASE);
    for(i=0;i < vlist->length;i++) {
      if(i % 5==0) set_index_string(i);
      else sprintf(i_string,"");
      
      fprintf(OUTFILE(self),"%s%g %g %g,%s\n",tab_buf,
	      vlist->normals[3*i],vlist->normals[3*i+1],
	      vlist->normals[3*i+2],i_string);
    }
    set_indent(DECREASE);
    fprintf(OUTFILE(self),"%s]}\n",tab_buf);
  }

  isFace=P3D_FALSE;
}

static void get_oriented(P_Vector *up,P_Vector *view,P_Vector *start)
{
  /* This routine sets combo to the rotation necessary to go from
   * the start direction with up as the y-axis, to 'view' direction with 
   * the up direction as 'up'. */

  P_Vector *normup,*rstart;
  P_Transform *rot1,*rot2;
  static P_Vector y_vec= { 0.0, 1.0, 0.0 };
  static P_Vector z_vec= { 0.0, 1.0, 0.0 };

  ger_debug("vrml_ren_mthd: get_oriented");

  normup = normal_component_wrt(up,view);
  /* Do something if the user gives a bad set of directions */
  if (normup==NULL) {
    ger_error("vrml_ren_mthd: get_oriented: bad up direction!");
    normup= normal_component_wrt(&y_vec,view);
    if (normup==NULL) normup= normal_component_wrt(&z_vec,view);
    if (normup==NULL) normup= &z_vec;
  }
  rot1 = make_aligning_rotation(&y_axis,normup);
  rstart = matrix_vector_mult(rot1,start);
  rot2 = make_aligning_rotation(rstart,view);
  premult_trans(rot2,rot1);
  combo = get_rotation(rot1->d,(P_Transform_type *)NULL);

  free(normup); free(rstart); free(rot1); free(rot2);
}

static void print_matrix(float d[])
{
  int i,j,fieldWidth=0,len;
  char s[50];

  ger_debug("vrml_ren_mthd: print_matrix");

  for(i=0;i<16;i++) {
    sprintf(s,"%f",d[i]);
    len = strlen(s);
    if(len > fieldWidth) fieldWidth = len;
  }
  
  for(i=0;i<16;i++) {
    if(i % 4==0)
      printf("\n( ");
    printf("%*f ",fieldWidth,d[i]);
    if(i % 4==3)
      printf(")");
  }
  printf("\n\n");
}

static P_Transform_type *get_translation(float d[],P_Transform_type *t_list)
  /* Returns the translation stored in the matrix d.*/
  /* Adds it's type to end of t_list */
{
  P_Transform_type *type;

  ger_debug("vrml_ren_mthd: get_translation");

  type = allocate_trans_type();
  type->type=P3D_TRANSLATION;
  type->generators[0] = d[3];
  type->generators[1] = d[7];
  type->generators[2] = d[11];
  type->generators[3] = 0;
  type->next = NULL;

  return( add_type_to_list(type,t_list)  );
}

static P_Transform_type *get_rotation(float d[],P_Transform_type *t_list)
  /* Returns the rotation stored in the matrix d.*/
  /* Adds it's type to end of t_list */
  /* Assumes a rotation only. */
{
  P_Transform_type *type;
  float trace;
  double sine,cosine;

  ger_debug("vrml_ren_mthd: get_rotation");

  type = allocate_trans_type();
  type->type=P3D_ROTATION;
  type->next = NULL;

  trace = d[0] + d[5] + d[10];
  type->generators[3] = acos((trace-1)/2);

  if(fabs(type->generators[3]) < EPSILON) {
    free(type);
    return( t_list );
  }

  sine = sin(type->generators[3]);
  /* if sine==0, we know the angle is 180, and it doesn't matter
   * whether you use axis or -axis */

  if(sine==0) {
    cosine = cos(type->generators[3]);
    type->generators[0] = sqrt((d[0]-cosine)/(1-cosine));
    type->generators[1] = sqrt((d[5]-cosine)/(1-cosine));
    type->generators[2] = sqrt((d[10]-cosine)/(1-cosine));
  } else {
    type->generators[0] = (d[9] - d[6])/(2*sine);
    type->generators[1] = (d[2] - d[8])/(2*sine);
    type->generators[2] = (d[4] - d[1])/(2*sine);
  }

  if(sqrt(type->generators[0]*type->generators[0] +
	  type->generators[1]*type->generators[1] +
	  type->generators[2]*type->generators[2]) == 0) {
    ger_error("vrml_ren_mthd: get_rotation: axis degenerate; using z axis");
    type->generators[0] = 0;
    type->generators[1] = 0;
    type->generators[2] = 1;
  }

  type->generators[3] = type->generators[3]*RadtoDeg;  

  return( add_type_to_list(type,t_list)  );
}

static P_Transform_type *get_scale(float d[],P_Transform_type *t_list)
  /* Returns the scale stored in the matrix d.*/
  /* Adds it's type to end of t_list. */
  /* Assumes a scale only. */
{
  P_Transform_type *type;

  ger_debug("vrml_ren_mthd: get_scale");

  type = allocate_trans_type();
  type->type=P3D_ASCALE;
  type->generators[0] = d[0];
  type->generators[1] = d[5];
  type->generators[2] = d[10];
  type->generators[3] = 0;
  type->next = NULL;

  return( add_type_to_list(type,t_list)  );
}

static void remove_neg_scale(float m[3][3],int choice)
  /* If a row/column is negative, it negates the row/column. */
{
  int i,j,k;

  ger_debug("vrml_ren_mthd: remove_neg_scale");

  if(choice==ROWS) {
    for(i=0;i<3;i++) {
      k=0;
      for(j=0;j<3;j++) {
	if(m[i][j] < 0 || m[i][j] == -0)
	  k++;
      }
      if(k==3) {
	m[i][0] = -m[i][0];
	m[i][1] = -m[i][1];
	m[i][2] = -m[i][2];
      }
    }
  } else {
    for(j=0;j<3;j++) {
      k=0;
      for(i=0;i<3;i++) {
	if(m[i][j] < 0 || m[i][j] == -0) {
	  k++;
	}
      }
      if(k==3) {
	m[0][j] = -m[0][j];
	m[1][j] = -m[1][j];
	m[2][j] = -m[2][j];
      }
    }
  }
}

static P_Transform_type *get_rot_and_scale(float d[],
					   P_Transform_type *t_list)
  /* Removes a rot and a scale from the matrix. If S2*R*S1
   * occurs, it does nothing. */
{
  int i,j;
  float m[3][3],rowlen[3],collen[3];
  float rotMatrix[16] = {0,};
  P_Transform *trans;

  ger_debug("vrml_ren_mthd: get_rot_and_scale");

  for(i=0;i<3;i++) {
    for(j=0;j<3;j++) {
      m[i][j] = d[i*4 + j];
    }
  }

  /* If you divide row by rowlen and the column lengths are now one,
   * the rowlengths are the scales. The reverse holds true also. */

  rowlen[0] = sqrt(m[0][0]*m[0][0] + m[0][1]*m[0][1] + m[0][2]*m[0][2]);
  rowlen[1] = sqrt(m[1][0]*m[1][0] + m[1][1]*m[1][1] + m[1][2]*m[1][2]);
  rowlen[2] = sqrt(m[2][0]*m[2][0] + m[2][1]*m[2][1] + m[2][2]*m[2][2]);

  for(j=0;j<3;j++) {
    m[0][j] = m[0][j]/rowlen[0];
    m[1][j] = m[1][j]/rowlen[1];
    m[2][j] = m[2][j]/rowlen[2];
  }
  
  collen[0] = sqrt(m[0][0]*m[0][0] + m[1][0]*m[1][0] + m[2][0]*m[2][0]);
  collen[1] = sqrt(m[0][1]*m[0][1] + m[1][1]*m[1][1] + m[2][1]*m[2][1]);
  collen[2] = sqrt(m[0][2]*m[0][2] + m[1][2]*m[1][2] + m[2][2]*m[2][2]);

  if(collen[0]-1 < EPSILON &&
     collen[1]-1 < EPSILON &&
     collen[2]-1 < EPSILON) {
    /* The rows hold the scales. Do rotation first. */

    remove_neg_scale(m,ROWS);

    for(i=0;i<3;i++) {
      for(j=0;j<3;j++) {
	rotMatrix[i*4 + j] = m[i][j];
      }
    }

    t_list = get_rotation(rotMatrix,t_list);

    trans = scale_trans(rowlen[0],rowlen[1],rowlen[2]);
    t_list = get_scale(trans->d,t_list);
    free(trans);

    return( t_list );
  }

  for(i=0;i<3;i++) {
    for(j=0;j<3;j++) {
      m[i][j] = d[i*4 + j];
    }
  }
  
  collen[0] = sqrt(m[0][0]*m[0][0] + m[1][0]*m[1][0] + m[2][0]*m[2][0]);
  collen[1] = sqrt(m[0][1]*m[0][1] + m[1][1]*m[1][1] + m[2][1]*m[2][1]);
  collen[2] = sqrt(m[0][2]*m[0][2] + m[1][2]*m[1][2] + m[2][2]*m[2][2]);
  
  for(j=0;j<3;j++) {
    m[j][0] = m[j][0]/collen[0];
    m[j][1] = m[j][1]/collen[1];
    m[j][2] = m[j][2]/collen[2];
  }
  
  rowlen[0] = sqrt(m[0][0]*m[0][0] + m[0][1]*m[0][1] + m[0][2]*m[0][2]);
  rowlen[1] = sqrt(m[1][0]*m[1][0] + m[1][1]*m[1][1] + m[1][2]*m[1][2]);
  rowlen[2] = sqrt(m[2][0]*m[2][0] + m[2][1]*m[2][1] + m[2][2]*m[2][2]);

  if(rowlen[0]-1 < EPSILON &&
     rowlen[1]-1 < EPSILON &&
     rowlen[2]-1 < EPSILON) {
    /* The columns hold the scales. Do rotation last. */

    trans = scale_trans(collen[0],collen[1],collen[2]);
    t_list = get_scale(trans->d,t_list);
    free(trans);

    remove_neg_scale(m,COLS);

    for(i=0;i<3;i++) {
      for(j=0;j<3;j++) {
	rotMatrix[i*4 + j] = m[i][j];
      }
    }
    
    t_list = get_rotation(rotMatrix,t_list);
    return( t_list );
  }

  /* They both hold the scales. Do nothing. */
  ger_error("vrml_ren_mthd: get_rot_and_scale: matricies of the form S2*R*S1 are ignored");
  print_matrix(d);
  
  return( t_list );
}

static P_Transform_type *reduce_matrix(float d[])
  /* Breaks down a matrix into translation, rotation,and scale types.
   * Returns the list of types. */
{
  P_Transform_type *t_list=NULL;

  ger_debug("vrml_ren_mthd: reduce_matrix");

  if(d[12] != 0 || d[13] != 0 || d[14] != 0 || d[15] != 1) {
    /* Projection matrix or something else. Screwed */
    ger_error("vrml_ren_mthd: reduce_matrix: matrix ignored; 4th row not 0,0,0,1");
    print_matrix(d);
    return( t_list );
  }

  if(d[3] != 0 || d[7] != 0 || d[11] != 0)
    t_list = get_translation(d,t_list);

  /* now its either a scale or rotation */
  /* if all but the diagonal is zero, there is no rotation, else there is */
  if(d[1]==0 && d[2]==0 && d[4]==0 && d[6]==0 && d[8]==0 && d[9]==0) {
    /* it's not a rotation */

    if(fabs(d[0]) != 1 || fabs(d[5]) != 1 || fabs(d[10]) != 1) {
      /* it's just a scale */
      t_list = get_scale(d,t_list);

    } else {/* its the identity matrix */
      ger_error("vrml_ren_mthd: reduce_matrix: identity matrix ignored");
      print_matrix(d);
      return( t_list );
    }

  } else {/* it has a rotation */

    /* if the dot(v,v) of all rows is 1 then it's just a rotation */
    if(d[0]*d[0] + d[1]*d[1] + d[2]*d[2] - 1 < EPSILON &&
       d[4]*d[4] + d[5]*d[5] + d[6]*d[6] - 1 < EPSILON &&
       d[8]*d[8] + d[9]*d[9] + d[10]*d[10] - 1 < EPSILON) {

      /* it's just a rotation */
      t_list = get_rotation(d,t_list);

    } else {/* its a rotation and a scale */
      t_list = get_rot_and_scale(d,t_list);
    }
  }
  
  return( t_list );
}

static void output_trans(FILE *outfile,P_Transform *trans)
{
  /* This routine outputs the matrix as a list of transforms
   * to comply with VRML. */

  char head[100];
  P_Transform_type *tmp,*prev=NULL,*t_list;
  int print,numTrans=0,numRot=0,numScale=0,numMatrixTypes=0;

  ger_debug("vrml_ren_mthd: output_trans");

  tmp = trans->type_front;
  
  while(tmp!=NULL) {
    print=P3D_FALSE;

    switch(tmp->type) {
    case P3D_TRANSLATION:
      sprintf(head,"translation %g %g %g\n",
	      tmp->generators[0],tmp->generators[1],tmp->generators[2]);
      print=P3D_TRUE;   numTrans++;
      break;
    case P3D_ROTATION:
      sprintf(head,"rotation %g %g %g  %g\n",
	      tmp->generators[0],tmp->generators[1],
	      tmp->generators[2],tmp->generators[3]*DegtoRad);
      print=P3D_TRUE;   numRot++;
      if(numTrans > 0)
	numRot = 2;
      break;
    case P3D_ASCALE:
      sprintf(head,"scale %g %g %g\n",
	      fabs(tmp->generators[0]),fabs(tmp->generators[1]),
	      fabs(tmp->generators[2]));
      print=P3D_TRUE;   numScale++;
      if(numTrans > 0 || numRot > 0) 
	numScale = 2;
      break;
    case P3D_TRANSFORMATION:
      t_list = reduce_matrix(((P_Transform *)tmp->trans)->d);
      t_list = add_type_to_list(tmp->next,t_list);
      if(prev==NULL)
	trans->type_front = t_list;
      else
	prev->next = t_list;
      break;
    default:
      break;
    }
    
    if(numTrans==2 || numRot==2 || numScale==2) {
      transforms[depth]++;
      fprintf(outfile,"%schildren[Transform{\n",tab_buf);
      set_indent(INCREASE);

      if(numTrans==2) {
	numTrans=1;  numRot=0;  numScale=0;
      }
      if(numRot==2) {
	numTrans=0;  numRot=1;  numScale=0;
      }
      if(numScale==2) {
	numTrans=0;  numRot=0;  numScale=1;
      }
    }

    if(print == P3D_TRUE) {
      fprintf(outfile,"%s%s",tab_buf,head);
    }

    if(tmp->type == P3D_TRANSFORMATION) {
      destroy_trans_type(tmp);
      tmp = t_list;
    } else {
      prev = tmp;
      tmp = tmp->next;
    }
  }    
}
