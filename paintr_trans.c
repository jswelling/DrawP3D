/****************************************************************************
 * paintr_trans.c
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Chris Nuuja
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
      This file defines the functions used for matrix creation and 
      multiplication
      */

#include <ctype.h>
#include <math.h>
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "gen_paintr_strct.h"
#include "gen_painter.h"

/*				VARIABLE DECLARATION			*/



float *pnt_make3dScale(float Sx, float Sy, float Sz)
{
  float *newScale;
  register int row,column;
  
  newScale = (float *) malloc(16*sizeof(float));
  if (!newScale) 
    ger_fatal(
       "paintr_trans: pnt_make3dScale: unable to allocate 16 floats!\n");
  for (row=0;row<4;row++)
    for(column=0;column<4;column++)
      {
	if (column == row)	newScale[4*row+column] = 1.0;
	else newScale[4*row+column] = 0.0;
      }
  newScale[0]= newScale[0] * Sx;
  newScale[5]= newScale[5] * Sy;
  newScale[10]= newScale[10] * Sz;
  return(newScale);
}

float *pnt_make3dRotate(float Angle, char *Axis)
{
  register float *newRotat,radAngle;
  int row,column,i;
  
  radAngle = Angle * DegtoRad;
  newRotat = (float *) malloc(16*sizeof(float));
  if (!newRotat) 
    ger_fatal(
       "paintr_trans: pnt_make3dRotate: unable to allocate 16 floats!\n");
  for (i=0;i<16;i++) newRotat[i]=0.0;

  
  switch(Axis[0])
    {
    case 'x':
      newRotat[0]= 1.0;
      newRotat[5]= cos(radAngle);
      newRotat[6]= sin(radAngle);
      newRotat[9]= 0.0-sin(radAngle);
      newRotat[10]= cos(radAngle);
      newRotat[15]= 1.0;
      break;
    case 'y':
      newRotat[0]= cos(radAngle);
      newRotat[2]= 0.0-sin(radAngle);
      newRotat[5]= 1.0;
      newRotat[8]= sin(radAngle);
      newRotat[10]= cos(radAngle);
      newRotat[15]= 1.0;
      break;
    case 'z':
      newRotat[0]= cos(radAngle);
      newRotat[1]= sin(radAngle);
      newRotat[4]= 0.0-sin(radAngle);
      newRotat[5]= cos(radAngle);
      newRotat[10]= 1.0;
      newRotat[15]= 1.0;
      break;
      default :
	ger_error("UNKOWN AXIS %s INPUT USING Z",Axis);
      newRotat[0]= cos(radAngle);
      newRotat[1]= sin(radAngle);
      newRotat[4]= 0.0-sin(radAngle);
      newRotat[5]= cos(radAngle);
      newRotat[10]= 1.0;
      newRotat[15]= 1.0;
      break;
    }
  
  return(newRotat);
}

float *pnt_make3dTrans(float Tx, float Ty, float Tz)
{
  float *newTrans;
  register int row,column;
  
  newTrans = (float *) malloc(16*sizeof(float));
  if (!newTrans) 
    ger_fatal(
	"paintr_trans: pnt_make3dTrans: unable to allocate 16 floats!\n");
  for (row=0;row<4;row++)
    for(column=0;column<4;column++)
      {
	if (column == row)	newTrans[4*row+column] = 1.0;
	else newTrans[4*row+column] = 0.0;
      }
  newTrans[12]=Tx;
  newTrans[13]=Ty;
  newTrans[14]=Tz;
  return(newTrans);
}

float *pnt_mult3dMatrices(register float M1[16], register float M2[16])
{
	register int row,column,i;
	register float *newMatrix;

	newMatrix = (float *) malloc(16*sizeof(float));

	for (i=0;i<16;i++)	newMatrix[i]=0.0;
	for (row = 0;row<4;row++)
		for (column= 0;column<4;column++)
			for (i=0;i<4;i++)
				newMatrix[(4*row)+column] += M1[(4*row)+i]*
				   M2[(4*i)+column];
	

	return(newMatrix);
}

