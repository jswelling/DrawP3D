/****************************************************************************
 * assist.c
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
capability to manage everything themselves..
*/
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"

static void reset_self( int hard )
/* This routine resets the assist module, in the event its symbol
 * environment is reset.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist: reset_self: hard= %d", hard);
  METHOD_RDY(self);
  ast_attr_reset( hard );
  ast_prim_reset( hard );
  ast_spln_reset( hard );
  ast_text_reset( hard );
  ast_trns_reset( hard );

  METHOD_OUT
}

static void destroy_self( void )
/* This destroys the assist object */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist: destroy_self");
  METHOD_RDY(self);
  ast_attr_destroy();
  ast_prim_destroy();
  ast_spln_destroy();
  ast_text_destroy();
  ast_trns_destroy();

  /* Destroy the object private data and the object itself. */
  free( (P_Void_ptr)ASTDATA(self) );
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

P_Assist *po_create_assist( P_Renderer *ren )
/* Create a renderer assist object */
{
  P_Assist *self;

  ger_debug("po_create_assist:");

  /* Create the assist object */
  if ( !(self=(P_Assist *)malloc( sizeof(P_Assist) ) ) )
    ger_fatal("po_create_assist: unable to allocate %d bytes!",
	      sizeof(P_Assist) );

  /* Create memory for object data */
  if ( !(self->object_data= (P_Void_ptr)malloc(sizeof(P_Assist_data))) )
    ger_fatal("po_create_assist: unable to allocate %d bytes!",
              sizeof(P_Assist_data) );

  /* Fill in the renderer and some methods */
  RENDERER(self)= ren;
  self->reset_self= reset_self;
  self->destroy_self= destroy_self;

  /* Initialize the object data and fill in the methods */
  ast_attr_init(self);
  ast_prim_init(self);
  ast_spln_init(self);
  ast_text_init(self);
  ast_trns_init(self);

  return(self);
}
