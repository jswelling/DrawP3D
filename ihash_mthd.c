/****************************************************************************
 * ihash_mthd.c
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
This module provides methods for integer hash tables.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"

/* Struct to hold hash cell */
typedef struct hash_cell_struct {
  struct hash_cell_struct *next;
  struct hash_cell_struct *last;
  int key;
  P_Void_ptr value;
} P_Hash_Cell;

#define HASH_LIST(htbl,index) (((P_Hash_Cell **)(htbl->object_data))+index)

#define hash_key( self, key ) ( key & (self->mask) )

static P_Void_ptr lookup(int key)
/* This function looks up a key, returning the associated value */
{
  P_Int_Hash *self= (P_Int_Hash *)po_this;
  P_Hash_Cell *thiscell;
  METHOD_IN

  ger_debug("ihash_mthd: lookup: key= %d",key);

  for (thiscell= *HASH_LIST(self,hash_key(self,key)); 
       thiscell; thiscell= thiscell->next)
    if ( thiscell->key == key ) {
      METHOD_OUT;
      return(thiscell->value);
    }
  METHOD_OUT;
  return( (P_Void_ptr)0 );
}

static P_Void_ptr add(int key, P_Void_ptr value)
/* This function adds a key and associated value to the table, returning
 * the value added.
 */
{
  P_Int_Hash *self= (P_Int_Hash *)po_this;
  P_Hash_Cell *thiscell;
  P_Hash_Cell **thisslot;
  METHOD_IN

  ger_debug("ihash_mthd: add: key= %d",key);
  if ( !(thiscell= (P_Hash_Cell *)malloc(sizeof(P_Hash_Cell))) )
    ger_fatal("ihash_mthd: add: unable to allocate %d bytes!",
	      sizeof(P_Hash_Cell));

  thisslot= HASH_LIST(self,hash_key(self,key));
  thiscell->key= key;
  thiscell->value= value;
  thiscell->next= *thisslot;
  thiscell->last= (P_Hash_Cell *)0;
  if (*thisslot) (*thisslot)->last= thiscell;
  *thisslot= thiscell;

  METHOD_OUT;
  return( value );
}

static void free_cell(int key)
/* This function frees the most recently added instance of the key
 * and its associated value, removing it from the table.
 */
{
  P_Int_Hash *self= (P_Int_Hash *)po_this;
  P_Hash_Cell *thiscell;
  METHOD_IN

  ger_debug("ihash_mthd: free: key= %d",key);

  for (thiscell= *HASH_LIST(self,hash_key(self,key)); 
       thiscell; thiscell= thiscell->next)
    if ( key == thiscell->key ) {
      if (thiscell->next) thiscell->next->last= thiscell->last;
      if (thiscell->last) thiscell->last->next= thiscell->next;
      else *HASH_LIST(self,hash_key(self,key))= thiscell->next;
      free( (P_Void_ptr)thiscell );
      break;
    }

  METHOD_OUT
}

static void destroy_self( VOIDLIST )
/* This is the destroy method for the hash table. */
{
  P_Int_Hash *self= (P_Int_Hash *)po_this;
  P_Hash_Cell **table;
  P_Hash_Cell *thiscell, *nextcell;
  int i;
  METHOD_IN
  ger_debug("ihash_mthd: destroy_self");

  table= (P_Hash_Cell **)self->object_data;

  /* free all lists */
  for (i=0; i<self->size; i++) {
    nextcell= (P_Hash_Cell *)0;
    for (thiscell= *HASH_LIST(self,i); thiscell; thiscell= nextcell) {
      free( (P_Void_ptr)thiscell->key );
      nextcell= thiscell->next;
      free( (P_Void_ptr)thiscell );
    }
  }

  /* free the table */
  free( self->object_data );
  
  /* free object storage */
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

P_Int_Hash *po_create_ihash( int mask_bits )
/* This function returns a character hash table.  size is the table size;
 * it should be prime.
 */
{
  P_Int_Hash *thishash;
  P_Hash_Cell **celltable;
  int size;
  int mask;
  int i;

  ger_debug("po_create_ihash: mask_bits= %d", mask_bits);

  size= 1 << mask_bits;
  mask= size-1;

  /* Create the hash table object */
  if ( !(thishash=(P_Int_Hash *)malloc( sizeof(P_Int_Hash) ) ) )
    ger_fatal("ihash_mthd: po_create_ihash: unable to allocate %d bytes!",
	      sizeof(P_Int_Hash) );

  /* Create and initialize the table itself */
  if ( !(celltable=(P_Hash_Cell **)malloc( size*sizeof(P_Hash_Cell *) )) )
    ger_fatal("ihash_mthd: po_create_ihash: unable to allocate %d bytes!",
	      size*sizeof(P_Hash_Cell *) );
  thishash->object_data= (P_Void_ptr)celltable;
  for (i=0; i<size; i++) *celltable++= (P_Hash_Cell *)0;

  /* Fill in object data */
  thishash->size= size;
  thishash->mask= mask;
  thishash->lookup= lookup;
  thishash->add= add;
  thishash->free= free_cell;
  thishash->destroy_self= destroy_self;

  return(thishash);
}
