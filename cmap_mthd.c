/****************************************************************************
 * cmap_mthd.c
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
This module provides methods for dealing with color maps.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

/* Struct to hold private data of gob */
typedef struct ren_list_struct {
  P_Renderer *renderer;
  P_Void_ptr data;
  struct ren_list_struct *next;
} P_Ren_List;
#define RENLIST(self) ((P_Ren_List *)(self->object_data))
#define RENDERER(block) (block->renderer)
#define RENDATA(block) (block->data)
#define WALK_RENLIST(map, operation) { \
    P_Ren_List *thisrenlist= RENLIST(map); \
    while (thisrenlist) { \
      METHOD_RDY(RENDERER(thisrenlist)); \
      (*(RENDERER(thisrenlist)->operation))(RENDATA(thisrenlist)); \
      thisrenlist= thisrenlist->next; \
    }}

static void define( P_Renderer *thisrenderer )
/* This method defines the color map on the given renderer */
{
  P_Color_Map *self= (P_Color_Map *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("cmap_mthd: define");

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
    ger_fatal("cmap_mthd: define: unable to allocate %d bytes!",
              sizeof(P_Ren_List) );
  thisrenlist->next= RENLIST(self);
  self->object_data= (P_Void_ptr)thisrenlist;

  RENDERER(thisrenlist)= thisrenderer;
  METHOD_RDY(thisrenderer)
  RENDATA(thisrenlist)= (*(thisrenderer->def_cmap))
    (self->name, self->min, self->max, self->mapfun );

  METHOD_OUT
}

static void install( VOIDLIST )
{
  P_Color_Map *self= (P_Color_Map *)po_this;
  METHOD_IN

  ger_debug("cmap_mthd: install");
  WALK_RENLIST(self,install_cmap);

  METHOD_OUT
}

static void print( VOIDLIST )
/* This is the print method for the gob. */
{
  P_Color_Map *self= (P_Color_Map *)po_this;
  METHOD_IN

  ger_debug("cmap_mthd: print");
  ind_write("Color map <%s>:  min= %f, max= %f",
	    self->name, self->min, self->max ); 
  ind_eol();

  METHOD_OUT
}

static void destroy( VOIDLIST )
/* This is the destroy method for the gob.
 */
{
  P_Color_Map *self= (P_Color_Map *)po_this;
  P_Ren_List *thisrenlist, *lastrenlist;
  METHOD_IN
  ger_debug("cmap_mthd: destroy");

  /* destroy renderer rep of this cmap */
  WALK_RENLIST(self,destroy_cmap);

  /* Clean up local memory */
  thisrenlist= RENLIST(self);
  while (thisrenlist) {
    lastrenlist= thisrenlist;
    thisrenlist= thisrenlist->next;
    free( (P_Void_ptr)lastrenlist );
  }
  free( (P_Void_ptr)self );
  self= NULL;

  METHOD_DESTROYED
}

extern P_Color_Map *po_create_color_map( char * name, double min, double max,
                                        void (*mapfun)( float *,
                                                  float *, float *,
                                                  float *, float * ) )
/* This function returns a color map object. */
{
  P_Color_Map *thismap;
  P_Ren_List *thisrenlist;

  ger_debug("po_create_color_map: name= <%s>",name);

  /* Create memory for color map object */
  if ( !(thismap= (P_Color_Map *)malloc(sizeof(P_Color_Map))) )
    ger_fatal("cmap_mthd: po_create_color_map: unable to allocate %d bytes!",
              sizeof(P_Color_Map) );

  thismap->object_data= (P_Void_ptr)0;
  strncpy(&(thismap->name),name,P3D_NAMELENGTH);
  thismap->name[P3D_NAMELENGTH-1]= '\0';
  thismap->min= min;
  thismap->max= max;
  thismap->mapfun= mapfun;
  thismap->define= define;
  thismap->install= install;
  thismap->print= print;
  thismap->destroy_self= destroy;

  return(thismap);
}
