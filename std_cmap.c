/****************************************************************************
 * std_cmap.h
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
This module defines the standard color map functions.
*/
#include <stdio.h>
#include <math.h>
#include "std_cmap.h"

#define PI 3.14159265
#define SQR(x) (x)*(x)
#define MIN(x,y) (x<y) ? x : y
#define GAUSS(x) exp( -(x)*(x) )

static void gray( float *val, float *r, float *g, float *b, float *a )
/* This is the gray scale map */
{
  /* No debugging; called too often */

  *r= *val;
  *g= *val;
  *b= *val;
  *a= 1.0;
}

static void blueyellow( float *val, float *r, float *g, float *b, float *a )
/* This is the blue-to-yellow map */
{
  /* No debugging; called too often */

  *r= *val;
  *g= *val;
  *b= 1.0 - *val;
  *a= 1.0;
}

static void waves( float *val, float *r, float *g, float *b, float *a )
/* This is the 'waves' map */
{
  double theta;
  /* No debugging; called too often */

  theta= *val * PI/2.0;
  *r= SQR( cos(theta) );
  *g= 0.8*pow( sin(10.0*theta), 6.0 );
  *b= SQR( sin(theta) );
  *a= 1.0;
}

static void pseudospect( float *val, float *r, float *g, float *b, float *a )
/* This is the pseudospectral map */
{
  double phase, temp;
  /* No debugging; called too often */

  phase= *val * 4.0;
  temp= GAUSS( phase-4.0 );
  *r= MIN( GAUSS( phase-1.0 ) + temp, 1.0 );
  *g= MIN( GAUSS( phase-2.0 ) + temp, 1.0 );
  *b= MIN( GAUSS( phase-3.0 ) + temp, 1.0 );
  *a= 1.0;
}

static void invertspect( float *val, float *r, float *g, float *b, float *a )
/* This is the inverted pseudospectral map */
{
  double phase, temp;
  /* No debugging; called too often */

  phase= (1.0 - *val) * 4.0;
  temp= GAUSS( phase-4.0 );
  *r= MIN( GAUSS( phase-1.0 ) + temp, 1.0 );
  *g= MIN( GAUSS( phase-2.0 ) + temp, 1.0 );
  *b= MIN( GAUSS( phase-3.0 ) + temp, 1.0 );
  *a= 1.0;
}

/* The following is necessary to assure that po_standard_mapfuns is
 * properly initialized under VAX VMS C.
 */
#ifdef VMS
globaldef
#endif
void (*po_standard_mapfuns[NUM_STANDARD_MAPFUNS])
                       ( float *, float *, float *, float *, float * )= {
       gray,
       blueyellow,
       waves,
       pseudospect,
       invertspect
     };
