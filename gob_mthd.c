/****************************************************************************
 * gob_mthd.c
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
This module provides methods for the root class gob.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

/* Struct to hold data about renderer rep of gob */
typedef struct ren_data_struct {
  P_Renderer *renderer;
  P_Void_ptr data;
  struct ren_data_struct *next;
} P_Ren_Data;
#define RENDERER(block) (block->renderer)
#define RENDATA(block) (block->data)
#define RENLIST(gob) ((P_Ren_Data *)(self->object_data))
#define WALK_RENLIST(gob, operation) { \
    P_Ren_Data *thisrendata= RENLIST(gob); \
    while (thisrendata) { \
      METHOD_RDY(RENDERER(thisrendata)); \
      (*(RENDERER(thisrendata)->operation))(RENDATA(thisrendata)); \
      thisrendata= thisrendata->next; \
    }}

static P_Gob_List *add_gob_cell( P_Gob_List *oldfirst )
/* This routine adds a cell to a gob list */
{
  P_Gob_List *thiscell;

  ger_debug("gob_mthd: add_gob_cell");

  if ( !(thiscell= (P_Gob_List *)malloc(sizeof(P_Gob_List))) )
    ger_fatal("gob_mthd: add_gob_cell: unable to allocate %d bytes!\n",
              sizeof(P_Gob_List));

  thiscell->next= oldfirst;
  thiscell->prev= (P_Gob_List *)0;
  if (oldfirst) oldfirst->prev= thiscell;

  return(thiscell);
}

static void add_transform( P_Transform *thistrans )
/* This method premultiplies a tranform onto the gob's transform. */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Transform_type *tmp;
  METHOD_IN
    
  ger_debug("gob_mthd: add_transform");

  if (self->has_transform) {
    premult_trans( thistrans, &(self->trans) );
    tmp = duplicate_trans_type(thistrans->type_front);
    self->trans.type_front = add_type_to_list(tmp,self->trans.type_front);

  } else { /* This is  the first transform. */
    copy_trans( &(self->trans), thistrans );
    self->trans.type_front = duplicate_trans_type(thistrans->type_front);
    self->has_transform= 1;
  }

  METHOD_OUT
}

static void refuse_transform( P_Transform *thistrans )
/* This this is the transform-adding method for primitive gobs, which cannot
 * have transforms associated with them.
 */
{
  P_Gob *self= (P_Gob *)po_this;
  METHOD_IN
    
  ger_debug("gob_mthd: refuse_transform");
  
  ger_error("gob_mthd: refuse_transform: \n\
Attempted to apply a transform to a primitive gob, which can't have any.");

  METHOD_OUT
}

static void add_child( P_Gob *thischild )
/* This method adds a gob to the gob's child list.  Notice that it does
 * not make a new copy of the added gob.
 */
{
  P_Gob *self= (P_Gob *)po_this;
  METHOD_IN

  ger_debug("gob_mthd: add_child");
  
  self->children= add_gob_cell( self->children );
  self->children->gob= thischild;
  METHOD_RDY(thischild);
  thischild->parents += 1;

  METHOD_OUT
}

static void refuse_child( P_Gob *thischild )
/* This is the child-adding method for primitive gobs, which cannot
 * have children.
 */
{
  P_Gob *self= (P_Gob *)po_this;
  METHOD_IN

  ger_debug("gob_mthd: refuse_child");

  ger_error("gob_mthd: refuse_child: \n\
Attempted to add a child gob to a primitive gob, which can't have any.");

  METHOD_OUT
}

static void add_attribute( char *attribute, int type, P_Void_ptr value )
/* This method adds an attribute-value pair to the gob's attribute list.
 * Note that it does not make a new copy of the attribute or value info.
 */
{
  P_Gob *self= (P_Gob *)po_this;
  METHOD_IN

  ger_debug("gob_mthd: add_attribute");
  
  self->attr= add_attr( self->attr, attribute, type, value );

  METHOD_OUT
}

