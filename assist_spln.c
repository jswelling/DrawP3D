/****************************************************************************
 * assist_spln.c
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides support for renderers which do not have the
capability to manage all necessary primitives.  It provides spline
patches using the renderer's mesh facility.

Bezier patch implementation notes:
     Colors and normals for the triangles making up the mesh for the patch
     are calculated by Bezier interpolation of the colors and normals of
     the control points (if given).  Note that this means that if one
     control point has a color and/or normal, they all must.  (In practice
     it's the first control point that gets checked).  Note that the
     actual (analytical) normals to the spline patch are *not* used;
     the computational cost seemed too great for the benefit.
*/
#include <math.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"

#define bezier_facets 18
#define bezier_vertices 16
#define bezier_knots 16

/* Unrefined Bezier connectivity data */
static int bezier_connect[bezier_facets][3]= {
  {0,1,4}, {1,5,4}, {1,2,5}, {2,6,5}, {2,7,6}, {2,3,7},
  {4,5,8}, {5,9,8}, {5,6,9}, {6,10,9}, {6,7,10}, {7,11,10},
  {8,13,12}, {8,9,13}, {9,10,13}, {10,14,13}, {10,11,14}, {11,15,14} };

/* Macro to calculate cubic Bezier interpolation */
#define mcr_bezier( s, v1, v2, v3, v4 ) \
  (((-(v1) + 3*(v2) - 3*(v3) + (v4))*(s) + (3*(v1) - 6*(v2) + 3*(v3)))*(s) \
    +(-3*(v1) + 3*(v2)))*(s) + (v1)

static P_Void_ptr build_mesh( P_Renderer *ren, int type, int vcount, 
			     float *varray, int fcount, int *vert_per_facet, 
			     int *connect )
/* This routine produces mesh data from coordinate and connectivity lists */
{
  P_Vlist *vlist;
  P_Void_ptr result;
  static int id= 0;
  char name[P3D_NAMELENGTH];

  ger_debug("assist_spln: build_mesh");

  /* Create the vertex list */
  vlist= po_create_cvlist( type, vcount, (float *)varray );

  /* Have the renderer create the mesh */
  sprintf(name,"$$p3d_assist_spln_mesh_%d",id++);
  METHOD_RDY(ren);
  result= (*(ren->def_mesh))(name, vlist, connect, vert_per_facet, fcount);

  /* Clean up */
  METHOD_RDY(vlist);
  (*(vlist->destroy_self))();

  return( result );
}

static float bezier1( float s, float v1, float v2, float v3, float v4 )
{
  return((((-(v1)+3*(v2)-3*(v3)+(v4))*(s)+(3*(v1)-6*(v2)+3*(v3)))*(s)
    +(-3*(v1) + 3*(v2)))*(s) + (v1));
}

static float bezier2(float s, float t, float v1, float v2, float v3, float v4,
		     float v5, float v6, float v7, float v8, float v9, 
		     float v10, float v11, float v12, float v13, float v14, 
		     float v15, float v16)
/* This routine computes a bicubic Bezier interpolation.  The main 
 * purpose of this is to make sure the intermediates only get interpolated 
 * once.
 */
{
  float x1, x2, x3, x4, result;

  x1= mcr_bezier( s, v1, v2, v3, v4 );
  x2= mcr_bezier( s, v5, v6, v7, v8 );
  x3= mcr_bezier( s, v9, v10, v11, v12 );
  x4= mcr_bezier( s, v13, v14, v15, v16 );

  /* If we use the macro for all 5 interpolations, some compilers
   * (DECs for example) will optimize themselves into a state of
   * confusion.
   */
  result= bezier1( t, x1, x2, x3, x4 );

  return( result );
}

static void calc_bezier(float s, float t, float *knots, float *target, 
			int stride)
/* This routine actually does the Bezier interpolation */
{
  int i;

  for (i=0; i<stride; i++) {
    *target++= bezier2( s, t,
		     *knots, *(knots+stride),
		     *(knots+2*stride), *(knots+3*stride), 
		     *(knots+4*stride), *(knots+5*stride), 
		     *(knots+6*stride), *(knots+7*stride), 
		     *(knots+8*stride), *(knots+9*stride), 
		     *(knots+10*stride), *(knots+11*stride), 
		     *(knots+12*stride), *(knots+13*stride), 
		     *(knots+14*stride), *(knots+15*stride) );
    knots++;
  }
}

static int get_cell_size( int type )
{
  int size;

  ger_debug("assist_spln: cell_size");

  switch (type) {
  case P3D_CVTX:    size= 3; break;
  case P3D_CCVTX:   size= 7; break;
  case P3D_CNVTX:   size= 6; break;
  case P3D_CCNVTX:  size= 10; break;
  case P3D_CVVTX:   size= 4; break;
  case P3D_CVNVTX:  size= 7; break;
  case P3D_CVVVTX:  size= 5; break;
  default: 
    ger_error("assist_spln: get_cell_size: unknown vertex type %d!",type);
    size= 3;
  }
  return(size);
}

