/****************************************************************************
 * light_mthd.c
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
This module provides methods for the light primitive gob.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

/* Struct to hold data about renderer rep of gob */
typedef struct ren_list_struct {
  P_Renderer *renderer;
  P_Void_ptr data;
  struct ren_list_struct *next;
} P_Ren_List;

/* Struct to hold private data of gob */
typedef struct ren_data_struct {
  P_Point location;
  P_Color color;
  P_Ren_List *renderer_list;
} P_Ren_Data;

#define DATA(gob) ((P_Ren_Data *)(gob->object_data))
#define RENLIST(gob) (DATA(gob)->renderer_list)
#define RENDERER(block) (block->renderer)
#define RENDATA(block) (block->data)
#define WALK_RENLIST(gob, operation) { \
    P_Ren_List *thisrenlist= RENLIST(gob); \
    while (thisrenlist) { \
      METHOD_RDY(RENDERER(thisrenlist)); \
      (*(RENDERER(thisrenlist)->operation))(RENDATA(thisrenlist)); \
      thisrenlist= thisrenlist->next; \
    }}

static void traverselights( P_Transform *thistrans, P_Attrib_List *thisattr )
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("light_mthd: traverselights");
  thisrenlist= RENLIST(self);
  while (thisrenlist) {
    METHOD_RDY(RENDERER(thisrenlist));
    (*(RENDERER(thisrenlist)->light_traverse_light))
      (RENDATA(thisrenlist),thistrans,thisattr);
    thisrenlist= thisrenlist->next;
  }

  METHOD_OUT
}

static void traverselights_to_ren(P_Renderer *thisrenderer, 
				  P_Transform *thistrans, 
				  P_Attrib_List *thisattr)
/* Traverse lights to the named renderer only */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("light_mthd: traverselights_to_ren");

  /* I hate to do a walk of the renderer list, but I don't
   * see an alternative.
   */
  thisrenlist= RENLIST(self);
  while (thisrenlist) { 
    if ( RENDERER(thisrenlist) == thisrenderer ) {
      METHOD_RDY(RENDERER(thisrenlist)); 
      (*(RENDERER(thisrenlist)->light_traverse_light))
	(RENDATA(thisrenlist),thistrans,thisattr);
      METHOD_OUT
      return;
    }
    else thisrenlist= thisrenlist->next;
  }

  METHOD_OUT
}

static void render( P_Transform *thistrans, P_Attrib_List *thisattr )
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("light_mthd: render");
  thisrenlist= RENLIST(self);
  while (thisrenlist) {
    METHOD_RDY(RENDERER(thisrenlist));
    (*(RENDERER(thisrenlist)->ren_light))
      (RENDATA(thisrenlist),thistrans,thisattr);
    thisrenlist= thisrenlist->next;
  }

  METHOD_OUT
}

static void render_to_ren(P_Renderer *thisrenderer, P_Transform *thistrans, 
			  P_Attrib_List *thisattr)
/* Render to the named renderer only */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("light_mthd: render_to_ren");

  /* I hate to do a walk of the renderer list, but I don't
   * see an alternative.
   */
  thisrenlist= RENLIST(self);
  while (thisrenlist) { 
    if ( RENDERER(thisrenlist) == thisrenderer ) {
      METHOD_RDY(RENDERER(thisrenlist)); 
      (*(RENDERER(thisrenlist)->ren_light))
	(RENDATA(thisrenlist),thistrans,thisattr);
      METHOD_OUT
      return;
    }
    else thisrenlist= thisrenlist->next;
  }

  METHOD_OUT
}

static P_Void_ptr get_ren_data( P_Renderer *thisrenderer )
/* This method returns the data given by the renderer at definition time */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("light_mthd: get_ren_data");

  thisrenlist= RENLIST(self);
  while (thisrenlist) {
    if ( RENDERER(thisrenlist) == thisrenderer ) {
      METHOD_OUT
      return( RENDATA(thisrenlist) );
    }
    thisrenlist= thisrenlist->next;
  }

  /* If not found, not defined for this renderer */
  METHOD_OUT
  return( (P_Void_ptr)0 );
}

