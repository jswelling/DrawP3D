/****************************************************************************
 * assist_prim.c
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
capability to manage all necessary primitives.  It provides simple
geometrical primitives using the renderer's mesh facility.
*/
#include <stdio.h>
#include <math.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"
#include "sphere.h"
#include "cylinder.h"

static P_Void_ptr build_mesh( P_Renderer *ren, int vcount, float varray[][3], 
			     float narray[][3], int fcount, 
			     int *vert_per_facet, int *connect )
/* This routine produces mesh data from coordinate and connectivity lists */
{
  P_Vlist *vlist;
  P_Void_ptr result;
  static int id= 0;
  char name[P3D_NAMELENGTH];

  ger_debug("assist_prim: build_mesh");

  /* Create the vertex list */
  vlist= po_create_mvlist( P3D_CNVTX, vcount, (float *)varray, (float *)0, 
			  (float *)narray );

  /* Have the renderer create the mesh */
  snprintf((char*)name,sizeof(name),"$$p3d_assist_prim_mesh_%d",id++);
  METHOD_RDY(ren);
  result= (*(ren->def_mesh))(name, vlist, connect, vert_per_facet, fcount);

  /* Clean up */
  METHOD_RDY(vlist);
  (*(vlist->destroy_self))();

  return( result );
}

static P_Void_ptr sphere_prim( void )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_prim: sphere_prim");

  if (!SPHEREDATA(self)) /* first call */
    SPHEREDATA(self)= build_mesh( RENDERER(self), sphere_vertices, 
				 sphere_coords, sphere_coords, sphere_facets, 
				 sphere_v_counts, sphere_connect );

  METHOD_OUT
  return(SPHEREDATA(self));
}

static P_Void_ptr cylinder_prim( void )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_prim: cylinder_prim");

  if (!CYLDATA(self)) /* first call */
    CYLDATA(self)= build_mesh( RENDERER(self), cylinder_vertices, 
			      cylinder_coords, cylinder_normals, 
			      cylinder_facets, cylinder_v_counts,
			      cylinder_connect );

  METHOD_OUT
  return(CYLDATA(self));
}

static P_Void_ptr torus_prim( double major, double minor )
{

#define major_divisions 8
#define minor_divisions 8
#define pi 3.14159265
#define torus_vertices major_divisions*minor_divisions
#define torus_facets major_divisions*minor_divisions

  P_Assist *self= (P_Assist *)po_this;
  P_Void_ptr result;
  float coords[torus_vertices][3];
  float norms[torus_vertices][3];
  int connect[4*torus_facets], *connect_copy;
  int vertex_counts[torus_facets];
  float theta= 0.0, dt= 2.0*pi/minor_divisions;
  float phi=0.0, dp= 2.0*pi/major_divisions;
  int iloop, jloop, ip, jp, vcount;
  METHOD_IN

  ger_debug("assist_prim: torus_prim");

  /* Fill the vertex_counts array */
  for (iloop=0; iloop<torus_facets; iloop++) 
    vertex_counts[iloop]= 4;

  /* Calculate vertex positions */
  vcount= 0;
  for (iloop=0; iloop<major_divisions; iloop++) {
    for (jloop=0; jloop<minor_divisions; jloop++) {
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

  /* Generate the connectivity array */
  connect_copy= connect;
  for (iloop=0; iloop<major_divisions; iloop++)
    for (jloop=0; jloop<minor_divisions; jloop++) {
      ip= (iloop+1) % major_divisions;
      jp= (jloop+1) % minor_divisions;
      *connect_copy++= jp + iloop*minor_divisions;
      *connect_copy++= jloop + iloop*minor_divisions;
      *connect_copy++= jloop + ip*minor_divisions;
      *connect_copy++= jp + ip*minor_divisions;
    }

  result= build_mesh( RENDERER(self), torus_vertices, coords,
		     norms, torus_facets, vertex_counts, connect );

  METHOD_OUT
  return(result);

#undef major_divisions
#undef minor_divisions
#undef pi
#undef torus_vertices
#undef torus_facets
}

static void ren_mesh( P_Void_ptr rendata, P_Transform *trans,
		     P_Attrib_List *attr )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_prim: ren_mesh");
  METHOD_RDY(RENDERER(self));
  (*(RENDERER(self)->ren_mesh))( rendata, trans, attr );

  METHOD_OUT
}

static void destroy_mesh( P_Void_ptr rendata )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_prim: destroy_mesh");
  METHOD_RDY(RENDERER(self));
  (*(RENDERER(self)->destroy_mesh))( rendata );

  METHOD_OUT
}

static void do_nothing( P_Void_ptr rendata )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_prim: do_nothing");
  /* No action needed; memory will get cleaned up when the assist object
   * is destroyed.
   */

  METHOD_OUT
}

void ast_prim_reset( int hard )
/* This routine resets the primitive part of the assist module, in the 
 * event its symbol environment is reset.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_prim: ast_prim_reset");
  /* Nothing to reset */

  METHOD_OUT
}

void ast_prim_destroy( void )
/* This destroys the primitive part of an assist object */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_prim: ast_prim_destroy");

  METHOD_RDY( RENDERER(self) );
  if (SPHEREDATA(self))
    (*(RENDERER(self)->destroy_mesh))(SPHEREDATA(self));
  if (CYLDATA(self))
    (*(RENDERER(self)->destroy_mesh))(CYLDATA(self));

  METHOD_OUT
}

void ast_prim_init( P_Assist *self )
/* This initializes the primitive part of an assist object */
{

  ger_debug("assist_prim: ast_prim_init");

  /* Initialize sphere and cylinder mesh data slots.  We wait until the
   * first call to generate the standard meshes, to avoid the possiblity
   * that the renderer is not yet ready to define primitives.
   */
  SPHEREDATA(self)= (P_Void_ptr)0;
  CYLDATA(self)= (P_Void_ptr)0;

  /* Fill out the methods */
  self->def_sphere= sphere_prim;
  self->ren_sphere= ren_mesh;
  self->destroy_sphere= do_nothing;
  self->def_cylinder= cylinder_prim;
  self->ren_cylinder= ren_mesh;
  self->destroy_cylinder= do_nothing;
  self->def_torus= torus_prim;
  self->ren_torus= ren_mesh;
  self->destroy_torus= destroy_mesh;

}

