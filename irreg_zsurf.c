/****************************************************************************
 * irreg_zsurf.c
 * Author Doug Straub; mods by Silvia Olabarriaga
 * Copyright 1993, Pittburgh Supercomputing Center, Carnegie Mellon University
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

#include <stdio.h>
#include <math.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"

#define FACET_VTX_SZ 3

#define X       0
#define Y       1
#define Z       2
#define CMAP    3

#define CVTX_SZ    3
#define CVVTX_SZ   4


/*
Adds three vertices to the facet_array
*/

static void append_to_fac_arr( int *facet_array, int val0, int val1, 
                        int val2, int nfacets )
{
  *( facet_array+  FACET_VTX_SZ*nfacets) = val0;
  *( facet_array+1+FACET_VTX_SZ*nfacets) = val1;
  *( facet_array+Z+FACET_VTX_SZ*nfacets) = val2;
}

/* calculates the number of floats necessary to store vertex.
   accepts only coordinates and 1 scalar. If vertex type not
   supported, changes it to the closest supported type.
*/

static int size_per_vertex ( int *vtxtype )
{
  switch( *vtxtype ) {
  case P3D_CVTX:
      return( CVTX_SZ);
  case P3D_CCVTX:
      ger_error( "p3dgen: pg_irreg_zsurface: colors not supported, using CVTX");
      *vtxtype= P3D_CVTX;
      return( CVTX_SZ );
  case P3D_CNVTX:
      ger_error( "p3dgen: pg_irreg_zsurface: normals not supported, using CVTX");
      *vtxtype= P3D_CVTX;
      return( CVTX_SZ );
  case P3D_CCNVTX:
      ger_error( "p3dgen: pg_irreg_zsurface: normals and colors not supported, using CNVTX");
      *vtxtype= P3D_CVTX;
      return ( CVTX_SZ );
  case P3D_CVVTX:
      return ( CVVTX_SZ );
  case P3D_CVNVTX:
      ger_error( "p3dgen: pg_irreg_zsurface: normals not supported, using CVVTX");
      *vtxtype= P3D_CVVTX;
      return ( CVVTX_SZ );
  case P3D_CVVVTX:
      ger_error( "p3dgen: pg_irreg_zsurface: 2 scalars not supported, using CVVTX");
      *vtxtype= P3D_CVVTX;
      return ( CVVTX_SZ );
  default:
      ger_error( "p3dgen: pg_irreg_zsurface: vertex type unknown");
      return ( 0 );
  }
}

/* 
Main pg routine.  Creates a zsurface.
*/