static float *alloc_space( int num_cells, int vtx_type )
{
  int cell_size;
  float *result;

  ger_debug("assist_spln: alloc_space");

  cell_size= get_cell_size( vtx_type );

  if ( !(result= (float *)malloc(num_cells*cell_size*sizeof(float))) )
    ger_fatal("assist_spln: alloc_space: unable to allocate %d floats!",
	      num_cells*cell_size);

  return(result);
}

static float *extract_knots( P_Vlist *knotlist )
/* This routine extracts knots from a knot list.  It has to select
 * which vertex type to create, and allocate space appropriately.
 */
{
  int i;
  int type;
  float *knots, *runner;

  ger_debug("assist_spnl: extract_knots");

  type= knotlist->type;

  knots= runner= alloc_space( knotlist->length, type );

  for (i=0; i<knotlist->length; i++) {
    METHOD_RDY(knotlist);
    *runner++= (*(knotlist->x))(i);
    *runner++= (*(knotlist->y))(i);
    *runner++= (*(knotlist->z))(i);

    if ((knotlist->type == P3D_CCVTX) || (knotlist->type == P3D_CCNVTX)) {
      *runner++= (*(knotlist->r))(i);
      *runner++= (*(knotlist->g))(i);
      *runner++= (*(knotlist->b))(i);
      *runner++= (*(knotlist->a))(i);
    }
    else if ((knotlist->type == P3D_CVVTX) || (knotlist->type == P3D_CVNVTX)
	     || (knotlist->type == P3D_CVVVTX)) {
      *runner++= (*(knotlist->v))(i);
    }
    
    if ((knotlist->type == P3D_CNVTX) || (knotlist->type == P3D_CVNVTX)
	|| (knotlist->type == P3D_CCNVTX)) {
      *runner++= (*(knotlist->nx))(i);
      *runner++= (*(knotlist->ny))(i);
      *runner++= (*(knotlist->nz))(i);
    }

    if (knotlist->type == P3D_CVVVTX)
      *runner++= (*(knotlist->v2))(i);
  }

  return(knots);
}

static P_Void_ptr def_bezier( P_Vlist *knotlist )
/* This routine uses the renderer's mesh function to emulate a bicubic
 * Bezier patch.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Void_ptr result;
  float *knots;
  float *coords;
  int i, j;
  float s, t, sstep, tstep;
  int vertex_counts[bezier_facets];
  int cellsize;
  METHOD_IN

  ger_debug("assist_spln: def_bezier");

  if (knotlist->length != bezier_knots) {
    ger_error(
       "assist_spln: def_bezier: need %d knots for a Bezier patch; got %d.",
	      bezier_knots, knotlist->length);
    return( (P_Void_ptr)0 );
  }

  /* Extract the knots from the knotlist, getting the appropriate type
   * for the mesh we will create.
   */
  knots= extract_knots( knotlist );

  /* Allocate coordinate list based on type */
  coords= alloc_space( bezier_vertices, knotlist->type );

  /* Fill the vertex_counts array */
  for (i=0; i<bezier_facets; i++) vertex_counts[i]= 3;
  
  /* Fill the coordinates array (calculating vertex positions) */
  cellsize= get_cell_size( knotlist->type );
  sstep= 1.0/3.0;
  tstep= 1.0/3.0;
  t= 0.0;
  for (j=0; j<4; j++) {
    s= 0.0;
    for (i=0; i<4; i++) {
      calc_bezier( s, t, knots, &coords[cellsize*(i+4*j)], cellsize );
      s += sstep;
    }
    t += tstep;
  }
  
  /* Generate the mesh from the torus data and draw it */
  result= build_mesh( RENDERER(self), knotlist->type, bezier_vertices, coords,
	     bezier_facets, vertex_counts, (int *)bezier_connect );

  /* Clean up */
  free( (P_Void_ptr)knots );
  free( (P_Void_ptr)coords );

  METHOD_OUT
  return(result);
}

static void ren_mesh( P_Void_ptr rendata, P_Transform *trans,
		     P_Attrib_List *attr )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_spln: ren_mesh");
  METHOD_RDY(RENDERER(self));
  (*(RENDERER(self)->ren_mesh))( rendata, trans, attr );

  METHOD_OUT
}

static void destroy_mesh( P_Void_ptr rendata )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_spln: destroy_mesh");
  METHOD_RDY(RENDERER(self));
  (*(RENDERER(self)->destroy_mesh))( rendata );

  METHOD_OUT
}

void ast_spln_reset( int hard )
/* This routine resets the spline handling part of the assist module, in the 
 * event its symbol environment is reset.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_spln: ast_spln_reset");
  /* Nothing to reset */

  METHOD_OUT
}

void ast_spln_destroy( void )
/* This destroys the spline handling part of an assist object */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_spln: ast_spln_destroy");
  /* Nothing to destroy */

  METHOD_OUT
}

void ast_spln_init( P_Assist *self )
/* This initializes the spline handling part of an assist object */
{

  ger_debug("assist_spln: ast_spln_init");

  /* Fill out the methods */
  self->def_bezier= def_bezier;
  self->ren_bezier= ren_mesh;
  self->destroy_bezier= destroy_mesh;

}

