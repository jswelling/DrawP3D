/****************************************************************************
 * transform.c
 * Author Joel Welling
 * Copyright 1990, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides some routines for manipulating transforms.
*/

#include <stdio.h>
#include <math.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "indent.h"

/* Identity transform */
static P_Transform identity_trans_body= {{
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0
}};
P_Transform *Identity_trans= &identity_trans_body; 

void dump_trans_type(P_Transform_type *t)
/* This routine dumps a transformation_type */
{
  char *type;

  switch(t->type) {
  case P3D_TRANSFORMATION:
    type = "P3D_TRANSFORMATION";
    break;
  case P3D_TRANSLATION:
    type = "P3D_TRANSLATION";
    break;
  case P3D_ROTATION:
    type = "P3D_ROTATION";
    break;
  case P3D_ASCALE:
    type = "P3D_ASCALE";
    break;
  default:
    type = "UNKNOWN";
    break;
  }
  printf("type = %s\n",type);

  if(t->type==P3D_TRANSFORMATION) {
    dump_trans((P_Transform *)t->trans);

  } else {
    printf("generator = %g %g %g %g\n",t->generators[0],
	    t->generators[1],t->generators[2],t->generators[3]);
  }
  printf("next = %d\n",t->next);
}

void dump_trans(P_Transform *trans)
/* This routine dumps a transformation */
{
  int i;
  float *floattrans;

  ger_debug("transform: dump_trans");

  floattrans= trans->d;
  ind_push();
  for (i=0; i<16; i+=4) {
    ind_write("( %f %f %f %f )",floattrans[i],floattrans[i+1],
              floattrans[i+2],floattrans[i+3]);
    ind_eol();
  }  
  ind_pop();
}

P_Transform_type *allocate_trans_type( VOIDLIST )
/* This routine allocates a transform_type */
{
  P_Transform_type *t;

  ger_debug("transform: allocate_trans_type");

  if ( !(t= (P_Transform_type *)malloc(sizeof(P_Transform_type))) )
    ger_fatal("transform: allocate_trans_type: unable to allocate %d bytes!",
              sizeof(P_Transform_type));

  return( t );
}

P_Transform *allocate_trans( VOIDLIST )
/* This routine allocates a transform */
{
  P_Transform *thistrans;

  ger_debug("transform: allocate_trans");

  if ( !(thistrans= (P_Transform *)malloc(sizeof(P_Transform))) )
    ger_fatal("transform: allocate_trans: unable to allocate %d bytes!",
              sizeof(P_Transform));

  return( thistrans );
}

void destroy_trans_type( P_Transform_type *type )
/* This routine destroys a transform_type */
{
  ger_debug("transform: destroy_trans_type");

  if(type->type == P3D_TRANSFORMATION) 
    destroy_trans((P_Transform *)type->trans);
  free( (P_Void_ptr)type );
}

void destroy_trans( P_Transform *trans )
/* This routine destroys a transform */
{
  ger_debug("transform: destroy_trans");
  free( (P_Void_ptr)trans );
}

void copy_trans_type( P_Transform_type *target, P_Transform_type *source )
/* This routine copies ONE transform_type into existing memory */
{
  int i;

  ger_debug("transform: copy_trans_type");

  target->type = source->type;
  for(i=0;i<4;i++) {
    target->generators[i] = source->generators[i];
  }

  if(source->type==P3D_TRANSFORMATION) {
    target->trans = (P_Void_ptr)duplicate_trans((P_Transform *)source->trans);
  }
  target->next = NULL;
}

void copy_trans( P_Transform *target, P_Transform *source )
/* This routine copies a transform into existing memory */
{
  int i;
  float *f1, *f2;
  
  ger_debug("transform: copy_trans");

  f1= target->d;
  f2= source->d;
  for (i=0; i<16; i++) 
    *f1++= *f2++;
}

P_Transform_type *duplicate_trans_type( P_Transform_type *source )
/* This routine returns a unique copy of a transform_type */
{
  P_Transform_type *t;
    
  ger_debug("transform: duplicate_trans_type");

  if (source==NULL) t= NULL;
  else {
    t = allocate_trans_type();
    copy_trans_type(t,source);
  }
  return( t );
}

P_Transform *duplicate_trans( P_Transform *thistrans )
/* This routine returns a unique copy of a transform */
{
  P_Transform *dup;

  ger_debug("transform: duplicate_trans");

  dup= allocate_trans();
  copy_trans(dup, thistrans);
  return( dup );
}