int pg_irreg_zsurf    ( int vtxtype, float *zdata, float *valdata, 
                int nx, int ny,
		void (*xyfun) ( float *, float *, int *, int *),
		void (*testfun)( int *, float *, int *, int * ), 
		int fort)
{
  P_Vlist *vlist;
  float *pt_array;
  int *facet_array, *facet_len_array;
  int nfacets=0, float_per_vtx, i, j;
  int tempi, tempj;
  int a_0, a_1, a_nx, a_nx_1;
  int a_0_exclude, a_1_exclude, a_nx_exclude, a_nx_1_exclude;
  int cy = 1, fx = 1, zsurf_flag=0;

  if (pg_gob_open() == P3D_SUCCESS) {
  
    if ( ! xyfun )
       {
       ger_error("p3dgen: pg_irreg_zsurface: must have a xy function");
       return( P3D_FAILURE );
       }

    ger_debug( "p3dgen: pg_irreg_zsurface: adding irregular zsurface" );

    /* calculates the necessary size for each vertex type */
    float_per_vtx = size_per_vertex( &vtxtype );

    pt_array = (float *) 
               malloc(float_per_vtx*ny*nx*sizeof(float));
    facet_array = (int *) 
               malloc(FACET_VTX_SZ*2*(nx-1)*(ny-1)*sizeof(int));
    facet_len_array = (int *) 
               malloc(2*(nx-1)*(ny-1)*sizeof(int));


    if (fort)   /* if the data was from fortran, get z in row order*/
      fx = nx;
    else 
      cy = ny;

    for (j=0; j<ny; j++) {          /* get the pts */
      for (i=0; i<nx; i++) {
	  if ( fort ) {
	    tempi = i + 1;
	    tempj = j + 1;
	  }
	  else {
	    tempi = i;
	    tempj = j;
	  }
	   (*xyfun) (  pt_array+X+float_per_vtx*(i*ny+j), 
	              pt_array+Y+float_per_vtx*(i*ny+j),
		      &tempi, &tempj ); 
	   *(pt_array+Z+float_per_vtx*(i*ny+j)) = *(zdata+j*fx+i*cy);
	if (valdata)
	  *(pt_array+CMAP+float_per_vtx*(i*ny+j) ) = *(valdata+j*fx+i*cy);
      }
    }

/*
creates array of facet legths.  All facets will have 3 vertices 
*/

    for ( i=0; i<2*(nx-1)*(ny-1); i++ )
      *(facet_len_array+i) = FACET_VTX_SZ;

/* 
creates the array that contains the vertices in each facet. If the 
pointer to the test function is non-NULL, then it checks to see 
if any of the vertices in the facet are equal to the skipval. If so, 
it tries to create a facet with an opposite orientation.
*/
    if (testfun) {            

      int tempi, tempj;

      for (j=0; j<ny-1; j++) {
	for (i=0; i<nx-1; i++) {

	  if ( fort ) {
	    tempi = i + 1;
	    tempj = j + 1;
	  }
	  else {
	    tempi = i;
	    tempj = j;
	  }

	  a_0 = i*ny+j;
	  a_1 = (i+1)*ny+j;
	  a_nx = i*ny+(j+1);
	  a_nx_1 = (i+1)*ny+(j+1);
	  
	  (*testfun)( &a_0_exclude, pt_array+a_0*float_per_vtx+Z, 
		     &tempi, &tempj );
	  (*testfun)( &a_1_exclude, pt_array+a_1*float_per_vtx+Z, 
		     &tempi, &tempj );
	  (*testfun)( &a_nx_exclude, pt_array+a_nx*float_per_vtx+Z, 
		     &tempi, &tempj );
	  (*testfun)( &a_nx_1_exclude, pt_array+a_nx_1*float_per_vtx+Z, 
		     &tempi, &tempj);

	  if (! ( a_0_exclude || a_1_exclude || a_nx_exclude ) )
	    append_to_fac_arr( facet_array, a_0, a_1, a_nx, nfacets++ );
	  if (! ( a_1_exclude || a_nx_exclude || a_nx_1_exclude ))
	    append_to_fac_arr( facet_array, a_1, a_nx_1, a_nx, nfacets++ );
	  else if ( a_1_exclude && 
		   ( !( a_0_exclude || a_nx_exclude || a_nx_1_exclude ) ) )
	    append_to_fac_arr( facet_array, a_0, a_nx_1, a_nx, nfacets++ );
	  else if ( a_nx_exclude && 
		   ( !( a_0_exclude || a_1_exclude || a_nx_1_exclude ) ) )
	    append_to_fac_arr( facet_array, a_0, a_1, a_nx_1, nfacets++ );
	}
      }
    }
    else {
      for (j=0; j<ny-1; j++) {
	for (i=0; i<nx-1; i++) {
	  a_0 = i*ny+j;
	  a_1 = (i+1)*ny+j;
	  a_nx = i*ny+(j+1);
	  a_nx_1 = (i+1)*ny+(j+1);	  
	  append_to_fac_arr(facet_array, a_0, a_1, a_nx, nfacets++);
	  append_to_fac_arr(facet_array, a_1, a_nx_1, a_nx, nfacets++);
	}
      }
    }

    pg_open("");
    if (nfacets>0) {
      vlist = po_create_cvlist( vtxtype, nx*ny, pt_array );
      zsurf_flag = pg_mesh( vlist, facet_array, facet_len_array, nfacets );
    }
    dp_close();

/*done*/
    free(facet_array);
    free(pt_array);
    free(facet_len_array);
    return( zsurf_flag );
  }
  else {
    ger_error("p3dgen: pg_irreg_zsurface: must have a renderer open first");
    return( P3D_FAILURE );
  }
}
