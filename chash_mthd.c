/****************************************************************************
 * chash_mthd.c
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
This module provides methods for character hash tables.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"

/* Struct to hold hash cell */
typedef struct hash_cell_struct {
  struct hash_cell_struct *next;
  struct hash_cell_struct *last;
  char *string;
  P_Void_ptr value;
} P_Hash_Cell;

#define HASH_LIST(htbl,index) (((P_Hash_Cell **)(htbl->object_data))+index)

static char *dup_string(char *string)
/* This function creates a new copy of a string */
{
  char *result;

  ger_debug("chash_mthd: dup_string");
  if ( !(result= (char *)malloc( sizeof(char)*(strlen(string)+1) ) ) )
    ger_fatal("chash_mthd: dup_string: unable to allocate %d bytes!",
	      sizeof(char)*(strlen(string)+1) );
  strcpy(result,string);
  return(result);
}

static int hash_string(P_String_Hash *self, char *string)
/* Returns an integer value between 0 and size-1 to be used as <string>'s
 * index into the hash table
 */
{
  char *p;
  unsigned h=0,g;
  
  for (p=string;*p != '\0';p=p+1)
    {
      h = (h << 4) + (*p);
      if (g = h&0xf0000000)
	{
	  h = h ^ (g >> 24);
	  h = h ^ g;
	}
    }
  ger_debug("hash_string: <%s> -> %d",string,h%(self->size));
  return(h%(self->size));
}

static P_Void_ptr lookup(char *string)
/* This function looks up a string, returning the associated value */
{
  P_String_Hash *self= (P_String_Hash *)po_this;
  P_Hash_Cell *thiscell;
  METHOD_IN

  ger_debug("chash_mthd: lookup: string= <%s>",string);

  for (thiscell= *HASH_LIST(self,hash_string(self,string)); 
       thiscell; thiscell= thiscell->next)
    if ( !strcmp(string,thiscell->string) ) {
      METHOD_OUT;
      return(thiscell->value);
    }
  METHOD_OUT;
  return( (P_Void_ptr)0 );
}

static P_Void_ptr add(char *string, P_Void_ptr value)
/* This function adds a string and associated value to the table, returning
 * the value added.
 */
{
  P_String_Hash *self= (P_String_Hash *)po_this;
  P_Hash_Cell *thiscell;
  P_Hash_Cell **thisslot;
  METHOD_IN

  ger_debug("chash_mthd: add: string= <%s>",string);
  if ( !(thiscell= (P_Hash_Cell *)malloc(sizeof(P_Hash_Cell))) )
    ger_fatal("chash_mthd: add: unable to allocate %d bytes!",
	      sizeof(P_Hash_Cell));

  thisslot= HASH_LIST(self,hash_string(self,string));
  thiscell->string= dup_string(string);
  thiscell->value= value;
  thiscell->next= *thisslot;
  thiscell->last= (P_Hash_Cell *)0;
  if (*thisslot) (*thisslot)->last= thiscell;
  *thisslot= thiscell;

  METHOD_OUT;
  return( value );
}

static void free_cell(char *string)
/* This function frees the most recently added instance of the string
 * and its associated value, removing it from the table.
 */
{
  P_String_Hash *self= (P_String_Hash *)po_this;
  P_Hash_Cell *thiscell;
  METHOD_IN

  ger_debug("chash_mthd: free: string= <%s>",string);

  for (thiscell= *HASH_LIST(self,hash_string(self,string)); 
       thiscell; thiscell= thiscell->next)
    if ( !strcmp(string,thiscell->string) ) {
      if (thiscell->next) thiscell->next->last= thiscell->last;
      if (thiscell->last) thiscell->last->next= thiscell->next;
      else *HASH_LIST(self,hash_string(self,string))= thiscell->next;
      free( (P_Void_ptr)thiscell );
      break;
    }

  METHOD_OUT
}

static void destroy_self( VOIDLIST )
/* This is the destroy method for the hash table. */
{
  P_String_Hash *self= (P_String_Hash *)po_this;
  P_Hash_Cell **table;
  P_Hash_Cell *thiscell, *nextcell;
  int i;
  METHOD_IN
  ger_debug("chash_mthd: destroy_self");

  table= (P_Hash_Cell **)self->object_data;

  /* free all lists */
  for (i=0; i<self->size; i++) {
    nextcell= (P_Hash_Cell *)0;
    for (thiscell= *HASH_LIST(self,i); thiscell; thiscell= nextcell) {
      free( (P_Void_ptr)thiscell->string );
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

static void walk_start( VOIDLIST )
{
  P_String_Hash *self= (P_String_Hash *)po_this;
  METHOD_IN
  self->walk_ptr= (P_Void_ptr)(*HASH_LIST(self,0));
  self->walk_offset= 0;
  METHOD_OUT
}

static P_Void_ptr walk( VOIDLIST )
{
  P_String_Hash *self= (P_String_Hash *)po_this;
  P_Hash_Cell **celltable= (P_Hash_Cell**)(self->object_data);
  P_Hash_Cell* walk_ptr= (P_Hash_Cell*)(self->walk_ptr);
  P_Void_ptr result= NULL;
  METHOD_IN;

  if (walk_ptr) {
    /* Proceed down the current list */
    result= walk_ptr->value;
    walk_ptr= walk_ptr->next;
    self->walk_ptr= (P_Void_ptr)walk_ptr;
  }
  else if (self->walk_offset < self->size) {
    /* Hop to next list */
    do {
      self->walk_offset++;
    }
    while ( (self->walk_offset < self->size) 
	   && !(*HASH_LIST(self,self->walk_offset)) );
    if (self->walk_offset<self->size) {
      walk_ptr= *HASH_LIST(self,self->walk_offset);
      self->walk_ptr= (P_Void_ptr)walk_ptr;
      METHOD_RDY(self);
      result= walk(); /* Step firmly onto new list */
    }
    else result= NULL;
  }
  else {
    result= NULL;
    self->walk_ptr= NULL;
    self->walk_offset= self->size;
  }

  METHOD_OUT;
  return result;
}

P_String_Hash *po_create_chash( int size )
/* This function returns a character hash table.  size is the table size;
 * it should be prime.
 */
{
  P_String_Hash *thishash;
  P_Hash_Cell **celltable;
  int i;

  ger_debug("po_create_chash: size= %d", size);

  /* Create the hash table object */
  if ( !(thishash=(P_String_Hash *)malloc( sizeof(P_String_Hash) ) ) )
    ger_fatal("chash_mthd: po_create_chash: unable to allocate %d bytes!",
	      sizeof(P_String_Hash) );

  /* Create and initialize the table itself */
  if ( !(celltable=(P_Hash_Cell **)malloc( size*sizeof(P_Hash_Cell *) )) )
    ger_fatal("chash_mthd: po_create_chash: unable to allocate %d bytes!",
	      size*sizeof(P_Hash_Cell *) );
  thishash->object_data= (P_Void_ptr)celltable;
  for (i=0; i<size; i++) *celltable++= (P_Hash_Cell *)0;

  /* Fill in object data */
  thishash->size= size;
  thishash->walk_ptr= (P_Void_ptr)(*HASH_LIST(thishash,0));
  thishash->walk_offset= 0;
  thishash->lookup= lookup;
  thishash->add= add;
  thishash->free= free_cell;
  thishash->destroy_self= destroy_self;
  thishash->walk_start= walk_start;
  thishash->walk= walk;

  return(thishash);
}