static void refuse_attribute( char *attribute, int type, P_Void_ptr value )
/* This is the attribute-adding method for primitive gobs, which
 * cannot have attributes.
 */
{
  P_Gob *self= (P_Gob *)po_this;
  METHOD_IN

  ger_debug("gob_mthd: refuse_attribute");

  ger_error(
"gob_mthd: refuse_attr:  \n\
Attempted to add an attribute to a primitive gob, which can't have any.");

  METHOD_OUT
}

static void traverselights( P_Transform *thistrans, P_Attrib_List *thisattr )
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_Data *thisrendata;
  METHOD_IN

  ger_debug("gob_mthd: traverselights");
  
  thisrendata= RENLIST(self);
  while (thisrendata) {
    METHOD_RDY(RENDERER(thisrendata));
    (*(RENDERER(thisrendata)->light_traverse_gob))
      (RENDATA(thisrendata), thistrans, thisattr);
    thisrendata= thisrendata->next;
  }

  METHOD_OUT
}

static void traverselights_to_ren( P_Renderer *ren, P_Transform *thistrans, 
				  P_Attrib_List *thisattr)
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_Data *thisrendata;
  METHOD_IN

  ger_debug("gob_mthd: traverselights_to_ren");
  
  /* I hate to do a walk of the renderer list, but I don't
   * see an alternative.
   */
  thisrendata= RENLIST(self);
  while (thisrendata) { 
    if ( RENDERER(thisrendata) == ren ) {
      METHOD_RDY(RENDERER(thisrendata)); 
      (*(RENDERER(thisrendata)->light_traverse_gob))
	(RENDATA(thisrendata), thistrans, thisattr);
      METHOD_OUT
      return;
    }
    else thisrendata= thisrendata->next;
  }

  METHOD_OUT
}

static void render( P_Transform *thistrans, P_Attrib_List *thisattr )
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_Data *thisrendata;
  METHOD_IN

  ger_debug("gob_mthd: render");

  thisrendata= RENLIST(self);
  while (thisrendata) {
    METHOD_RDY(RENDERER(thisrendata));
    (*(RENDERER(thisrendata)->ren_gob))
      (RENDATA(thisrendata), thistrans, thisattr);
    thisrendata= thisrendata->next;
  }

  METHOD_OUT
}

static void render_to_ren(P_Renderer *thisrenderer, P_Transform *thistrans, 
			  P_Attrib_List *thisattr)
/* Render to the named renderer only */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_Data *thisrendata;
  METHOD_IN

  ger_debug("gob_mthd: render_to_ren");

  /* I hate to do a walk of the renderer list, but I don't
   * see an alternative.
   */
  thisrendata= RENLIST(self);
  while (thisrendata) { 
    if ( RENDERER(thisrendata) == thisrenderer ) {
      METHOD_RDY(RENDERER(thisrendata)); 
      (*(RENDERER(thisrendata)->ren_gob))
	(RENDATA(thisrendata), thistrans, thisattr);
      METHOD_OUT
      return;
    }
    else thisrendata= thisrendata->next;
  }

  METHOD_OUT
}

static void define_gob(P_Renderer *thisrenderer)
/* This routine defines the gob within the context of the given renderer */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_Data *thisrendata;
  METHOD_IN

  ger_debug("gob_mthd: define_gob");

  /* If already defined, return */
  thisrendata= RENLIST(self);
  while (thisrendata) {
    if ( RENDERER(thisrendata) == thisrenderer ) {
      METHOD_OUT
	return;
    }
    thisrendata= thisrendata->next;
  }

  /* Create memory for the renderer data */
  if ( !(thisrendata= (P_Ren_Data *)malloc(sizeof(P_Ren_Data))) )
    ger_fatal("gob_mthd: define_gob: unable to allocate %d bytes!",
              sizeof(P_Ren_Data) );
  thisrendata->next= RENLIST(self);
  self->object_data= (P_Void_ptr)thisrendata;

  METHOD_RDY(thisrenderer)
  RENDERER(thisrendata)= thisrenderer;
  RENDATA(thisrendata)= (*(thisrenderer->def_gob))(self->name,self);

  /* If the gob is held, tell the renderer to hold it as well */
  if (self->held) (*(thisrenderer->hold_gob))(RENDATA(thisrendata));

  METHOD_OUT
}

