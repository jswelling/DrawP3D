/****************************************************************************
 * assist_trns.c
 * Author Joel Welling
 * Copyright 1995, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
mechanism to manage transformation matrices during traversal of the
geometry database.  
*/
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"

#define INITIAL_TRANS_STACK_DEPTH 16

static P_Transform* clear_trans_stack( VOIDLIST )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_trns: clear_trans_stack");

  TRANS(self)= TRANS_STACK(self);
  copy_trans(TRANS(self), Identity_trans);

  METHOD_OUT
  return( TRANS(self) );
}

static P_Transform* load_trans( P_Transform *trans_in )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_trns: load_trans");

  copy_trans(TRANS(self), trans_in);

  METHOD_OUT
  return( TRANS(self) );
}

static P_Transform* push_trans( P_Transform *trans_in )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_trns: push_trans");

  if ( (TRANS(self)-TRANS_STACK(self)) >= (TRANS_STACK_DEPTH(self)-1) ) {
    /* Grow the stack */
    int slot_id;
    int new_depth;
    
    new_depth= 2*TRANS_STACK_DEPTH(self);
    slot_id= TRANS(self)-TRANS_STACK(self);
    if (!(TRANS(self)=(P_Transform*)realloc(new_depth*sizeof(P_Transform)))) {
      ger_fatal("assist_trns: push_trans: cannot realloc %d bytes!",
		new_depth*sizeof(P_Transform));
    }
    TRANS_STACK_DEPTH(self)= new_depth;
    TRANS(self)= TRANS(self) + slot_id;
  }
      
  TRANS(self)= TRANS(self)+1;
  copy_trans(TRANS(self), TRANS(self)-1);
  (void)postmult_trans( TRANS(self), trans_in );

  METHOD_OUT
  return( TRANS(self) );
}

static P_Transform* pop_trans( VOIDLIST )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_trns: pop_trans");

  if (TRANS(self)>TRANS_STACK(self)) {
    TRANS(self)= TRANS(self)-1;
  }
  else {
    ger_fatal("assist_trns: transformation stack underflow!");
  }

  METHOD_OUT
  return( TRANS(self) );
}

static P_Transform* get_trans( VOIDLIST )
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_trns: get_trans");

  METHOD_OUT
  return( TRANS(self) );
}

void ast_trns_reset( int hard )
/* This routine resets the attribute part of the assist module, in the 
 * event its symbol environment is reset.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_trns: ast_trns_reset");
  /* But at the moment it does nothing. */

  METHOD_OUT
}

void ast_trns_destroy( void )
/* This destroys the attribute part of an assist object */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_trns: ast_trns_destroy");
  free( (P_Void_ptr)TRANS_STACK(self) );

  METHOD_OUT
}

void ast_trns_init( P_Assist *self )
/* This initializes the attribute part of an assist object */
{
  ger_debug("assist_trns: ast_trns_init");

  TRANS_STACK_DEPTH(self)= INITIAL_TRANS_STACK_DEPTH;
  if (!(TRANS_STACK(self)= 
	(P_Transform*)malloc(TRANS_STACK_DEPTH(self)*sizeof(P_Transform)))) {
    ger_fatal("ast_trns_init: cannot allocate %d bytes!",
	      TRANS_STACK_DEPTH(self)*sizeof(P_Transform));
  }
  TRANS(self)= TRANS_STACK(self);
  copy_trans(TRANS(self), Identity_trans);

  self->clear_trans_stack= clear_trans_stack;
  self->load_trans= load_trans;
  self->push_trans= push_trans;
  self->pop_trans= pop_trans;
  self->get_trans= get_trans;
  
}
