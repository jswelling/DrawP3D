/****************************************************************************
 * boundbox.c
 * Author Doug Straub
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
This module describes a routine for a bounding box.  
*/

#include <stdio.h>

#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"

#define P3D_CVTX_SZ 3
#define PTS_5 5
#define PTS_2 2
#define X 0
#define Y 1
#define Z 2

void pt_exchange(float *dest, float x, float y, float z) 
/* This routine copies a point into an array in correct order*/

{
 *(dest+X)= x;
 *(dest+Y)= y;
 *(dest+Z)= z;
}

int pg_boundbox( P_Point *corner1, P_Point *corner2 )
/* This routine adds a bounding box gob to the currently open gob */

{
  P_Gob *thisgob;
  float *data_array;
  int bound_box_ok;


  ger_debug("p3dgen: pg_boundbox: adding bounding box, corner1= (%f %f %f)",
	    corner1->x, corner1->y, corner1->z);

  if ((corner1->x == corner2->x) || (corner1->y == corner2->y) || 
 (corner1->z == corner2->z)) {
    ger_error("p3dgen: pg_boundbox: pts can not be colinear or coplanar;call ignored");
    return( P3D_FAILURE );
  }

  if (pg_gob_open()) {

    data_array = (float *) malloc(PTS_5*P3D_CVTX_SZ*sizeof(float));   
    
    pg_open("");
                                                   /* top */
                                                          /* (x1, y1, z1) */
    pt_exchange(data_array, corner1->x, corner1->y, corner1->z);
                                                          /* (x2, y1, z1) */
    pt_exchange(data_array+3, corner2->x, corner1->y, corner1->z);
                                                          /* (x2, y1, z2) */
    pt_exchange(data_array+6, corner2->x, corner1->y, corner2->z);
                                                          /* (x1, y1, z2) */
    pt_exchange(data_array+9, corner1->x, corner1->y, corner2->z);
                                                          /* (x1, y1, z1) */
    pt_exchange(data_array+12, corner1->x, corner1->y, corner1->z);
    bound_box_ok= pg_polyline(po_create_cvlist( P3D_CVTX, PTS_5, data_array ));

                                                   /* bottom */
                                                          /* (x1, y2, z1) */
    pt_exchange(data_array, corner1->x, corner2->y, corner1->z);
                                                          /* (x2, y2, z1) */
    pt_exchange(data_array+3, corner2->x, corner2->y, corner1->z);
                                                          /* (x2, y2, z2) */
    pt_exchange(data_array+6, corner2->x, corner2->y, corner2->z);
                                                          /* (x1, y2, z2) */
    pt_exchange(data_array+9, corner1->x, corner2->y, corner2->z);
                                                          /* (x1, y2, z1) */
    pt_exchange(data_array+12, corner1->x, corner2->y, corner1->z);
    bound_box_ok= bound_box_ok && 
          pg_polyline( po_create_cvlist( P3D_CVTX, PTS_5, data_array ) );

                                                   /* side1 */
                                                          /* (x1, y1, z1) */
    pt_exchange(data_array, corner1->x, corner1->y, corner1->z);
                                                          /* (x1, y2, z1) */
    pt_exchange(data_array+3, corner1->x, corner2->y, corner1->z);
    bound_box_ok= bound_box_ok && 
          pg_polyline( po_create_cvlist( P3D_CVTX, PTS_2, data_array ) );

                                                   /* side2 */
                                                          /* (x1, y1, z2) */
    pt_exchange(data_array, corner1->x, corner1->y, corner2->z);
                                                          /* (x1, y2, z2) */
    pt_exchange(data_array+3, corner1->x, corner2->y, corner2->z);
    bound_box_ok= bound_box_ok && 
          pg_polyline( po_create_cvlist( P3D_CVTX, PTS_2, data_array ) );

                                                   /* side3 */
                                                          /* (x2, y1, z2) */
    pt_exchange(data_array, corner2->x, corner1->y, corner2->z);
                                                          /* (x2, y2, z2) */
    pt_exchange(data_array+3, corner2->x, corner2->y, corner2->z);
    bound_box_ok= bound_box_ok &&
          pg_polyline( po_create_cvlist( P3D_CVTX, PTS_2, data_array ) );

                                                   /* side4 */
                                                          /* (x2, y1, z1) */
    pt_exchange(data_array, corner2->x, corner1->y, corner1->z);
                                                          /* (x2, y2, z1) */
    pt_exchange(data_array+3, corner2->x, corner2->y, corner1->z);
    bound_box_ok= bound_box_ok && 
          pg_polyline( po_create_cvlist( P3D_CVTX, PTS_2, data_array ) );

    pg_close();
    free(data_array);
    return(bound_box_ok);
  }
  else {
    ger_error("boundbox: pg_boundbox: must have a renderer open first");
    return( P3D_FAILURE );
  }
}