static P_Void_ptr get_ren_data(P_Renderer *thisrenderer)
/* This method returns the data given by the renderer at definition time */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_Data *thisrendata;
  METHOD_IN

  ger_debug("gob_mthd: get_ren_data");

  thisrendata= RENLIST(self);
  while (thisrendata) {
    if ( RENDERER(thisrendata) == thisrenderer ) {
      METHOD_OUT
      return( RENDATA(thisrendata) );
    }
    thisrendata= thisrendata->next;
  }

  /* If not found, not defined for this renderer */
  METHOD_OUT
  return( (P_Void_ptr)0 );
}

static void print_gob( VOIDLIST )
/* This is the print method for the gob. */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Gob_List *child_list;
  METHOD_IN

  ger_debug("gob_mthd: print_gob");
  ind_write("Gob <%s>:",self->name); 
  if (self->held) ind_write(" %d parents", self->parents);
  ind_eol();
  ind_push();
  if (self->has_transform) {
    ind_write("Transformation:"); ind_eol();
    dump_trans(&(self->trans));
  }
  if (self->attr) {
    ind_write("Attributes:"); ind_eol();
    print_attr(self->attr);
  }
  if (self->children) {
    ind_write("Children:"); ind_eol();
    ind_push();
    child_list= self->children;
    while (child_list) {
      METHOD_RDY(child_list->gob)
      (*(child_list->gob->print))();
      child_list= child_list->next;
    }
    ind_pop();
  }
  ind_pop();

  METHOD_OUT
}

static void hold( VOIDLIST )
/* This method marks a gob as being held, so that it cannot be destroyed */
{
  P_Gob *self= (P_Gob *)po_this;
  METHOD_IN

  ger_debug("gob_mthd: hold");
  self->held= 1;
  WALK_RENLIST(self,hold_gob);

  METHOD_OUT
}

static void unhold( VOIDLIST )
/* This method marks a gob as not being held, so that it can be destroyed */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Ren_Data *thisrendata;
  METHOD_IN

  ger_debug("gob_mthd: unhold");
  self->held= 0;
  thisrendata= RENLIST(self);
  while (thisrendata) {
    METHOD_RDY(RENDERER(thisrendata));
    (*(RENDERER(thisrendata)->unhold_gob))(RENDATA(thisrendata));
    thisrendata= thisrendata->next;
  }

  METHOD_OUT
}