P_Transform *transpose_trans( P_Transform *thistrans )
/* This routine takes the transpose of the given transform in place */
{
  register float temp;
  register int i,j;
  register float *matrix;
  
  ger_debug("transform: transpose_trans");

  matrix= thistrans->d;
  for (i=0; i<4; i++)
    for (j=0; j<i; j++)
      if ( i != j) {
	temp= *(matrix + 4*i +j);
	*(matrix + 4*i + j)= *(matrix + 4*j +i);
	*(matrix + 4*j + i)= temp;
      };
  
  return( thistrans );
}

P_Transform *premult_trans( P_Transform *multby, P_Transform *original )
/* This routine premultiplies 'multby' into 'original' in place,
 * and returns a pointer to original.
 */
{
  int i, row, column;
  float *M1, *M2;
  float newMatrix[16];

  ger_debug("transform: premult_trans");

  M1= multby->d;
  M2= original->d;
    
  for (i=0;i<16;i++) newMatrix[i]=0.0;

  for (row = 0;row<4;row++)
    for (column= 0;column<4;column++)
      for (i=0;i<4;i++)
        newMatrix[(4*row)+column] += M1[(4*row)+i]*
          M2[(4*i)+column];

  M2= original->d;
  for (i=0; i<16; i++) *M2++= newMatrix[i];
  return( original );
}

P_Transform *postmult_trans( P_Transform *original, P_Transform *multby )
/* This routine postmultiplies 'original' by 'multby' in place,
 * replacing the contents of original and returning a pointer to original.
 */
{
  int i, row, column;
  float *M1, *M2;
  float newMatrix[16];

  ger_debug("transform: postmult_trans");

  M1= original->d;
  M2= multby->d;

  for (i=0;i<16;i++) newMatrix[i]=0.0;

  for (row = 0;row<4;row++)
    for (column= 0;column<4;column++)
      for (i=0;i<4;i++)
        newMatrix[(4*row)+column] += M1[(4*row)+i]*
          M2[(4*i)+column];

  M1= original->d;
  for (i=0; i<16; i++) *M1++= newMatrix[i];
  return( original );
}

P_Transform *translate_trans( double x, double y, double z )
/* This function returns a translation transformation */
{
  P_Transform *trans;
  float *fptr;
  int row, column;

  ger_debug("transform: translate_trans");

  trans= allocate_trans();
  fptr= trans->d;
  
  for (row=0;row<4;row++)
    for(column=0;column<4;column++)
      {
	if (column == row) fptr[4*row+column] = 1.0;
	else fptr[4*row+column] = 0.0;
      }
  fptr[3]=x;
  fptr[7]=y;
  fptr[11]=z;

  return(trans);
}

P_Transform *rotate_trans( P_Vector *axis, double angle )
/* This function returns a rotation transformation */
{
  float s, c;
  P_Transform *trans;
  float *result, x, y, z, norm;

  ger_debug("transform: rotate_trans");

  x= axis->x;
  y= axis->y;
  z= axis->z;
  norm= sqrt( x*x + y*y + z*z );
  if (norm==0.0) {
    ger_error("transform: rotate_trans: axis degenerate; using z axis");
    x= 0.0;
    y= 0.0;
    z= 1.0;
  }
  else {
    x= x/norm;
    y= y/norm;
    z= z/norm;
  }
  
  s= sin( DegtoRad * angle );
  c= cos( DegtoRad * angle );

  trans= allocate_trans();
  result= trans->d;
  *(result+0)=   x*x + (1.0-x*x)*c;
  *(result+1)=   x*y*(1.0-c) - z*s;
  *(result+2)=   x*z*(1.0-c) + y*s;
  *(result+3)=   0.0;
  *(result+4)=   x*y*(1.0-c) + z*s;
  *(result+5)=   y*y + (1.0-y*y)*c;
  *(result+6)=   y*z*(1.0-c) - x*s;
  *(result+7)=   0.0;
  *(result+8)=   x*z*(1.0-c) - y*s;
  *(result+9)=   y*z*(1.0-c) + x*s;
  *(result+10)=  z*z + (1.0-z*z)*c;
  *(result+11)=  0.0;
  *(result+12)=  0.0;
  *(result+13)=  0.0;
  *(result+14)=  0.0;
  *(result+15)=  1.0;
  
  return( trans );
}

P_Transform *scale_trans( double x, double y, double z )
/* This function returns a scale transformation */
{
  P_Transform *trans;
  float *fptr;
  int row,column;

  ger_debug("transform: scale_trans");

  trans= allocate_trans();
  fptr= trans->d;

  for (row=0;row<4;row++)
    for(column=0;column<4;column++)
      {
	if (column == row) fptr[4*row+column] = 1.0;
	else fptr[4*row+column] = 0.0;
      }
  fptr[0]= fptr[0] * x;
  fptr[5]= fptr[5] * y;
  fptr[10]= fptr[10] * z;

  return(trans);
}

