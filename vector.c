/****************************************************************************
 * vector.c
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
This module provides methods for dealing with vectors.
Most of the functions in this module contain at least one src
and a dest.  Usually if the dest is NULL (==0) the function
will create a vector, and return a pointer to the 
created vector.  These created vectors should be destroyed
by vec_destroy when the vector is no longer needed.  If dest was 
not null, the function will return a pointer to dest.
*/

#include <stdio.h>
#include <math.h>

#include "p3dgen.h"
#include "pgen_objects.h"


/*
Creates a vector
*/
P_Vector *vec_create( VOIDLIST )
{
  return( ( P_Vector *) malloc( sizeof( P_Vector ) ) );
}



/*
Destroys a vector
*/
void vec_destroy( P_Vector *vec )
{
  free( vec );
}



/*
Copies returns a copy of src. If dest is NULL, dest
is created, src copied into it, and dest is returned.
*/
P_Vector *vec_copy( P_Vector *dest, P_Vector *src )
{
  if (!dest) 
    dest = ( P_Vector * ) malloc( sizeof( P_Vector ) );
  
  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;

  return( dest );
}



/*
Returns a malloc'ed pointer a P_Vector which contains
the info in src
*/
P_Vector *vec_convert_pt_to_vec( P_Point *src )
{
  P_Vector *dest;

  dest = ( P_Vector * ) malloc( sizeof( P_Vector ) );

  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;

  return( dest );
}



/*
Returns a malloc'ed pointer a P_Point which contains
the info in src
*/
P_Point *vec_convert_vec_to_pt( P_Vector *src )
{
  P_Point *dest;

  dest = ( P_Point * ) malloc( sizeof( P_Point ) );

  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;

  return( dest );
}



/*
Adds addend1 and addend2. If dest is null, creates a new vector
and returns the result in the created vector. If dest is not null,
It returns the result in dest.
*/
P_Vector *vec_add( P_Vector *dest, P_Vector *addend1, P_Vector *addend2 )
{
  
  if (!dest)
    dest = ( P_Vector * ) malloc( sizeof( P_Vector ) );

  dest->x = addend1->x + addend2->x;
  dest->y = addend1->y + addend2->y;
  dest->z = addend1->z + addend2->z;

  return( dest );
}




/*
Subtracts sub2 from sub1 (sub1-sub2). If dest is null, creates a new vector
and returns the result in the created vector. If dest is not null,
It returns the result in dest.
*/
P_Vector *vec_sub( P_Vector *dest, P_Vector *sub1, P_Vector *sub2 )
{

  if (!dest) 
    dest = ( P_Vector * ) malloc( sizeof( P_Vector ) );

  dest->x = sub1->x - sub2->x;
  dest->y = sub1->y - sub2->y;
  dest->z = sub1->z - sub2->z;

  return( dest );
}




/*
Returns the float length of a vector
*/
float vec_length( P_Vector *src)
{
  float g;
  double h;

  h = (double) ((src->x)*(src->x) + (src->y)*(src->y) + (src->z)*(src->z));
  g= (float) sqrt( h );     
  return(g); 
}




/*
Returns the unit vector of src.  If dest is NULL, it creates
a vector which is returned
*/
P_Vector *vec_make_unit(P_Vector *dest, P_Vector *src )
{
  float len;
  
  if (!dest)
    dest = ( P_Vector * ) malloc( sizeof( P_Vector ) );
  len = vec_length( src );
  dest->x = (src->x) /len;
  dest->y = (src->y) /len;
  dest->z = (src->z) /len;
  return( dest );
}



/*
Multiplies a vector by a scalar. If dest is NULL, creates a vector
*/
P_Vector *vec_scale(P_Vector *dest, P_Vector *src, float scale)
{
  if (!dest) 
    dest = ( P_Vector * ) malloc( sizeof( P_Vector ) );

  dest->x = src->x * scale;
  dest->y = src->y * scale;
  dest->z = src->z * scale;

  return( dest );
}



/*
Returns the dot product of two vectors
*/
float vec_dot_product(P_Vector *v1, P_Vector *v2)
{
  return(v1->x*v2->x + v1->y*v2->y + v1->z*v2->z);
}



/* This routine extracts the vector that's normal to 'view'
 * with respect to 'up'. */
P_Vector *normal_component_wrt(P_Vector *up, P_Vector *view)
{
  P_Vector *tmp,*normview;
  double lengthsqr,dot;

  ger_debug("vector: normal_component_wrt");

  lengthsqr = (view->x*view->x) + (view->y*view->y) + (view->z*view->z);

  if (lengthsqr  > 0.0) {
    normview = vec_make_unit(NULL,view);
    dot = vec_dot_product(up,normview);
    normview = vec_scale(normview,normview,dot);
    tmp = vec_sub(NULL,up,normview);
    if(tmp->x==0 && tmp->y==0 && tmp->z==0) /* they are parallel*/
      return( vec_copy(NULL,up) );
    free(normview);
    return tmp;
  }
  return( NULL );
}