static void define_self(P_Renderer *thisrenderer)
/* This method defines the gob within the context of the given renderer */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("light_mthd: define_self");

  /* If already defined, return */
  thisrenlist= RENLIST(self);
  while (thisrenlist) {
    if ( RENDERER(thisrenlist) == thisrenderer ) {
      METHOD_OUT
      return;
    }
    thisrenlist= thisrenlist->next;
  }

  /* Create memory for the renderer data */
  if ( !(thisrenlist= (P_Ren_List *)malloc(sizeof(P_Ren_List))) )
    ger_fatal("light_mthd: define_gob: unable to allocate %d bytes!",
              sizeof(P_Ren_List) );
  thisrenlist->next= RENLIST(self);
  RENLIST(self)= thisrenlist;

  RENDERER(thisrenlist)= thisrenderer;
  METHOD_RDY(thisrenderer)
  RENDATA(thisrenlist)= (*(thisrenderer->def_light))
    (self->name, &(DATA(self)->location), &(DATA(self)->color));

  /* If the gob is held, tell the renderer to hold it as well */
  if (self->held) (*(thisrenderer->hold_gob))(RENDATA(thisrenlist));

  METHOD_OUT
}

static void print( VOIDLIST )
/* This is the print method for the gob. */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Gob_List *child_list;
  METHOD_IN

  ger_debug("light_mthd: print");
  ind_write("Gob <%s>:  light",self->name); ind_eol();
  ind_push();
  ind_write("location: ( %f %f %f )",DATA(self)->location.x,
	    DATA(self)->location.y, DATA(self)->location.z); 
  ind_eol();
  ind_write("color:    ");
  dump_color( &(DATA(self)->color) );
  ind_eol();
  ind_pop();

  METHOD_OUT
}

static void destroy( int destroy_ren_rep )
/* This is the destroy method for the gob. */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_List *thisrenlist, *lastrenlist;
  METHOD_IN

  if ( (self->held) || (self->parents) ) {
    ger_debug("light_mthd: destroy: held or with parents; not destroyed");
    METHOD_OUT
    return;
  }

  ger_debug("light_mthd: destroy");

  /* destroy renderer rep of this gob */
  if (destroy_ren_rep) WALK_RENLIST(self,destroy_light);

  /* Clean up local memory */
  if (self->object_data) {
    thisrenlist= RENLIST(self);
    while (thisrenlist) {
      lastrenlist= thisrenlist;
      thisrenlist= thisrenlist->next;
      free( (P_Void_ptr)lastrenlist );
    }
    free( self->object_data );
  }
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

P_Gob *po_create_light( char *name, P_Point *location, P_Color *color )
/* This function returns a light primitive gob.  Other methods are
 * provided by po_create_primitive. 
 */
{
  P_Gob *thisgob;
  P_Ren_Data *thisrendata;

  ger_debug("po_create_light: name= <%s>",name);

  thisgob= po_create_primitive( name );

  thisgob->destroy_self= destroy;
  thisgob->print= print;
  thisgob->define= define_self;
  thisgob->render= render;
  thisgob->render_to_ren= render_to_ren;
  thisgob->traverselights= traverselights;
  thisgob->traverselights_to_ren= traverselights_to_ren;
  thisgob->get_ren_data= get_ren_data;

  /* Create memory for private data */
  if ( !(thisrendata= (P_Ren_Data *)malloc(sizeof(P_Ren_Data))) )
    ger_fatal("light_mthd: po_create_light: unable to allocate %d bytes!",
              sizeof(P_Ren_Data) );
  thisgob->object_data= (P_Void_ptr)thisrendata;
  RENLIST(thisgob)= (P_Ren_List *)0;
  DATA(thisgob)->location.x= location->x;
  DATA(thisgob)->location.y= location->y;
  DATA(thisgob)->location.z= location->z;
  copy_color( &(DATA(thisgob)->color), color );
  rgbify_color( &(DATA(thisgob)->color) );

  return(thisgob);
}