P_Vector *matrix_vector_mult(P_Transform *matrix,P_Vector *vec)
/* This routine multiplies a vector by a matrix. */
{

  int i,j;
  float result[4],vector[4];
  P_Vector *ans;
	
  ger_debug("transform: matrix_vector_mult:");

  if ( !(ans= (P_Vector *)malloc(sizeof(P_Vector))) )
    ger_fatal("transform: matrix_vector_mult: unable to allocate %d bytes!",
	       sizeof(P_Vector) );
	
	vector[0] = vec->x;
	vector[1] = vec->y;
	vector[2] = vec->z;
	vector[3] = 1.0;

        for (i=0; i<4; i++) {
                result[i]= 0.0;
                for (j=0; j<4; j++) {
                        result[i] += matrix->d[4*i+j] * vector[j];
                        }
                }
	ans->x = result[0];
	ans->y = result[1];
	ans->z = result[2];
        return( ans );
}

P_Transform_type *add_type_to_list(P_Transform_type *type,
				   P_Transform_type *t_list)
  /*Adds type to end of list and returns list*/
{
  P_Transform_type *tmp;

  ger_debug("transform: add_type_to_list");

  if(t_list==NULL)
    return( type );

  tmp = t_list;
  while(tmp->next) {
    tmp = tmp->next;
  }

  tmp->next = type;
  
  return( t_list );
}

P_Transform *flip_vec(P_Vector *vec)
{
  float px= 0.0, py= 0.0, pz= 1.0, dot, cx, cy, cz, normsqr;
  P_Vector axis;

  ger_debug("transform: flip_vec: flipping %f %f %f", vec->x, 
	    vec->y, vec->z);

  /* Find a vector not parallel to the given vector */
  dot= px*vec->x + py*vec->y + pz*vec->z;
  cx= py*vec->z - pz*vec->y;
  cy= pz*vec->x - px*vec->z;
  cz= px*vec->y - py*vec->x;
  if ( (cx*cx + cy*cy + cz*cz) < EPSILON*dot*dot ) { /* this p won't work */
    px= 1.0; py= 0.0; pz= 0.0;
    dot= px*vec->x + py*vec->y + pz*vec->z;
    };

  /* Extract the normal component of that vector */
  normsqr= vec->x*vec->x + vec->y*vec->y + vec->z*vec->z;
  px= px - vec->x*dot/normsqr;
  py= py - vec->y*dot/normsqr;
  pz= pz - vec->z*dot/normsqr;

  /* Return a 180 degree rotation about that vector */
  axis.x = px; axis.y = py; axis.z = pz;
  return( rotate_trans(&axis, 180.0) );
}

P_Transform *make_aligning_rotation(P_Vector *v1, P_Vector *v2 )
{
/* Vectors are considered to align if the ratio of their cross product
 * squared to their dot product squared is less than the following value.
 * This routine returns a rotation which will rotate its first
 * parameter vector to align with its second parameter vector.
 */

  float ax, ay, az, dotprod;
  double theta;
  P_Vector axis;
  P_Transform *result;
  int i,j;

  ger_debug("transform: make_aligning_rotation: %f %f %f to %f %f %f",
            v1->x, v1->y, v1->z, v2->x, v2->y, v2->z);

  ax= (  (v1->y)*(v2->z) - (v1->z)*(v2->y)  );
  ay= (  (v1->z)*(v2->x) - (v1->x)*(v2->z)  );
  az= (  (v1->x)*(v2->y) - (v1->y)*(v2->x)  );
  dotprod = (  (v1->x)*(v2->x) + (v1->y)*(v2->y) + (v1->z)*(v2->z)  );

  if ( ((ax*ax) + (ay*ay) + (az*az)) < ( (EPSILON)*(dotprod)*(dotprod) ) ) {
    if (dotprod >= 0.0 ) {
      result = duplicate_trans(Identity_trans);
    } else {
      result = flip_vec(v1);
    }
  }
  else {
    theta= acos( ( v1->x*v2->x+v1->y*v2->y+v1->z*v2->z ) /
                ( sqrt( v1->x*v1->x+v1->y*v1->y+v1->z*v1->z )
                 * sqrt( v2->x*v2->x+v2->y*v2->y+v2->z*v2->z ) ) );
    axis.x = ax; axis.y = ay; axis.z = az;
    result = rotate_trans(&axis,RadtoDeg*theta);
  }
  return( result );
}
