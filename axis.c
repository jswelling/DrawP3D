/****************************************************************************
 * axis.c
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
This module creates an axis. The length of the tics is determined by
TIC_SCALE.  The number of decimal points saved is 8.
*/

#include <stdio.h>
#include <math.h>

#include "p3dgen.h"
#include "pgen_objects.h"

#define PTS_2 2
#define TEXT_POS_SCALE 1.0
#define LABEL_POS_SCALE 1.5
/* tic_vec is initially a unit vector, and is scaled by this constant */
#define TIC_SCALE 0.3      
#define SMALL_PRECIS 0
#define LARGE_PRECIS 8

#define DATA_ARRAY_LEN 6


#define PARALLEL_P \
(u_vec->x==v_vec->x)&&(u_vec->y==v_vec->y)&&(u_vec->z==v_vec->z)
#define PARALLEL_N \
(u_vec->x==-v_vec->x)&&(u_vec->y==-v_vec->y)&&(u_vec->z==-v_vec->z)


void copy_to_array( P_Vector *pt1, P_Vector *pt2, float *dest)
{
  *(dest+0) = pt1->x;
  *(dest+1) = pt1->y;
  *(dest+2) = pt1->z;
  *(dest+3) = pt2->x;
  *(dest+4) = pt2->y;
  *(dest+5) = pt2->z;
}


int pg_axis( P_Point *start, P_Point *end, P_Vector *v, double startval, 
            double endval, int num_tics, char *label, 
            double text_height, int precision)
{


  P_Vector *start_vec, *end_vec, *temp_vec, *u_vec, *v_vec;
  P_Vector *src, *seg_dest, *tic_dest, *seg_vec, *tic_vec, *label_vec;
  P_Vector *text_dest, *text_vec;
  P_Point *label_point, *text_point;

  float *data_array, seg_len;
  int axis_ok, i;
  char text_string[32],precision_str[16];
  double increment;

  ger_debug("pg_axis: (%f %f %f) to (%f %f %f), label is <%s>",
	    start->x, start->y, start->z, end->x, end->y, end->z, label);

  if (pg_gob_open() == P3D_SUCCESS) {
    /*all operations done with vectors*/
    start_vec = vec_convert_pt_to_vec( start ); 
    end_vec = vec_convert_pt_to_vec( end ); 
    u_vec = vec_sub( VEC_CREATE, end_vec, start_vec );
    
    /* make tic vector. tic_vec: v - u*(u.v)/||v||*/
    u_vec = vec_make_unit( u_vec, u_vec );
    v_vec = vec_make_unit( VEC_CREATE, v );

    /* check to see if they are parallel. */
    if ((PARALLEL_P)||(PARALLEL_N)) {
      ger_error("p3dgen: pg_axis: up and (end-start) cannot be parallel. Call ignored.");
      return( P3D_FAILURE );
    }
    if ( num_tics < 2 ) 
      num_tics = 2;

    data_array = (float *) malloc(DATA_ARRAY_LEN*sizeof(float));

    temp_vec = vec_scale( VEC_CREATE, u_vec, vec_dot_product( u_vec, v_vec ));
    v_vec = vec_sub( v_vec, v_vec, temp_vec );
    tic_vec = vec_make_unit( VEC_CREATE, v_vec  );
    tic_vec = vec_scale( tic_vec, tic_vec, TIC_SCALE);  /* made tic vector */

    /* make seg vector */
    if ( num_tics < 2 ) 
      num_tics = 2;
    seg_vec = vec_sub( VEC_CREATE, end_vec, start_vec );
    seg_len = vec_length(seg_vec);
    seg_vec = vec_make_unit( seg_vec, seg_vec);
    seg_vec = vec_scale( seg_vec, seg_vec, seg_len/(num_tics-1) );
    
    pg_open("");
    axis_ok = pg_textheight( text_height ); /* set height for text */

    /* make tics, segs, and tic labels except end tic and end tic label*/
    tic_dest = vec_sub( VEC_CREATE, start_vec, tic_vec );
    seg_dest = vec_add( VEC_CREATE, start_vec, seg_vec );
    text_dest = vec_copy( VEC_CREATE, tic_dest );
    text_vec = vec_make_unit( VEC_CREATE, tic_vec );
    /* vector to be added on to tic_dest to position the text. Dependent
       on text height and and the defined scale float */
    text_vec = vec_scale( text_vec, text_vec, 
                           TEXT_POS_SCALE*((float)text_height ) );
   
    text_dest= vec_sub( text_dest, text_dest, text_vec ); 
    src = vec_copy(VEC_CREATE,start_vec);
    num_tics -= 1;
    increment = (endval - startval) / num_tics;
    if ( precision < SMALL_PRECIS )      /* limit precision to range 0..8 */
      precision = SMALL_PRECIS;
    else if ( precision > LARGE_PRECIS )
      precision = LARGE_PRECIS;
    sprintf(precision_str,"%%1.%dlf",precision);  

    /* make big label */

    label_vec = vec_scale( VEC_CREATE, text_vec, LABEL_POS_SCALE );
    label_vec = vec_sub( label_vec, text_dest, label_vec );
    label_point = vec_convert_vec_to_pt( label_vec );
    axis_ok = axis_ok && pg_text( label, label_point, seg_vec, tic_vec );

    /* Make the all the segment vectors, all the tic vectors and labels
       except the end label and tic vector */
    for (i=0; i<num_tics; i++) {
      copy_to_array( src, seg_dest, data_array );    
      axis_ok =  axis_ok && 
              pg_polyline(po_create_cvlist( P3D_CVTX, PTS_2, data_array ) );
      copy_to_array( src, tic_dest, data_array );    
      axis_ok =  axis_ok && 
              pg_polyline(po_create_cvlist( P3D_CVTX, PTS_2, data_array ) );  
      sprintf(text_string, precision_str, startval);
      text_point = vec_convert_vec_to_pt( text_dest );
      axis_ok = axis_ok && pg_text( text_string, text_point, u_vec, tic_vec);

      src = vec_add( src, seg_vec, src );
      seg_dest = vec_add( seg_dest, seg_vec, seg_dest );
      tic_dest = vec_add( tic_dest, seg_vec, tic_dest );
      text_dest = vec_add( text_dest, seg_vec, text_dest );
      startval += increment;
    }
    
    /* make end tic and end tic label */
    tic_dest = vec_sub( tic_dest, end_vec, tic_vec );
    copy_to_array( end_vec, tic_dest, data_array );    
    axis_ok =  axis_ok && 
      pg_polyline(po_create_cvlist( P3D_CVTX, PTS_2, data_array ));
    text_dest = vec_sub( text_dest, tic_dest, text_vec );
    text_point = vec_convert_vec_to_pt( text_dest );
    sprintf( text_string, precision_str, endval );
    axis_ok = axis_ok && pg_text( text_string, text_point, u_vec, tic_vec ); 

    /*done*/
    pg_close();
    vec_destroy( start_vec );
    vec_destroy( end_vec );
    vec_destroy( temp_vec );
    vec_destroy( u_vec );
    vec_destroy( v_vec );
    vec_destroy( src );
    vec_destroy( seg_dest );
    vec_destroy( tic_dest );
    vec_destroy( seg_vec );
    vec_destroy( tic_vec );
    vec_destroy( text_dest );
    vec_destroy( text_vec );
    vec_destroy( label_vec );
    free( label_point );
    free( text_point );
    free( data_array );
    return( axis_ok );
  }
  else {
    ger_error("p3dgen: pg_axis: must have a renderer open first.  Call ignored.");
    return( P3D_FAILURE );
  }
}



















