/****************************************************************************
 * attribute.c
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
This module provides methods for the manipulation of attribute lists.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

/* Macro to help allocate space */
#define GET_STORAGE( ptr, type ) \
  if ( !( ptr= (type *)malloc( sizeof(type) ) ) ) \
  ger_fatal("attribute: add_attr: unable to allocate %d bytes!", sizeof(type))

P_Attrib_List *add_attr( P_Attrib_List *oldfirst, char *attribute,
                        int type, P_Void_ptr value)
/* This routine adds a cell to an attribute list */
{
  P_Symbol symbol;
  P_Attrib_List *thiscell;
  P_Color *clr;
  P_Point *pt;
  P_Vector *vec;
  int *intp;
  float *floatp;

  ger_debug("attribute: add_attr");

  if ( !(thiscell= (P_Attrib_List *)malloc(sizeof(P_Attrib_List))) )
    ger_fatal("transform: add_attr: unable to allocate %d bytes!\n",
              sizeof(P_Attrib_List));

  thiscell->next= oldfirst;
  thiscell->prev= (P_Attrib_List *)0;
  if (oldfirst) oldfirst->prev= thiscell;
  thiscell->attribute= create_symbol(attribute);
  thiscell->type= type;
  switch (type) {
  case P3D_INT: 
  case P3D_BOOLEAN: 
    GET_STORAGE( intp, int ); 
    *intp= *(int *)value;
    thiscell->value= (P_Void_ptr)intp;
    break;
  case P3D_FLOAT: /* Clumsy notation because DEC cc can't handle casts */
    GET_STORAGE( floatp, float ); 
    *floatp= *(float *)value;
    thiscell->value= (P_Void_ptr)floatp;
    break;
  case P3D_STRING:
    if ( !(thiscell->value= (P_Void_ptr)malloc(strlen((char *)value)+1) ) )
      ger_fatal("attribute: add_attr: unable to allocate %d bytes for string!",
		strlen((char *)value)+1);
    strcpy((char *)thiscell->value, (char *)value);
    break;
  case P3D_COLOR: 
    thiscell->value= (P_Void_ptr)duplicate_color( (P_Color *)value );
    break;
  case P3D_POINT: 
    GET_STORAGE( pt, P_Point );
    thiscell->value= (P_Void_ptr)pt;
    pt->x= ((P_Point *)value)->x;
    pt->y= ((P_Point *)value)->y;
    pt->z= ((P_Point *)value)->z;
    break;
  case P3D_VECTOR: 
    GET_STORAGE( vec, P_Vector );
    thiscell->value= (P_Void_ptr)vec;
    vec->x= ((P_Vector *)value)->x;
    vec->y= ((P_Vector *)value)->y;
    vec->z= ((P_Vector *)value)->z;
    break;
  case P3D_TRANSFORM:
    thiscell->value= (P_Void_ptr)duplicate_trans( (P_Transform *)value );
    break;
  case P3D_MATERIAL:
    thiscell->value= (P_Void_ptr)duplicate_material( (P_Material *)value );
    break;
  case P3D_OTHER: 
    /* Can't do much but copy the pointer and hope it isn't deallcoated */
    thiscell->value= value;
    break;
  }

  return(thiscell);
}

void print_attr(P_Attrib_List *attr)
/* This routine dumps an attribute list */
{
  P_Point *pt;
  P_Vector *vec;
  P_Color *clr;

  ger_debug("gob_mthd: print_attr");

  ind_push();
  while (attr) {
    ind_write("%s: ",symbol_string(attr->attribute)); 
    switch (attr->type) {
    case P3D_INT: ind_write("int %d",*(int *)(attr->value)); ind_eol();
      break;
    case P3D_BOOLEAN: 
      ind_write("boolean ");
      if (*(int *)(attr->value)) ind_write("TRUE"); else ind_write("FALSE"); 
      ind_eol();
      break;
    case P3D_FLOAT: 
      ind_write("float %f",*(float *)(attr->value)); ind_eol();
      break;
    case P3D_STRING: ind_write("string <%s>",(char *)(attr->value)); ind_eol();
      break;
    case P3D_COLOR: 
      dump_color( (P_Color *)attr->value );
      break;
    case P3D_POINT: 
      pt= (P_Point *)attr->value;
      ind_write( "point ( %f %f %f )", pt->x, pt->y, pt->z );
      ind_eol();
      break;
    case P3D_VECTOR: 
      vec= (P_Vector *)attr->value;
      ind_write( "vector ( %f %f %f )", vec->x, vec->y, vec->z );
      ind_eol();
      break;
    case P3D_TRANSFORM: 
      ind_write("transform"); ind_eol(); 
      dump_trans( (P_Transform *)attr->value );
      break;
    case P3D_MATERIAL:
      ind_write("material: "); dump_material( (P_Material *)attr->value );
      break;
    case P3D_OTHER: 
      ind_write("(unprintable)"); ind_eol();
      break;
    }
    attr= attr->next;
  }
  ind_pop();
}

void destroy_attr(P_Attrib_List *attr)
/* This routine walks an attribute list, destroying it. */
{
  P_Attrib_List *this;

  ger_debug("attribute: destroy_attr");

  while (attr) {
    this= attr;
    attr= attr->next;
    switch (this->type) {
    case P3D_INT: 
    case P3D_BOOLEAN: 
    case P3D_FLOAT:
    case P3D_STRING:
    case P3D_POINT: 
    case P3D_VECTOR: 
      free( this->value );
      break;
    case P3D_COLOR: 
      destroy_color( (P_Color *)this->value );
      break;
    case P3D_TRANSFORM:
      destroy_trans( (P_Transform *)this->value );
      break;
    case P3D_MATERIAL:
      destroy_material( (P_Material *)this->value );
      break;
    case P3D_OTHER: 
      /* Wasn't allocated, so do nothing */
      break;
    }
    free( (P_Void_ptr)this );
  }
}

