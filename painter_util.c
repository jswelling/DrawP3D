/****************************************************************************
 * painter_util.c
 * Author Chris Nuuja
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

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "gen_paintr_strct.h"
#include "gen_painter.h"

/*				FUNCTION  DECLARATION			*/


/* Calculate polygon normal */
void pnt_calc_normal(P_Renderer *self, Pnt_Polytype *polygon, float *trans,
		     Pnt_Vectortype *result)
{
  int x_index, y_index, z_index;
  float *xp, *yp, *zp;
  float v1x, v1y, v1z, v2x, v2y, v2z, nx, ny, nz;
  register float *rtrans= trans;

  if (polygon->numcoords < 3) {
    ger_error("painter_util: pnt_calc_normal: passed a polygon with %d coords",
	    polygon->numcoords);
    result->x= result->y= result->z= 0.0;
  }

  xp= polygon->xcoords;
  yp= polygon->ycoords;
  zp= polygon->zcoords;

  /* Find edge vector components in model coordinate system */
  v1x= *(xp+1) - *xp;
  v2x= *(xp+2) - *(xp+1);
  v1y= *(yp+1) - *yp;
  v2y= *(yp+2) - *(yp+1);
  v1z= *(zp+1) - *zp;
  v2z= *(zp+2) - *(zp+1);

  /* Find normal in model coordinate system */
  nx= v1y*v2z - v2y*v1z;
  ny= v1z*v2x - v2z*v1x;
  nz= v1x*v2y - v2x*v1y;

  /* Transform normal to world coordinates */
  result->x = rtrans[0]*nx + rtrans[4]*ny + rtrans[8]*nz;
  result->y = rtrans[1]*nx + rtrans[5]*ny + rtrans[9]*nz;
  result->z = rtrans[2]*nx + rtrans[6]*ny + rtrans[10]*nz;
}

float pnt_vector_length(register Pnt_Vectortype *vector)
{
	float result;
	
	result = (float) sqrt ( ((vector->x * vector->x) + 
				 (vector->y * vector->y) +
			         (vector->z * vector->z)) );
	if (result < 0.0)
		result = 0.0 - result;

	return(result);
}

