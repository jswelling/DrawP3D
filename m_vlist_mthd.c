/****************************************************************************
 * m_vlist_mthd.c
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

typedef struct p_mdata_struct {
  float *coords, *normals, *colors, *values;
  int value_stride;
} P_Mdata;
#define DATA(self) ((P_Mdata *)(self->object_data))
#define HAS_COORDS(self) (DATA(self)->coords)
#define HAS_NORMALS(self) (DATA(self)->normals)
#define HAS_COLORS(self) (DATA(self)->colors)
#define HAS_VALUES(self) (DATA(self)->values)
#define COORDS(self,axis,index) (*((DATA(self)->coords)+3*index+axis))
#define COLORS(self,axis,index) (*((DATA(self)->colors)+4*index+axis))
#define NORMALS(self,axis,index) (*((DATA(self)->normals)+3*index+axis))
#define VALUES(self,index) \
  (*((DATA(self)->values)+(DATA(self)->value_stride*index)))
#define VALUES2(self,index) \
  (*((DATA(self)->values)+(DATA(self)->value_stride*index)+1))

static void m_destroy( VOIDLIST )
/* This is the destroy_self method for non-retained vlists. */
{
  P_Vlist *thisvlist;
  METHOD_IN

  ger_debug("m_vlist_mthd: m_destroy: destroying non-retained m-type vlist");
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

  ger_debug("m_vlist_mthd: printfun: type= %d, length= %d", thislist->type, 
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

static double m_x(int i)
/* This routine returns the appropriate x value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_COORDS(thislist)) result= COORDS(thislist,0,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get x value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_y(int i)
/* This routine returns the appropriate y value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_COORDS(thislist)) result= COORDS(thislist,1,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get y value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_z(int i)
/* This routine returns the appropriate z value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_COORDS(thislist)) result= COORDS(thislist,2,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get z value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_r(int i)
/* This routine returns the appropriate r value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_COLORS(thislist)) result= COLORS(thislist,0,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get r value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_g(int i)
/* This routine returns the appropriate g value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_COLORS(thislist)) result= COLORS(thislist,1,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get g value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_b(int i)
/* This routine returns the appropriate b value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_COLORS(thislist)) result= COLORS(thislist,2,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get b value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_a(int i)
/* This routine returns the appropriate a value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_COLORS(thislist)) result= COLORS(thislist,3,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get a value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_nx(int i)
/* This routine returns the appropriate normal x value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_NORMALS(thislist)) result= NORMALS(thislist,0,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get nx value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_ny(int i)
/* This routine returns the appropriate normal y value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_NORMALS(thislist)) result= NORMALS(thislist,1,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get ny value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_nz(int i)
/* This routine returns the appropriate normal z value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_NORMALS(thislist)) result= NORMALS(thislist,2,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get nz value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_v(int i)
/* This routine returns the appropriate associated value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if (HAS_VALUES(thislist)) result= VALUES(thislist,i);
  else {
    ger_error(
   "m_vlist_mthd: m_x: attempt to get v value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

static double m_v2(int i)
/* This routine returns the appropriate associated value from a m-type vlist */
{
  float result;
  METHOD_IN
  P_Vlist *thislist= (P_Vlist *)po_this;

  /* called too often for debugging */

  if ( HAS_VALUES(thislist) && DATA(thislist)->value_stride>=2 ) 
    result= VALUES2(thislist,i);
  else {
    ger_error(
   "m_vlist_mthd: m_v2: attempt to get v2 value from vlist which lacks it.");
    result= 0.0;
  }

  METHOD_OUT
  return((double)result);
}

P_Vlist *po_create_mvlist( int type, int length, 
			  float *coord, float *color, float *normal )
/* This routine creates a non-retained fortran-style vertex list.  Non-
 * retained means that the data is left in the original memory of the
 * calling program, which can mess it up any time it wants.
 */
{
  P_Vlist *thislist;
  P_Mdata *thisdata;

  ger_debug(
	 "m_vlist_mthd: po_create_mvlist: type= %d, length= %d",type, length);

  if ( !(thislist= (P_Vlist *)malloc(sizeof(P_Vlist))) )
    ger_fatal( "m_vlist_mthd: po_create_mvlist: couldn't allocate %d bytes!",
              sizeof(P_Vlist) );

  thislist->type= type;
  thislist->length= length;
  thislist->retained= 0;
  thislist->data_valid= 1;

  if ( !(thisdata= (P_Mdata *)malloc( sizeof(P_Mdata) )) ) 
    ger_fatal("m_vlist_mthd: po_create_mvlist: couldn't allocate %d bytes!",
              sizeof(P_Mdata) );
  thislist->object_data= (P_Void_ptr)thisdata;
  thisdata->coords= coord;
  switch (type) {
  case P3D_CVTX: break;
  case P3D_CCVTX:
    thisdata->normals= (float *)0;
    thisdata->colors= color;
    thisdata->values= (float *)0;
    thisdata->value_stride= 1;
    break;
  case P3D_CCNVTX:
    thisdata->normals= normal;
    thisdata->colors= color;
    thisdata->values= (float *)0;
    thisdata->value_stride= 1;
    break;
  case P3D_CNVTX:
    thisdata->normals= normal;
    thisdata->colors= (float *)0;
    thisdata->values= (float *)0;
    thisdata->value_stride= 1;
    break;
  case P3D_CVVTX:
    thisdata->normals= (float *)0;
    thisdata->colors= (float *)0;
    thisdata->values= color;
    thisdata->value_stride= 1;
    break;
  case P3D_CVNVTX:
    thisdata->normals= normal;
    thisdata->colors= (float *)0;
    thisdata->values= color;
    thisdata->value_stride= 1;
    break;
  case P3D_CVVVTX:
    thisdata->normals= (float *)0;
    thisdata->colors= (float *)0;
    thisdata->values= color;
    thisdata->value_stride= 2;
    break;
  }

  thislist->x= m_x;
  thislist->y= m_y;
  thislist->z= m_z;
  thislist->r= m_r;
  thislist->g= m_g;
  thislist->b= m_b;
  thislist->a= m_a;
  thislist->nx= m_nx;
  thislist->ny= m_ny;
  thislist->nz= m_nz;
  thislist->v= m_v;
  thislist->v2= m_v2;
  thislist->print= printfun;
  thislist->destroy_self= m_destroy;

  return( thislist );
}