static void destroy( int destroy_ren_rep )
/* This is the destroy method for the gob.  If the gob is not held and
 * has no parents, it will be destroyed.  Likewise, any of its children
 * which are not held and have no parents are also destroyed, and so
 * on recursively.
 */
{
  P_Gob *self= (P_Gob *)po_this;
  P_Gob *kidgob;
  P_Gob_List *kids, *nextkids;
  P_Ren_Data *thisrendata, *lastrendata;
  METHOD_IN

  if ( (self->held) || (self->parents) ) {
    ger_debug("gob_mthd: destroy: held or with parents; not destroyed");
    METHOD_OUT
    return;
  }

  ger_debug("gob_mthd: destroy");

  /* Clean up attribute list */
  if (self->attr) destroy_attr( self->attr );
  
  /* Clean up child list, destoying child gobs if appropriate */
  kids= self->children;
  while (kids) {
    nextkids= kids->next;
    kidgob= kids->gob;
    METHOD_RDY(kidgob);
    kidgob->parents -= 1;
    (*(kidgob->destroy_self))(destroy_ren_rep);  
                  /* does nothing if held or has parents */
    free( (P_Void_ptr)kids );
    kids= nextkids;
  }

  /* destroy renderer rep of this gob */
  if (destroy_ren_rep) WALK_RENLIST(self,destroy_gob);
  
  /* Clean up local memory */
  thisrendata= RENLIST(self);
  while (thisrendata) {
    lastrendata= thisrendata;
    thisrendata= thisrendata->next;
    free( (P_Void_ptr)lastrendata );
  }
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

P_Gob *po_create_gob( char *name )
/* This function returns a gob of the sort which forms a non-terminal
 * DAG node.  It is created without children, however.
 */
{
  P_Gob *thisgob;

  ger_debug("po_create_gob: name= <%s>", name);

  /* Create memory for the gob */
  if ( !(thisgob= (P_Gob *)malloc(sizeof(P_Gob))) )
    ger_fatal("po_create_gob: unable to allocate %d bytes!",
              sizeof(P_Gob) );

  /* Fill out object data */
  strncpy( thisgob->name, name, P3D_NAMELENGTH-1 );
  thisgob->name[P3D_NAMELENGTH-1]= '\0';
  thisgob->held= 0;
  thisgob->parents= 0;
  thisgob->children= (P_Gob_List *)0;
  thisgob->attr= (P_Attrib_List *)0;
  thisgob->has_transform= 0;
  copy_trans( &(thisgob->trans), Identity_trans );
  thisgob->object_data= (P_Void_ptr)0;
  thisgob->hold= hold;
  thisgob->unhold= unhold;
  thisgob->destroy_self= destroy;
  thisgob->print= print_gob;
  thisgob->define= define_gob;
  thisgob->render= render;
  thisgob->render_to_ren= render_to_ren;
  thisgob->traverselights= traverselights;
  thisgob->traverselights_to_ren= traverselights_to_ren;
  thisgob->add_attribute= add_attribute;
  thisgob->add_child= add_child;
  thisgob->add_transform= add_transform;
  thisgob->get_ren_data= get_ren_data;

  return(thisgob);
}

P_Gob *po_create_primitive( char *name )
/* This function returns a gob of the sort which forms a terminal (leaf)
 * DAG node.  It is created without children, attributes, or transformation,
 * and has no method to add them.
 */
{
  P_Gob *thisgob;

  ger_debug("po_create_primitive: name= <%s>", name);

  /* Create memory for the gob */
  if ( !(thisgob= (P_Gob *)malloc(sizeof(P_Gob))) )
    ger_fatal("po_create_gob: unable to allocate %d bytes!",
              sizeof(P_Gob) );

  /* Fill out object data */
  strncpy( thisgob->name, name, P3D_NAMELENGTH-1 );
  thisgob->name[P3D_NAMELENGTH-1]= '\0';
  thisgob->held= 0;
  thisgob->parents= 0;
  thisgob->children= (P_Gob_List *)0;
  thisgob->attr= (P_Attrib_List *)0;
  thisgob->has_transform= 0;
  copy_trans( &(thisgob->trans), Identity_trans );
  thisgob->object_data= (P_Void_ptr)0;
  thisgob->add_attribute= refuse_attribute;
  thisgob->add_child= refuse_child;
  thisgob->add_transform= refuse_transform;
  thisgob->hold= hold;
  thisgob->unhold= unhold;
  thisgob->destroy_self= (void (*)(int))null_method;
  thisgob->print= null_method;
  thisgob->define= (void (*)(P_Renderer *))null_method;
  thisgob->render= (void (*)(P_Transform *, P_Attrib_List *))null_method;
  thisgob->render_to_ren= 
    (void (*)(P_Renderer *,P_Transform *, P_Attrib_List *))null_method;
  thisgob->traverselights= 
    (void (*)(P_Transform *, P_Attrib_List *))null_method;
  thisgob->traverselights_to_ren=
    (void (*)(P_Renderer *,P_Transform *, P_Attrib_List *))null_method;
  thisgob->get_ren_data= (P_Void_ptr (*)(P_Renderer *))null_method;

  return(thisgob);
}


