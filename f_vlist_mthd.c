/****************************************************************************
 * f_vlist_mthd.c
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
This module provides the methods and generators for vlist objects.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

typedef struct p_fdata_struct {
  float *x, *y, *z, *nx, *ny, *nz, *r, *g, *b, *a, *v, *v2;
} P_Fdata;

static void f_destroy( VOIDLIST )
/* This is the destroy_self method for non-retained vlists. */
{
  P_Vlist *thisvlist;
  METHOD_IN

  ger_debug("f_vlist_mthd: f_destroy: destroying non-retained f-type vlist");
  thisvlist= (P_Vlist *)po_this;
  free( (P_Void_ptr)(thisvlist->object_data) );
  free( (P_Void_ptr)po_this );
  METHOD_DESTROYED
}

static void printfun( VOIDLIST )
/* This is the print method for vlists */
{
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  int i;

  ger_debug("f_vlist_mthd: printfun: type= %d, length= %d", thislist->type, 
            thislist->length);

  switch (thislist->type) {
  case P3D_CVTX: 
    ind_write("Vertex list with coordinates only; %d vertices:",
              thislist->length);
    ind_eol();
    if ( thislist->data_valid || thislist->retained ) {
      ind_push();
      for (i=0; i<thislist->length; i++) {
	ind_write( "( %f %f %f )",
		  (*thislist->x)(i), (*thislist->y)(i), (*thislist->z)(i) );
	ind_eol();
      }
      ind_pop();
    }
    break;
  case P3D_CCVTX: 
    ind_write("Vertex list with coordinates and colors; %d vertices:",
              thislist->length);
    ind_eol();
    if ( thislist->data_valid || thislist->retained ) {
      ind_push();
      for (i=0; i<thislist->length; i++) {
	ind_write( "( %f %f %f )",
		  (*thislist->x)(i), (*thislist->y)(i), (*thislist->z)(i) );
	ind_eol();
	ind_push();
	ind_write( "color= ( %f %f %f %f )",
		  (*thislist->r)(i),(*thislist->g)(i),(*thislist->b)(i),
		  (*thislist->a)(i) );
	ind_eol();
	ind_pop();
      }
      ind_pop();
    }
    break;
  case P3D_CCNVTX: 
    ind_write(
             "Vertex list with coordinates, colors, and normals; %d vertices:",
             thislist->length);
    ind_eol();
    if ( thislist->data_valid || thislist->retained ) {
      ind_push();
      for (i=0; i<thislist->length; i++) {
	ind_write( "( %f %f %f )",
		  (*thislist->x)(i), (*thislist->y)(i), (*thislist->z)(i) );
	ind_eol();
	ind_push();
	ind_write( "color= ( %f %f %f %f ) ",
		  (*thislist->r)(i),(*thislist->g)(i),(*thislist->b)(i),
		  (*thislist->a)(i) );
	ind_eol();
	ind_write( "normal= ( %f %f %f )",
		  (*thislist->nx)(i),(*thislist->ny)(i),(*thislist->nz)(i) );
	ind_eol();
	ind_pop();
      }
      ind_pop();
    }
    break;
  case P3D_CNVTX: 
    ind_write("Vertex list with coordinates and normals; %d vertices:",
              thislist->length);
    ind_eol();
    if ( thislist->data_valid || thislist->retained ) {
      ind_push();
      for (i=0; i<thislist->length; i++) {
	ind_write( "( %f %f %f )",
		  (*thislist->x)(i), (*thislist->y)(i), (*thislist->z)(i) );
	ind_eol();
	ind_push();
	ind_write( "normal= ( %f %f %f )",
		  (*thislist->nx)(i),(*thislist->ny)(i),(*thislist->nz)(i) );
	ind_eol();
	ind_pop();
      }
      ind_pop();
    }
    break;
  case P3D_CVVTX:
    ind_write("Vertex list with coordinates and values; %d vertices:",
              thislist->length);
    ind_eol();
    if ( thislist->data_valid || thislist->retained ) {
      ind_push();
      for (i=0; i<thislist->length; i++) {
	ind_write( "( %f %f %f )",
		  (*thislist->x)(i), (*thislist->y)(i), (*thislist->z)(i) );
	ind_eol();
	ind_push();
	ind_write( "value= %f ",(*thislist->v)(i) );
	ind_eol();
	ind_pop();
      }
      ind_pop();
    }
    break;
  case P3D_CVNVTX:
    ind_write("Vertex list with coordinates, normals and values; %d vertices:",
              thislist->length);
    ind_eol();
    if ( thislist->data_valid || thislist->retained ) {
      ind_push();
      for (i=0; i<thislist->length; i++) {
	ind_write( "( %f %f %f )",
		  (*thislist->x)(i), (*thislist->y)(i), (*thislist->z)(i) );
	ind_eol();
	ind_push();
	ind_write( "value= %f ",(*thislist->v)(i) );
	ind_write( "normal= ( %f %f %f )",
		  (*thislist->nx)(i),(*thislist->ny)(i),(*thislist->nz)(i) );
	ind_eol();
	ind_pop();
      }
      ind_pop();
    }
    break;
  case P3D_CVVVTX:
    ind_write("Vertex list with coordinates and two values; %d vertices:",
              thislist->length);
    ind_eol();
    if ( thislist->data_valid || thislist->retained ) {
      ind_push();
      for (i=0; i<thislist->length; i++) {
	ind_write( "( %f %f %f )",
		  (*thislist->x)(i), (*thislist->y)(i), (*thislist->z)(i) );
	ind_eol();
	ind_push();
	ind_write( "value= %f ",(*thislist->v)(i) );
	ind_eol();
	ind_write( "value2= %f ",(*thislist->v2)(i) );
	ind_eol();
	ind_pop();
      }
      ind_pop();
    }
    break;
  }

  METHOD_OUT
}