float pnt_dotproduct(register Pnt_Vectortype *v1, register Pnt_Vectortype *v2)
{
	float result;

	result = (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
	return(result);
}


void pnt_append3dMatrices(float *oldMatrix, float *newMatrix)
{
	int row,column,i;
	float temp[16];

	for (i=0;i<16;i++)	
		{
		temp[i]=oldMatrix[i];
		oldMatrix[i]=0;
		}
	for (row = 0;row<4;row++)
		for (column= 0;column<4;column++)
			for (i=0;i<4;i++)
				oldMatrix[(4*row)+column] += temp[(4*row)+i]*
				   newMatrix[(4*i)+column];
	

}

Pnt_Vectortype *pnt_vector_matrix_mult3d(Pnt_Vectortype *vector, float *matrix)
{
	Pnt_Vectortype *new_vector;

	new_vector = pnt_makevector_rec(1.0,1.0,1.0);
	new_vector->x = matrix[0]*vector->x + matrix[4]*vector->y 
	  + matrix[8]*vector->z;
	new_vector->y = matrix[1]*vector->x + matrix[5]*vector->y 
	  + matrix[9]*vector->z;	
	new_vector->z = matrix[2]*vector->x + matrix[6]*vector->y 
	  + matrix[10]*vector->z;	

	return(new_vector);
}

/*
   Global Data Used:NONE
   Expl: debug printing routine 
*/
static void print_poly(poly)
Pnt_DPolytype *poly;
{
/*
	int i;

	printf("rendering poly with %d coords",poly->numcoords);
	for (i=0;i<poly->numcoords;i++)
		printf("C:(%f %f %f)",poly->xcoords[i],
					   poly->ycoords[i],
					   poly->zcoords[i]);
	printf("Color:(%f %f %f)",poly->color.r,poly->color.g,
	   poly->color.b);
	printf("Normal:(%f %f %f)",poly->normal.x,poly->normal.y,
	   poly->normal.z);
*/
}

/*
   Global Data Used: NONE
   Expl:debug printing routine
*/
static void print_matrix(matrix)
float *matrix;
{
	int i,j;
	
	printf("Matrix::\n");
	for (i=0;i<4;i++)
		{
		printf(" ( ");
		for (j=0;j<4;j++)
			printf(" %f ",matrix[4*i+j]);
		printf(" )\n ");
		}
	printf("\n");	
}

static int new_DepthCoords(P_Renderer *self, int numcoords)
{
  int result;
  
  if (DCOORDINDEX(self)+numcoords >= MAXDCOORDINDEX(self))
    {
      MAXDCOORDINDEX(self) = 2 * MAXDCOORDINDEX(self);
      DCOORDBUFFER(self) = (float *)
	realloc(DCOORDBUFFER(self),
		MAXDCOORDINDEX(self)*sizeof(float));
      if (!DCOORDBUFFER(self))
	{
	  ger_fatal("ERROR, Could not reallocate %d Coordinates",
		 numcoords);
	}
      return(new_DepthCoords(self,numcoords));
    }
  else
    {
      result = DCOORDINDEX(self);
      DCOORDINDEX(self) += numcoords;
      return(result);
    }
}

int new_DepthColor(P_Renderer *self)
{
	if (DCOLORCOUNT(self) >= MAXDCOLORCOUNT(self))
		{
		MAXDCOLORCOUNT(self) = 2 * MAXDCOLORCOUNT(self);
		DCOLORBUFFER(self)  = (Pnt_Colortype *)
		      realloc(DCOLORBUFFER(self),
			      MAXDCOLORCOUNT(self)*sizeof(Pnt_Colortype));
		if (!DCOLORBUFFER(self))
			{
			ger_fatal("ERROR, Could not reallocate %d Colors",
				MAXDCOLORCOUNT(self));
			};
		return(new_DepthColor(self));
		}
	else
		return(DCOLORCOUNT(self)++);
}

int pnt_new_DepthPoly(P_Renderer *self)
{
	if (DEPTHPOLYCOUNT(self)+1 == MAXDEPTHPOLY(self))
		{
		MAXDEPTHPOLY(self) = 2 * MAXDEPTHPOLY(self);
		DPOLYBUFFER(self) = (Pnt_DPolytype *)
			realloc(DPOLYBUFFER(self),
				MAXDEPTHPOLY(self)*sizeof(Pnt_DPolytype));
		if (!DPOLYBUFFER(self))
			{
			ger_fatal("ERROR, Could not reallocate %d Polygons",
				MAXDEPTHPOLY(self));
			};
		DEPTHBUFFER(self) = (depthtable_rec *)
		       realloc( DEPTHBUFFER(self),
				MAXDEPTHPOLY(self)*sizeof(depthtable_rec) );
		return(pnt_new_DepthPoly(self));
		}
	else
		return(DEPTHPOLYCOUNT(self)++);
}

int pnt_new_DLightIndex(P_Renderer *self)
{
  if (DLIGHTCOUNT(self)+1 >= MAXDLIGHTCOUNT(self))
    {
      MAXDLIGHTCOUNT(self) = 2 * MAXDLIGHTCOUNT(self);
      DLIGHTBUFFER(self) = (Pnt_Lighttype *)
	realloc(DLIGHTBUFFER(self), 
		MAXDLIGHTCOUNT(self)*sizeof(Pnt_Lighttype));
      if (!DLIGHTBUFFER(self))
	{
	  ger_fatal("ERROR, Could not reallocate %d Lights",
		 MAXDLIGHTCOUNT(self));
	}
      return(pnt_new_DLightIndex(self));
    }
  else
    return(DLIGHTCOUNT(self)++);
}

void pnt_fillDpoly_rec(P_Renderer *self, int poly, int numcoords, 
		       primtype type)
{
  Pnt_DPolytype *newpoly;
  
  newpoly = DPOLYBUFFER(self)+poly;
  newpoly->x_index = new_DepthCoords(self,numcoords);
  newpoly->y_index = new_DepthCoords(self,numcoords);
  newpoly->z_index = new_DepthCoords(self,numcoords);
  
  newpoly->numcoords = numcoords; 
  newpoly->type = type;
}

/*
   Global Data Used:NONE
   Expl:  returns a pointer to a vector record with the input values.
*/
Pnt_Vectortype *pnt_makevector_rec(float x, float y, float z)
{
  Pnt_Vectortype *vector;
  
  ger_debug("pnt_makevector_rec (%f %f %f)",x,y,z);
  vector = (Pnt_Vectortype *) malloc(sizeof(Pnt_Vectortype));
  if (!vector) 
    ger_fatal(
       "painter_util: pnt_makevector_rec: unable to allocate %d bytes!\n",
	      sizeof(Pnt_Vectortype));
  vector->x = x;
  vector->y = y;
  vector->z = z;
  return(vector);
}

/*
   Global Data Used:NONE
   Expl:  returns a pointer to a point record.  You might as well make an
	  instance of Pnt_Vectortype.
*/
Pnt_Pointtype *pnt_makepoint_rec(float x, float y, float z)
{
  Pnt_Pointtype *point;
  
  ger_debug("pnt_makepoint_rec (%f %f %f)",x,y,z);
  point = (Pnt_Pointtype *) malloc(sizeof(Pnt_Pointtype));
  if (!point) 
    ger_fatal(
	 "painter_util: pnt_makepoint_rec: unable to allocate %d bytes!\n",
	      sizeof(Pnt_Vectortype));
  point->x = x;
  point->y = y;
  point->z = z;
  return(point);
}

/*
   Global Data Used:NONE
   Expl:  this returns a pointer to a vector of unit length pointing from 
	  <pnt1> to <pnt2>
*/
Pnt_Vectortype *pnt_make_directionvector(Pnt_Pointtype *pnt1, 
					 Pnt_Pointtype *pnt2)
{
  Pnt_Vectortype *vector;
  float length;
  
  ger_debug("pnt_make_directionvector (%f %f %f)",
	    pnt1->x,pnt1->y,pnt1->z);
  vector = (Pnt_Vectortype *) malloc(sizeof(Pnt_Vectortype));
  if (!vector) 
    ger_fatal(
      "painter_util: pnt_make_directionvector: unable to allocate %d bytes!\n",
	      sizeof(Pnt_Vectortype));
  vector->x = pnt1->x - pnt2->x;
  vector->y = pnt1->y - pnt2->y;
  vector->z = pnt1->z - pnt2->z;
  length = pnt_vector_length(vector);
  if (length == 0.0)
    length = HUGE;
  vector->x /= length;
  vector->y /= length;
  vector->z /= length;
  
  return(vector);
}

/*
   Global Data Used:NONE
   Expl:  returns a pointer to a color record with the input values.
*/
int pnt_makeDcolor_rec( P_Renderer *self, 
		       double r, double g, double b, double a )
{
	int new_color;

	ger_debug("make_color (%f %f %f %f)",r,g,b,a);
	new_color = new_DepthColor(self);
	DCOLORBUFFER(self)[new_color].r = r;
	DCOLORBUFFER(self)[new_color].g = g;
	DCOLORBUFFER(self)[new_color].b = b;
	DCOLORBUFFER(self)[new_color].a = a;
	return(new_color);
}


