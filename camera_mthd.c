/****************************************************************************
 * camera_mthd.c
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
This module provides methods for camera objects. 
*/

#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

/* Struct to hold data about renderer rep of camera */
typedef struct ren_list_struct {
  P_Renderer *renderer;
  P_Void_ptr data;
  struct ren_list_struct *next;
} P_Ren_List;

#define RENLIST(camera) ((P_Ren_List *)(camera->object_data))
#define RENDERER(block) (block->renderer)
#define RENDATA(block) (block->data)
#define WALK_RENLIST(camera, operation) { \
    P_Ren_List *thisrenlist= RENLIST(camera); \
    while (thisrenlist) { \
      METHOD_RDY(RENDERER(thisrenlist)); \
      (*(RENDERER(thisrenlist)->operation))(RENDATA(thisrenlist)); \
      thisrenlist= thisrenlist->next; \
    }}

static void destroy( VOIDLIST )
/* This is the destroy method for the camera */
{
  P_Camera *self= (P_Camera *)po_this;
  P_Ren_List *thisrenlist, *lastrenlist;
  METHOD_IN
  ger_debug("camera_mthd: destroy");

  /* destroy renderer rep of this camera */
  WALK_RENLIST(self,destroy_camera);

  /* Clean up local memory */
  thisrenlist= RENLIST(self);
  while (thisrenlist) {
    lastrenlist= thisrenlist;
    thisrenlist= thisrenlist->next;
    free( (P_Void_ptr)lastrenlist );
  }
  free( (P_Void_ptr)self );

  METHOD_DESTROYED
}

static void print( VOIDLIST )
/* This is the print method for the camera */
{
  P_Camera *self= (P_Camera *)po_this;
  METHOD_IN

  ger_debug("camera_mthd: print");
  ind_write("Camera <%s>:",self->name); ind_eol();
  ind_push();
  ind_write("lookfrom: ( %f %f %f )",
            self->lookfrom.x, self->lookfrom.y, self->lookfrom.z); ind_eol();
  ind_write("lookat:   ( %f %f %f )",
            self->lookat.x, self->lookat.y, self->lookat.z); ind_eol();
  ind_write("up:       ( %f %f %f )",
            self->up.x, self->up.y, self->up.z); ind_eol();
  ind_write("fovea= %f, hither= %f, yon= %f",
            self->fovea, self->hither, self->yon); ind_eol();
  ind_write("background color RGBA= ( %f %f %f %f )",
	    self->background.r, self->background.g, self->background.b,
	    self->background.a); ind_eol();
  ind_pop();

  METHOD_OUT
}

static void define( P_Renderer *thisrenderer )
/* This method defines this camera for the given renderer */
{
  P_Camera *self= (P_Camera *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("camera_mthd: define");

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
    ger_fatal("camera_mthd: define: unable to allocate %d bytes!",
              sizeof(P_Ren_List) );
  thisrenlist->next= RENLIST(self);
  self->object_data= (P_Void_ptr)thisrenlist;

  RENDERER(thisrenlist)= thisrenderer;
  METHOD_RDY(thisrenderer)
  RENDATA(thisrenlist)= (*(thisrenderer->def_camera))(self);

  METHOD_OUT
}

static void set( VOIDLIST )
/* This method sets the camera for the renderer for which it was defined. */
{
  P_Camera *self= (P_Camera *)po_this;
  METHOD_IN

  ger_debug("camera_mthd: set");
  WALK_RENLIST(self,set_camera);

  METHOD_OUT
}

static void set_background( P_Color *color )
/* This method sets the camera background color to that given */
{
  P_Camera *self= (P_Camera *)po_this;
  P_Ren_List *thisrenlist;
  METHOD_IN

  ger_debug("camera_mthd: set_background");

  /* Inform the renderers of the change by brutally destroying the
   * old renderer reps of the camera and then creating new ones,
   * this time with the appropriate background color.
   */
  WALK_RENLIST(self,destroy_camera);

  copy_color( &(self->background), color );
  rgbify_color( &(self->background) );

  thisrenlist= RENLIST(self);
  while (thisrenlist) { 
    METHOD_RDY(RENDERER(thisrenlist));
    RENDATA(thisrenlist)= (*(RENDERER(thisrenlist)->def_camera))(self);
    thisrenlist= thisrenlist->next;
  }

  METHOD_OUT
}

P_Camera *po_create_camera( char *name, P_Point *lookfrom, P_Point *lookat, 
                           P_Vector *up, double fovea, double hither, 
                           double yon)
/* This routine creates a camera */
{
  P_Camera *thiscamera;

  ger_debug("po_create_camera: name= <%s>",name);

  /* Create memory for the camera */
  if ( !(thiscamera= (P_Camera *)malloc(sizeof(P_Camera))) )
    ger_fatal("po_create_camera: unable to allocate %d bytes!",
              sizeof(P_Camera) );

  /* Fill in data and methods */
  thiscamera->object_data= (P_Void_ptr)0;
  strncpy( thiscamera->name, name, P3D_NAMELENGTH-1 );
  thiscamera->name[P3D_NAMELENGTH-1]= '\0';
  thiscamera->lookfrom.x= lookfrom->x;
  thiscamera->lookfrom.y= lookfrom->y;
  thiscamera->lookfrom.z= lookfrom->z;
  thiscamera->lookat.x= lookat->x;
  thiscamera->lookat.y= lookat->y;
  thiscamera->lookat.z= lookat->z;
  thiscamera->up.x= up->x;
  thiscamera->up.y= up->y;
  thiscamera->up.z= up->z;
  thiscamera->fovea= fovea;
  thiscamera->hither= hither;
  thiscamera->yon= yon;
  thiscamera->background.ctype= P3D_RGB;
  thiscamera->background.r= thiscamera->background.g= 
    thiscamera->background.b= 0.0;
  thiscamera->background.a= 1.0;
  thiscamera->print= print;
  thiscamera->define= define;
  thiscamera->set= set;
  thiscamera->destroy_self= destroy;
  thiscamera->set_background= set_background;

  return( thiscamera );
}