static double f_x(int i)
/* This routine returns the appropriate x value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->x) result= thisdata->x[i];
  else {
    ger_error(
   "f_vlist_mthd: f_x: attempt to get x value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_y(int i)
/* This routine returns the appropriate y value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->y) result= thisdata->y[i];
  else {
    ger_error(
      "f_vlist_mthd: f_y: attempt to get y value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_z(int i)
/* This routine returns the appropriate z value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->z) result= thisdata->z[i];
  else {
    ger_error(
      "f_vlist_mthd: f_z: attempt to get z value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_r(int i)
/* This routine returns the appropriate r value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->r) result= thisdata->r[i];
  else {
    ger_error(
      "f_vlist_mthd: f_r: attempt to get r value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_g(int i)
/* This routine returns the appropriate g value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->g) result= thisdata->g[i];
  else {
    ger_error(
      "f_vlist_mthd: f_g: attempt to get g value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_b(int i)
/* This routine returns the appropriate b value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->b) result= thisdata->b[i];
  else {
    ger_error(
      "f_vlist_mthd: f_b: attempt to get b value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_a(int i)
/* This routine returns the appropriate a value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->a) result= thisdata->a[i];
  else {
    ger_error(
   "f_vlist_mthd: f_a: attempt to get alpha value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_nx(int i)
/* This routine returns the appropriate normal x value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->nx) result= thisdata->nx[i];
  else {
    ger_error(
     "f_vlist_mthd: f_nx: attempt to get nx value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_ny(int i)
/* This routine returns the appropriate normal y value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->ny) result= thisdata->ny[i];
  else {
    ger_error(
     "f_vlist_mthd: f_ny: attempt to get ny value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_nz(int i)
/* This routine returns the appropriate normal z value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->nz) result= thisdata->nz[i];
  else {
    ger_error(
     "f_vlist_mthd: f_nz: attempt to get nz value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_v(int i)
/* This routine returns the appropriate associated value from a f-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->v) result= thisdata->v[i];
  else {
    ger_error(
      "f_vlist_mthd: f_v: attempt to get v value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double f_v2(int i)
/* This routine returns the appropriate associated value2 from a 
 * f-type vlist 
 */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  P_Fdata *thisdata;

  /* called too often for debugging */

  thisdata= (P_Fdata *)thislist->object_data;
  if (thisdata->v2) result= thisdata->v2[i];
  else {
    ger_error(
     "f_vlist_mthd: f_v2: attempt to get v2 value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

P_Vlist *po_create_fvlist( int type, int length, 
			  float *x, float *y, float *z, 
			  float *nx, float *ny, float *nz,
			  float *c1, float *c2, float *c3, float *c4 )
/* This routine creates a non-retained fortran-style vertex list.  Non-
 * retained means that the data is left in the original memory of the
 * calling program, which can mess it up any time it wants.
 */
{
  P_Vlist *thislist;
  P_Fdata *thisdata;

  ger_debug(
	 "f_vlist_mthd: po_create_fvlist: type= %d, length= %d",type, length);

  if ( !(thislist= (P_Vlist *)malloc(sizeof(P_Vlist))) )
    ger_fatal( "f_vlist_mthd: po_create_fvlist: couldn't allocate %d bytes!",
              sizeof(P_Vlist) );

  thislist->type= type;
  thislist->length= length;
  thislist->retained= 0;
  thislist->data_valid= 1;

  if ( !(thisdata= (P_Fdata *)malloc( sizeof(P_Fdata) )) ) 
    ger_fatal("f_vlist_mthd: po_create_fvlist: couldn't allocate %d bytes!",
              sizeof(P_Fdata) );
  thislist->object_data= (P_Void_ptr)thisdata;
  thisdata->x= x;
  thisdata->y= y;
  thisdata->z= z;
  switch (type) {
  case P3D_CVTX: 
    thisdata->nx= (float *)0;
    thisdata->ny= (float *)0;
    thisdata->nz= (float *)0;
    thisdata->r= (float *)0;
    thisdata->g= (float *)0;
    thisdata->b= (float *)0;
    thisdata->a= (float *)0;
    thisdata->v= (float *)0;
    thisdata->v2= (float *)0;
    break;
  case P3D_CCVTX:
    thisdata->nx= (float *)0;
    thisdata->ny= (float *)0;
    thisdata->nz= (float *)0;
    thisdata->r= c1;
    thisdata->g= c2;
    thisdata->b= c3;
    thisdata->a= c4;
    thisdata->v= (float *)0;
    thisdata->v2= (float *)0;
    break;
  case P3D_CCNVTX:
    thisdata->nx= nx;
    thisdata->ny= ny;
    thisdata->nz= nz;
    thisdata->r= c1;
    thisdata->g= c2;
    thisdata->b= c3;
    thisdata->a= c4;
    thisdata->v= (float *)0;
    thisdata->v2= (float *)0;
    break;
  case P3D_CNVTX:
    thisdata->nx= nx;
    thisdata->ny= ny;
    thisdata->nz= nz;
    thisdata->r= (float *)0;
    thisdata->g= (float *)0;
    thisdata->b= (float *)0;
    thisdata->a= (float *)0;
    thisdata->v= (float *)0;
    thisdata->v2= (float *)0;
    break;
  case P3D_CVVTX:
    thisdata->nx= (float *)0;
    thisdata->ny= (float *)0;
    thisdata->nz= (float *)0;
    thisdata->r= (float *)0;
    thisdata->g= (float *)0;
    thisdata->b= (float *)0;
    thisdata->a= (float *)0;
    thisdata->v= c1;
    thisdata->v2= (float *)0;
    break;
  case P3D_CVNVTX:
    thisdata->nx= nx;
    thisdata->ny= ny;
    thisdata->nz= nz;
    thisdata->r= (float *)0;
    thisdata->g= (float *)0;
    thisdata->b= (float *)0;
    thisdata->a= (float *)0;
    thisdata->v= c1;
    thisdata->v2= (float *)0;
    break;
  case P3D_CVVVTX:
    thisdata->nx= (float *)0;
    thisdata->ny= (float *)0;
    thisdata->nz= (float *)0;
    thisdata->r= (float *)0;
    thisdata->g= (float *)0;
    thisdata->b= (float *)0;
    thisdata->a= (float *)0;
    thisdata->v= c1;
    thisdata->v2= c2;
    break;
  }

  thislist->x= f_x;
  thislist->y= f_y;
  thislist->z= f_z;
  thislist->r= f_r;
  thislist->g= f_g;
  thislist->b= f_b;
  thislist->a= f_a;
  thislist->nx= f_nx;
  thislist->ny= f_ny;
  thislist->nz= f_nz;
  thislist->v= f_v;
  thislist->v2= f_v2;
  thislist->print= printfun;
  thislist->destroy_self= f_destroy;

  return( thislist );
}

