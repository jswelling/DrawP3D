/****************************************************************************
 * zsurface.c
 * Author Doug Straub
 * Copyright 1991, Pittburgh Supercomputing Center, Carnegie Mellon University
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
This module creates a "zsurface", which is in effect a rectangular mesh
with a height and color attribute associated with each point 
*/

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
#define CNVTX_SZ   6
#define CVNVTX_SZ  7


#define GETZ( pt_array, i, j, nx, float_vtx_sz ) \
	   *( (pt_array) + ((nx)*(j) + (i))*(float_vtx_sz) + Z ) 


/*
Adds three vertices to the facet_array
*/

void append_to_fac_arr( int *facet_array, int val0, int val1, 
                        int val2, int nfacets )
{
  *( facet_array+X+FACET_VTX_SZ*nfacets) = val0;
  *( facet_array+Y+FACET_VTX_SZ*nfacets) = val1;
  *( facet_array+Z+FACET_VTX_SZ*nfacets) = val2;
}


static float deriv_forwards( float v1, float v2, float v3, float step )
/* This routine takes a first derivative to second order accuracy by
 * forward differencing.
 */
{
  return( (-0.5*v3 +2.0*v2 -1.5*v1)/step );
}


static float deriv_centered( float v1, float v2, float step )
/* This routine takes a first derivative to second order accuracy by
 * centered differencing.
 */
{
  return( (v2-v1)/(2.0*step) );
}



/* 
Main pg routine.  Creates a zsurface.
*/

