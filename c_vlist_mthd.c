/****************************************************************************
 * c_vlist_mthd.c
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

#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

typedef struct p_fdata_struct {
  float *x, *y, *z, *nx, *ny, *nz, *r, *g, *b, *a, *v;
} P_Fdata;

static void c_destroy( VOIDLIST )
/* This is the destroy_self method for non-retained vlists. */
{
  METHOD_IN
  ger_debug("c_vlist_mthd: c_destroy: destroying non-retained c-type vlist");
  free( (P_Void_ptr)po_this );
  METHOD_DESTROYED
}

static void printfun( VOIDLIST )
/* This is the print method for vlists */
{
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;
  int i;

  ger_debug("c_vlist_mthd: printfun: type= %d, length= %d", thislist->type, 
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

static double c_x(int i)
/* This routine returns the appropriate x value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
    result= *( (float *)thislist->object_data + 3*i );
    break;
  case P3D_CCVTX:
    result= *( (float *)thislist->object_data + 7*i );
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i );
    break;
  case P3D_CNVTX:
    result= *( (float *)thislist->object_data + 6*i );
    break;
  case P3D_CVVTX:
    result= *( (float *)thislist->object_data + 4*i );
    break;
  case P3D_CVNVTX:
    result= *( (float *)thislist->object_data + 7*i );
    break;
  case P3D_CVVVTX:
    result= *( (float *)thislist->object_data + 5*i );
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_y(int i)
/* This routine returns the appropriate y value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
    result= *( (float *)thislist->object_data + 3*i + 1 );
    break;
  case P3D_CCVTX:
    result= *( (float *)thislist->object_data + 7*i + 1 );
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 1 );
    break;
  case P3D_CNVTX:
    result= *( (float *)thislist->object_data + 6*i + 1 );
    break;
  case P3D_CVVTX:
    result= *( (float *)thislist->object_data + 4*i + 1 );
    break;
  case P3D_CVNVTX:
    result= *( (float *)thislist->object_data + 7*i + 1 );
    break;
  case P3D_CVVVTX:
    result= *( (float *)thislist->object_data + 5*i + 1 );
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_z(int i)
/* This routine returns the appropriate z value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
    result= *( (float *)thislist->object_data + 3*i + 2 );
    break;
  case P3D_CCVTX:
    result= *( (float *)thislist->object_data + 7*i + 2 );
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 2 );
    break;
  case P3D_CNVTX:
    result= *( (float *)thislist->object_data + 6*i + 2 );
    break;
  case P3D_CVVTX:
    result= *( (float *)thislist->object_data + 4*i + 2 );
    break;
  case P3D_CVNVTX:
    result= *( (float *)thislist->object_data + 7*i + 2 );
    break;
  case P3D_CVVVTX:
    result= *( (float *)thislist->object_data + 5*i + 2 );
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_r(int i)
/* This routine returns the appropriate r value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
    ger_error(
  "c_vlist_mthd: c_r: tried to get red component of a vlist with no color!");
    result= 0.0;
    break;
  case P3D_CCVTX:
    result= *( (float *)thislist->object_data + 7*i + 3 );
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 3 );
    break;
  case P3D_CNVTX:
  case P3D_CVVTX:
  case P3D_CVNVTX:
  case P3D_CVVVTX:
    ger_error(
  "c_vlist_mthd: c_r: tried to get red component of a vlist with no color!");
    result= 0.0;
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_g(int i)
/* This routine returns the appropriate g value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
    ger_error(
  "c_vlist_mthd: c_g: tried to get green component of a vlist with no color!");
    result= 0.0;
    break;
  case P3D_CCVTX:
    result= *( (float *)thislist->object_data + 7*i + 4 );
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 4 );
    break;
  case P3D_CNVTX:
  case P3D_CVVTX:
  case P3D_CVNVTX:
  case P3D_CVVVTX:
    ger_error(
 "c_vlist_mthd: c_g: tried to get green component of a vlist with no color!");
    result= 0.0;
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_b(int i)
/* This routine returns the appropriate b value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
    ger_error(
  "c_vlist_mthd: c_b: tried to get blue component of a vlist with no color!");
    result= 0.0;
    break;
  case P3D_CCVTX:
    result= *( (float *)thislist->object_data + 7*i + 5 );
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 5 );
    break;
  case P3D_CNVTX:
  case P3D_CVVTX:
  case P3D_CVNVTX:
  case P3D_CVVVTX:
    ger_error(
  "c_vlist_mthd: c_b: tried to get blue component of a vlist with no color!");
    result= 0.0;
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_a(int i)
/* This routine returns the appropriate a value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
    ger_error(
 "c_vlist_mthd: c_a: tried to get alpha component of a vlist with no color!");
    result= 0.0;
    break;
  case P3D_CCVTX:
    result= *( (float *)thislist->object_data + 7*i + 6 );
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 6 );
    break;
  case P3D_CNVTX:
  case P3D_CVVTX:
  case P3D_CVNVTX:
  case P3D_CVVVTX:
    ger_error(
 "c_vlist_mthd: c_a: tried to get alpha component of a vlist with no color!");
    result= 0.0;
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_nx(int i)
/* This routine returns the appropriate normal x value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
  case P3D_CCVTX:
  case P3D_CVVTX:
  case P3D_CVVVTX:
    ger_error(
  "c_vlist_mthd: c_nx: tried to get normal x from a vlist with no normals!");
    result= 0.0;
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 7 );
    break;
  case P3D_CNVTX:
    result= *( (float *)thislist->object_data + 6*i + 3 );
    break;
  case P3D_CVNVTX:
    result= *( (float *)thislist->object_data + 7*i + 4 );
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_ny(int i)
/* This routine returns the appropriate normal y value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
  case P3D_CCVTX:
  case P3D_CVVTX:
  case P3D_CVVVTX:
    ger_error(
  "c_vlist_mthd: c_ny: tried to get normal y from a vlist with no normals!");
    result= 0.0;
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 8 );
    break;
  case P3D_CNVTX:
    result= *( (float *)thislist->object_data + 6*i + 4 );
    break;
  case P3D_CVNVTX:
    result= *( (float *)thislist->object_data + 7*i + 5 );
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_nz(int i)
/* This routine returns the appropriate normal z value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
  case P3D_CCVTX:
  case P3D_CVVTX:
  case P3D_CVVVTX:
    ger_error(
  "c_vlist_mthd: c_nz: tried to get normal z from a vlist with no normals!");
    result= 0.0;
    break;
  case P3D_CCNVTX:
    result= *( (float *)thislist->object_data + 10*i + 9 );
    break;
  case P3D_CNVTX:
    result= *( (float *)thislist->object_data + 6*i + 5 );
    break;
  case P3D_CVNVTX:
    result= *( (float *)thislist->object_data + 7*i + 6 );
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_v(int i)
/* This routine returns the appropriate associated value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
  case P3D_CCVTX:
  case P3D_CNVTX:
  case P3D_CCNVTX:
    ger_error(
  "c_vlist_mthd: c_v: tried to get value from a vlist with none associated!");
    result= 0.0;
    break;
  case P3D_CVVTX:
    result= *( (float *)thislist->object_data + 4*i + 3 );
    break;
  case P3D_CVNVTX:
    result= *( (float *)thislist->object_data + 7*i + 3 );
    break;
  case P3D_CVVVTX:
    result= *( (float *)thislist->object_data + 5*i + 3 );
    break;
  }

  METHOD_OUT
  return((double)result);
}

static double c_v2(int i)
/* This routine returns the appropriate associated value from a c-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  switch (thislist->type) {
  case P3D_CVTX:
  case P3D_CCVTX:
  case P3D_CNVTX:
  case P3D_CCNVTX:
  case P3D_CVVTX:
  case P3D_CVNVTX:
  case P3D_CVVVTX:
    result= *( (float *)thislist->object_data + 5*i + 4 );
    break;
  }

  METHOD_OUT
  return((double)result);
}

P_Vlist *po_create_cvlist( int type, int length, float *data )
/* This routine creates a non-retained c-style vertex list.  Non-
 * retained means that the data is left in the original memory of the
 * calling program, which can mess it up any time it wants.
 */
{
  P_Vlist *thislist;

  ger_debug(
	 "c_vlist_mthd: po_create_cvlist: type= %d, length= %d",type, length);

  if ( !(thislist= (P_Vlist *)malloc(sizeof(P_Vlist))) )
    ger_fatal( "c_vlist_mthd: po_create_cvlist: couldn't allocate %d bytes!",
              sizeof(P_Vlist) );

  thislist->type= type;
  thislist->length= length;
  thislist->object_data= (P_Void_ptr)data;
  thislist->retained= 0;
  thislist->data_valid= 1;

  thislist->x= c_x;
  thislist->y= c_y;
  thislist->z= c_z;
  thislist->r= c_r;
  thislist->g= c_g;
  thislist->b= c_b;
  thislist->a= c_a;
  thislist->nx= c_nx;
  thislist->ny= c_ny;
  thislist->nz= c_nz;
  thislist->v= c_v;
  thislist->v2= c_v2;
  thislist->print= printfun;
  thislist->destroy_self= c_destroy;

  return( thislist );
}
