/****************************************************************************
 * rand_zsurf.c
 * Authors Doug Straub and Joel Welling
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
  This module creates a random zsurface using a dirichlet tesselation
*/

#include <stdio.h>
#ifndef stardent
#include <stdlib.h>
#endif
#include "p3dgen.h"
#include "pgen_objects.h" /* because we must access vertex list methods */
#include "dirichlet.h"
#include "ge_error.h"

/* Notes-
*/

static P_Vlist *current_vlist;

static float *coord_access( float *dummy, int i, P_Void_ptr *usr_hook )
/* This routine is called by the Dirichlet tesselation package to get
 * coordinate data.  Note that the coordinate data is recopied, so we
 * can use static storage for it.
 */
{
  static float coords[2];

  METHOD_RDY( current_vlist );
  coords[0]= (*(current_vlist->x))(i);
  coords[1]= (*(current_vlist->y))(i);
  *usr_hook= (P_Void_ptr)i; /* unethically but functionally stick an
			       integer in the pointer slot */

  return(coords);
}

static void mark_outside_vtxs( dch_Tess *tess )
/* This routine marks all tesselation vertices with at least one 'bogus'
 * point as deleted.  This amounts to marking all the Delaunay simplices
 * outside the convex hull, so they can be skipped as we accumulate
 * vertices.
 */
{
  dch_Pt_list *plist;
  dch_Vtx_list *vlist;

  ger_debug("rand_zsurf: mark_outside_vtxs:");

  plist= tess->bogus_pts; /* the list of outside forming points */
  while (plist) {
    vlist= plist->pt->verts; /* the vertices this point helps form */
    while (vlist) {
      vlist->vtx->deleted= 1;
      vlist= vlist->next;
    }
    plist= plist->next;
  }
}

static int get_facets( int *facet_array, dch_Tess *tess, P_Vlist *vlist,
		       void (*testfun)(int *flag, float *x, float *y, 
				       float *z, int *index) )
/* This routine extracts the facet information from the tesselation */
{
  dch_Vtx_list *facets;
  dch_Pt_list *corners;
  int real_count= 0;
  int test_flag, index;
  float x, y, z;
  
  ger_debug("rand_zsurf: get_facets:");
  
  facets= tess->vtx_list;

  if (testfun) while (facets) { 
    if ( !facets->vtx->degenerate && !facets->vtx->deleted ) {

      /* call the user test function for all three corners */
      test_flag= 0;
      corners= facets->vtx->forming_pts;
      while (corners) {
	index= (int)(corners->pt->user_hook); /* recover index */
	x= corners->pt->coords[0];
	y= corners->pt->coords[1];
	METHOD_RDY( vlist );
	z= (*(vlist->z))(index);
	(*testfun)(&test_flag, &x, &y, &z, &index);
	if (test_flag) break;
	corners= corners->next;
      }

      /* we extract the facet if none of the tests said not to. */
      if (!test_flag) {
	real_count++;
	corners= facets->vtx->forming_pts;
	while (corners) { /* Guaranteed to be three corners per facet */
	  *facet_array++= (int)(corners->pt->user_hook); /* recover index */
	  corners= corners->next;
	}
      }

    }
    facets= facets->next;
  }

  /* null test function case- don't exclude any facets */
  else while (facets) {
    if ( !facets->vtx->degenerate && !facets->vtx->deleted ) {
      real_count++;
      corners= facets->vtx->forming_pts;
      while (corners) { /* Guaranteed to be three corners per facet */
	*facet_array++= (int)(corners->pt->user_hook); /* recover index */
	corners= corners->next;
      }
    }
    facets= facets->next;
  }

  return( real_count );
}

static void check_facet_orientation( P_Vlist *vlist, int *triple )
/* This routine swaps indices as necessary to make sure the normal vector
 * is up.  We do this by checking the z component of the cross product
 * of the edge vectors of the triangle.
 */
{
  float dx1, dy1, dx2, dy2, z_component;
  int temp;

  dx1= (*(vlist->x))(triple[1]) - (*(vlist->x))(triple[0]);
  dy1= (*(vlist->y))(triple[1]) - (*(vlist->y))(triple[0]);

  dx2= (*(vlist->x))(triple[2]) - (*(vlist->x))(triple[1]);
  dy2= (*(vlist->y))(triple[2]) - (*(vlist->y))(triple[1]);

  z_component= dx1*dy2 - dy1*dx2;

  if (z_component<0) {
    temp= triple[1];
    triple[1]= triple[2];
    triple[2]= temp;
  }
}

/*
  Creates a random zsurface from the points in data.
*/
int pg_rand_zsurf( P_Vlist *vlist, 
		  void (*testfun)(int *flag, float *x, float *y, float *z, 
				  int *index) )
{
  int facet_cnt= 0, vtx_cnt= 0, i;
  int *facet_array, *facet_len_array;
  dch_Tess *tess;
  dch_Vtx_list *facet_list;
  dch_Pt_list *facet_pt_list;
  int r_zsurf_flag= 0;

  ger_debug("p3dgen: pg_rand_zsurf: adding random zsurface of %d vertices",
            vlist->length);

  /* Quit if no gob is open */
  if (!pg_gob_open()) {
    ger_error("pg_rand_zsurf: No gob is currently open; call ignored.");
    return(P3D_FAILURE);
  }

  /* Create the tesselation */
  current_vlist= vlist;
  tess= dch_create_dirichlet_tess( (float *)0, vlist->length, 2,
					 coord_access );

  /* Mark vertices outside the convex hull deleted, and count the 
   * facets (one for each non-degenerate Delaunay simplex) 
   */
  mark_outside_vtxs( tess );
  facet_list= tess->vtx_list;
  while (facet_list) {
    if ( !facet_list->vtx->degenerate && !facet_list->vtx->deleted ) 
      facet_cnt++;
    facet_list= facet_list->next;
  }
  
  /* Each facet is a triangle having 3 vertices */
  vtx_cnt= 3*facet_cnt;

  /* Get memory */
  if ( !(facet_array = (int *) malloc( vtx_cnt*sizeof( int ) ) ) )
    ger_fatal("rand_zsurf: could not malloc %d ints for facet array!",vtx_cnt);
  if ( !(facet_len_array = (int *) malloc( facet_cnt*sizeof( int ) )) )
    ger_fatal("rand_zsurf: could not malloc %d ints for facet length array!",
	      facet_cnt);

  /* Each facet has three vertices */
  for (i=0; i<facet_cnt; i++) facet_len_array[i]= 3;

  /* Get the vertex index data.  The test function may cause some
   * facets to be omitted, but since they are all the same length
   * and there can be no more than memory has been allocated for
   * that is no problem.
   */
  facet_cnt= get_facets( facet_array, tess, vlist, testfun );

  /* Clean up the tesselation */
  dch_destroy_tesselation( tess );

  /* Make sure all the facets point upward */
  for (i=0; i<facet_cnt; i++)
    check_facet_orientation( vlist, facet_array+3*i );

  /* Create the primitive */
  pg_open("");
  if (facet_cnt>0)
    r_zsurf_flag = pg_mesh( vlist, facet_array, facet_len_array, facet_cnt );
  pg_close();
  
  /* Clean up local storage */
  free( facet_array );
  free( facet_len_array );

  return( r_zsurf_flag );
}