int pg_zsurface(int vtxtype, float *zdata, float *valdata, 
                 int nx, int ny, P_Point *corner1, 
                 P_Point *corner2, 
		void (*testfun)( int *, float *, int *, int * ), int fort)
{
  P_Vlist *vlist;
  float *pt_array, *dest, offset;
  float x_pos, y_pos;
  float deltax, deltay, gradx, grady, norm_len;
  int *facet_array, *facet_len_array;
  int nfacets=0, float_per_vtx, i, j;
  int a_0, a_1, a_ny, a_ny_1;
  int a_0_exclude, a_1_exclude, a_ny_exclude, a_ny_1_exclude;
  int cy = 1, fx = 1, zsurf_flag;

  if (pg_gob_open() == P3D_SUCCESS) {
   
    if ( ( corner1->x == corner2->x ) || ( corner1->y == corner2->y ) ) { 
      ger_error("p3dgen: pg_zsurface: corners must not be colinear in the x or y direction");
      return( P3D_FAILURE );
    }
    
  
    ger_debug( "p3dgen: pg_zsurface: adding zsurface, corner1= (%f %f %f)",
	    corner1->x, corner1->y, corner1->z);


    /* calculates the necessary size for each vertex type */

    switch( vtxtype ) {
    case P3D_CVTX:
      float_per_vtx = CVTX_SZ;
      break;
    case P3D_CCVTX:
      ger_error(
	 "p3dgen: pg_zsurface: CCVTX vertex type not supported, using CVTX");
      vtxtype= P3D_CVTX;
      float_per_vtx = CVTX_SZ;
      break;
    case P3D_CNVTX:
      float_per_vtx = CNVTX_SZ;
      break;
    case P3D_CCNVTX:
      ger_error(
	 "p3dgen: pg_zsurface: CCNVTX vertex type not supported, using CNVTX");
      vtxtype= P3D_CNVTX;
      float_per_vtx = CNVTX_SZ;
      break;      
    case P3D_CVVTX:
      float_per_vtx = CVVTX_SZ;
      break;
    case P3D_CVNVTX:
      float_per_vtx = CVNVTX_SZ;
      break;
    case P3D_CVVVTX:
      ger_error(
	 "p3dgen: pg_zsurface: CVVVTX vertex type not supported, using CVVTX");
      vtxtype= P3D_CVVTX;
      float_per_vtx = CVVTX_SZ;
    default:
      break;
    }

    pt_array = (float *) 
               malloc(float_per_vtx*ny*nx*sizeof(float));
    facet_array = (int *) 
               malloc(FACET_VTX_SZ*2*(nx-1)*(ny-1)*sizeof(int));
    facet_len_array = (int *) 
               malloc(2*(nx-1)*(ny-1)*sizeof(int));

    for ( i=0; i<2*(nx-1)*(ny-1); i++ )     /*create array of facet lengths*/
      *(facet_len_array+i) = FACET_VTX_SZ; /* all facets will have 3 vertices*/

    x_pos = corner1->x;    /*starting pos*/
    y_pos = corner1->y;    /*ending pos*/
    deltax = ( corner2->x - corner1->x ) / ( nx - 1 );
    deltay = ( corner2->y - corner1->y ) / ( ny - 1 );

    if (fort)   /* if the data was from fortan, get z in row order*/
      fx = nx;
    else 
      cy = ny;

    for (i=0; i<nx; i++) {          /* get the pts */
      for (j=0; j<ny; j++) {
	*(pt_array+X+float_per_vtx*(i*ny+j) ) = x_pos + i*deltax;
	*(pt_array+Y+float_per_vtx*(i*ny+j) ) = y_pos + j*deltay; 
	*(pt_array+Z+float_per_vtx*(i*ny+j) ) = *(zdata+i*cy+j*fx);
	if (valdata)
	  *(pt_array+CMAP+float_per_vtx*(i*ny+j) ) = *(valdata+i*cy+j*fx);
      }
    }

/*
If we need to calculate normals, this portion does it.  The zsurface is
made up of right triangles, and this portion calculates a normal to 
each right triangle. 
*/


    if ( ( float_per_vtx == CVNVTX_SZ ) || ( float_per_vtx == CNVTX_SZ ) )  {
      for (j=0; j<ny; j++) 
	for (i=0; i<nx; i++) {

	  if (i==0) gradx= 
	    deriv_forwards( GETZ( pt_array, 0, j, nx, float_per_vtx ), 
			   GETZ( pt_array, 1, j, nx, float_per_vtx ), 
			   GETZ( pt_array, 2, j, nx, float_per_vtx ), deltax );
	  else if (i==nx-1) gradx=
	    deriv_forwards( GETZ( pt_array, nx-1, j, nx, float_per_vtx ), 
			   GETZ( pt_array, nx-2, j, nx, float_per_vtx ), 
			   GETZ( pt_array, nx-3, j, nx, float_per_vtx ),
			   -deltax );
	  else gradx=
	    deriv_centered( GETZ( pt_array, i-1, j, nx, float_per_vtx ), 
			   GETZ( pt_array, i+1, j, nx, float_per_vtx ), 
			   deltax );
	  if (j==0) grady= 
	    deriv_forwards( GETZ( pt_array, i, 0, nx, float_per_vtx ), 
			   GETZ( pt_array, i, 1, nx, float_per_vtx ), 
			   GETZ( pt_array, i, 2, nx, float_per_vtx ),
			   deltay );
	  else if (j==ny-1) grady=
	    deriv_forwards( GETZ( pt_array, i, ny-1, nx, float_per_vtx ), 
			   GETZ( pt_array, i, ny-2, nx, float_per_vtx ), 
			   GETZ( pt_array, i, ny-3, nx, float_per_vtx ),
			   -deltay );
	  else grady=
	    deriv_centered( GETZ( pt_array, i, j-1, nx, float_per_vtx ), 
			   GETZ( pt_array, i, j+1, nx, float_per_vtx ),
			   deltay );

	  norm_len = ( float )
	    sqrt( ( double ) gradx*gradx + grady*grady + 1.0 );

	  *( pt_array + float_per_vtx*(i*ny + j + 1) - 3 ) = -gradx/norm_len;
	  *( pt_array + float_per_vtx*(i*ny + j + 1) - 2 ) = -grady/norm_len;
	  *( pt_array + float_per_vtx*(i*ny + j + 1) - 1 ) = 1.0/norm_len;
	}
    }
/* 
This section creates the array that contains the vertices in each
facet. If the pointer to the test function is non-NULL, then it checks to see 
if any of the vertices in the facet are equal to the skipval. If so, 
it tries to create a facet with an opposite orientation.
*/
    if (testfun) {            

      int tempi, tempj, tempi_ny, tempj_1;

      for (i=0; i<nx-1; i++) {
	for (j=0; j<ny-1; j++) {

	  if ( fort ) {
	    tempi = i + 1;
	    tempi_ny= i + 2;
	    tempj = j + 1;
	    tempj_1= j + 2;
	  }
	  else {
	    tempi = i;
	    tempi_ny= i + 1;
	    tempj = j;
	    tempj_1= j + 1;
	  }

	  a_0 = j+i*ny;
	  a_1 = j+1+i*ny;
	  a_ny = j+(i+1)*ny;
	  a_ny_1 = j+(i+1)*ny+1;
	  
	  (*testfun)( &a_0_exclude, 
		     pt_array+a_0*float_per_vtx+Z, &tempi, &tempj );
	  (*testfun)( &a_1_exclude, 
		     pt_array+a_1*float_per_vtx+Z, &tempi, &tempj_1 );
	  (*testfun)( &a_ny_exclude, 
		     pt_array+a_ny*float_per_vtx+Z, &tempi_ny, &tempj );
	  (*testfun)( &a_ny_1_exclude, 
		     pt_array+a_ny_1*float_per_vtx+Z, &tempi_ny, &tempj_1);

	  if (! ( a_0_exclude || a_1_exclude || a_ny_exclude ) )
	    append_to_fac_arr( facet_array, a_0, a_ny, a_1, nfacets++ );
	  if (! ( a_1_exclude || a_ny_exclude || a_ny_1_exclude ))
	    append_to_fac_arr( facet_array, a_1, a_ny, a_ny_1, nfacets++ );
	  else if ( a_1_exclude && 
		   ( !( a_0_exclude || a_ny_exclude || a_ny_1_exclude ) ) )
	    append_to_fac_arr( facet_array, a_0, a_ny, a_ny_1, nfacets++ );
	  else if ( a_ny_exclude && 
		   ( !( a_0_exclude || a_1_exclude || a_ny_1_exclude ) ) )
	    append_to_fac_arr( facet_array, a_0, a_ny_1, a_1, nfacets++ );
	}
      }
    }
    else {
      for (i=0; i<ny-1; i++) {
	for (j=0; j<nx-1; j++) {
	  a_0 = j+i*nx;
	  a_1 = j+1+i*nx;
	  a_ny = j+(i+1)*nx;
	  a_ny_1 = j+(i+1)*nx+1;
	  append_to_fac_arr(facet_array, a_0, a_ny, a_1, nfacets++);
	  append_to_fac_arr(facet_array, a_1, a_ny, a_ny_1, nfacets++);
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
    ger_error("p3dgen: pg_zsurface: must have a renderer open first");
    return( P3D_FAILURE );
  }
}


