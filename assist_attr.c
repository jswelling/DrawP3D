/****************************************************************************
 * assist_attr.c
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides support for renderers which do not have the
capability to manage all necessary attributes.  It provides a fast 
mechanism to manage attributes during traversal of the
geometry database.  
*/
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"

/*
 * The mechanism is to hash the symbols of each
 * symbol-value pair into a table, and then essentially accumulate
 * a stack of values at that table location.  We do no memory management
 * on the Pairs themselves, because it is assumed that they will be
 * bound into attribute lists on the lisp side for the entire interval
 * when they are bound into stacks in this module.
 *
 * The attributes are stored in a hash table.  We assume that the things
 * begin passed in are pointers to word-aligned objects.  We thus guess
 * that a fair value to hash from is the value passed in, right shifted
 * two bits (to throw away the bits which will always match due to word
 * alignment) and masked to 8 bits.  This allows us to use a hash table
 * size which is a power of two, and generate a hash index without a
 * modulo operation.  This operation is encoded by the following 
 * definitions.
 */
#define SYMBOL_TO_INT( sym ) (((int)sym)>>2)
#define ATTR_HASH_BITS 8

static void push_attributes(P_Attrib_List *attrlist)
/* This routine scans an attribute list, and pushes those values found
 * onto the appropriate stacks.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN;

  ger_debug("assist_attr: push_attributes");

  while ( attrlist ) {
    METHOD_RDY(ATTRHASH(self));
    (void)(*(ATTRHASH(self)->add))( SYMBOL_TO_INT(attrlist->attribute),
				   (P_Void_ptr)attrlist );
    attrlist= attrlist->next;
  }

  METHOD_OUT;
}

static void pop_attributes(P_Attrib_List *attrlist)
/* This routine scans an attribute list, and pops the values found off
 * the appropriate stacks.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN;

  ger_debug("assist_attr: pop_attributes");

  while ( attrlist ) {
    METHOD_RDY(ATTRHASH(self));
    (*(ATTRHASH(self)->free))( SYMBOL_TO_INT( attrlist->attribute ) );
    attrlist= attrlist->next;
  }
  METHOD_OUT;
}

static int bool_attribute(P_Symbol symbol)
/* This routine returns the current value of a boolean attribute.  It
 * returns 0 for false, 1 for true.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Attrib_List *thispair;
  METHOD_IN;

  ger_debug("assist_attr: bool_attribute");

  METHOD_RDY(ATTRHASH(self));
  thispair= 
    (P_Attrib_List *)(*(ATTRHASH(self)->lookup))( SYMBOL_TO_INT( symbol ) );
  if ( !thispair || thispair->type != P3D_BOOLEAN ) ger_fatal(
"assist_attr: bool_attribute: requested attribute undefined or wrong type!");
  METHOD_OUT;
  return( attribute_boolean(thispair) );
}

static P_Color *color_attribute(P_Symbol symbol)
/* This routine returns the current value of a color attribute.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Attrib_List *thispair;
  METHOD_IN;

  ger_debug("assist_attr: color_attribute");

  METHOD_RDY(ATTRHASH(self));
  thispair= 
    (P_Attrib_List *)(*(ATTRHASH(self)->lookup))( SYMBOL_TO_INT( symbol ) );
  if ( !thispair || thispair->type != P3D_COLOR ) ger_fatal(
"assist_attr: color_attribute: requested attribute undefined or wrong type!");
  METHOD_OUT;
  return( attribute_color(thispair) );
}

static P_Material *material_attribute(P_Symbol symbol)
/* This routine returns the current value of a material attribute. */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Attrib_List *thispair;
  METHOD_IN;

  ger_debug("assist_attr: material_attribute");

  METHOD_RDY(ATTRHASH(self));
  thispair= 
    (P_Attrib_List *)(*(ATTRHASH(self)->lookup))( SYMBOL_TO_INT( symbol ) );
  if ( !thispair || thispair->type != P3D_MATERIAL ) ger_fatal(
"assist_attr: material_attribute: requested attr undefined or wrong type!");
  METHOD_OUT;
  return( attribute_material(thispair) );
}

static float float_attribute(P_Symbol symbol)
/* This routine returns the current value of a floating point attribute. 
 */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Attrib_List *thispair;
  METHOD_IN;

  ger_debug("assist_attr: float_attribute");

  METHOD_RDY(ATTRHASH(self));
  thispair= 
    (P_Attrib_List *)(*(ATTRHASH(self)->lookup))( SYMBOL_TO_INT( symbol ) );
  if ( !thispair || thispair->type != P3D_FLOAT ) ger_fatal(
"assist_attr: float_attribute: requested attribute undefined or wrong type!");
  METHOD_OUT;
  return( attribute_float(thispair) );
}

static int int_attribute(P_Symbol symbol)
/* This routine returns the current value of an integer attribute.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Attrib_List *thispair;
  METHOD_IN;

  ger_debug("assist_attr: int_attribute");

  METHOD_RDY(ATTRHASH(self));
  thispair= 
    (P_Attrib_List *)(*(ATTRHASH(self)->lookup))( SYMBOL_TO_INT( symbol ) );
  if ( !thispair || thispair->type != P3D_INT ) ger_fatal(
"assist_attr: int_attribute: requested attribute undefined or wrong type!");
  METHOD_OUT;
  return( attribute_int(thispair) );
}

static char *string_attribute(P_Symbol symbol)
/* This routine returns the current value of a string attribute.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Attrib_List *thispair;
  METHOD_IN;

  ger_debug("assist_attr: string_attribute");

  METHOD_RDY(ATTRHASH(self));
  thispair= 
    (P_Attrib_List *)(*(ATTRHASH(self)->lookup))( SYMBOL_TO_INT( symbol ) );
  if ( !thispair || thispair->type != P3D_STRING ) ger_fatal(
"assist_attr: string_attribute: requested attribute undefined or wrong type!");
  METHOD_OUT;
  return( attribute_string(thispair) );
}

void ast_attr_reset( int hard )
/* This routine resets the attribute part of the assist module, in the 
 * event its symbol environment is reset.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_attr: ast_attr_reset");
  /* But at the moment it does nothing. */

  METHOD_OUT
}

void ast_attr_destroy( void )
/* This destroys the attribute part of an assist object */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_attr: ast_attr_destroy");
  METHOD_RDY( ATTRHASH(self) );
  (*(ATTRHASH(self)->destroy_self))();

  METHOD_OUT
}

void ast_attr_init( P_Assist *self )
/* This initializes the attribute part of an assist object */
{
  ger_debug("assist_attr: ast_attr_init");

  ATTRHASH(self)= po_create_ihash( ATTR_HASH_BITS );

  self->push_attributes= push_attributes;
  self->pop_attributes= pop_attributes;
  self->bool_attribute= bool_attribute;
  self->color_attribute= color_attribute;
  self->material_attribute= material_attribute;
  self->float_attribute= float_attribute;
  self->int_attribute= int_attribute;
  self->string_attribute= string_attribute;
  
}
