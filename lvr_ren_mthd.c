/****************************************************************************
 * lvr_ren_mthd.c
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides a fake renderer that causes frames to be recorded
on the LVR.  To work properly, it must be created *before* the renderer
producing the frames to be recorded.
*/

#include <stdio.h>
#include <stdlib.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"

#define LVR_SETUP_CMD "lvr -S"
#define LVR_RECORD_CMD "lvr -R 1"
#define LVR_SHUTDOWN_CMD "lvr -E"

/* Struct for object data, and access functions for it */
typedef struct renderer_data_struct {
  int open;
  int initialized;
  char *name;
} P_Renderer_data;

#define RENDATA( self ) ((P_Renderer_data *)(self->object_data))
#define NAME( self ) (RENDATA(self)->name)
#define OPEN( self ) (RENDATA(self)->open)
#define INITIALIZED( self ) (RENDATA(self)->initialized)

static void destroy_object(P_Void_ptr the_thing) {
  /*Destroys any object.*/  
  P_Renderer *self = (P_Renderer *)po_this;
  
  METHOD_IN    
  ger_debug("gl_ren_mthd: destroy_object\n");
  if (the_thing) free(the_thing);
  METHOD_OUT
}

static void ren_object(P_Void_ptr the_thing, P_Transform *transform,
		P_Attrib_List *attrs) {
  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: ren_object");
  }
  METHOD_OUT
}

static P_Void_ptr def_sphere(char *name) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
    ger_debug("lvr_ren_mthd: def_sphere");
  METHOD_OUT
  return NULL;
}

static P_Void_ptr def_cylinder(char *name) {
	  
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
    ger_debug("lvr_ren_mthd: def_cylinder");
  METHOD_OUT
  return NULL;
}

static P_Void_ptr def_torus(char *name, double major, double minor) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
    ger_debug("lvr_ren_mthd: def_torus");
  METHOD_OUT
  return NULL;
}

static void ren_gob(P_Void_ptr primdata, P_Transform *thistrans,
		P_Attrib_List *thisattrlist) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  if (OPEN(self)) {
    ger_debug("lvr_ren_mthd: ren_gob");
    /* transform non-null implies this is top level call */
    if (thistrans) {
      if (system(LVR_RECORD_CMD)==-1) perror("lvr_ren_mthd: ren_gob");
    }
  }
  METHOD_OUT
}

static void traverse_gob( P_Void_ptr primdata, P_Transform *thistrans,
                    P_Attrib_List *thisattrlist )
/* This routine traverses a gob looking for lights. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  if (OPEN(self)) {
    ger_debug("lvr_ren_mthd: traverse_gob");
  }
  METHOD_OUT
}

static void destroy_gob( P_Void_ptr primdata )
/* This routine destroys the renderer's data for the gob. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: destroy_gob");
    if (primdata) free(primdata);
  }
  METHOD_OUT
}

static P_Void_ptr def_polything(char *name, P_Vlist *vertices) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: def_polything");
  }
  METHOD_OUT
  return NULL;
}

static P_Void_ptr def_mesh(char *name, P_Vlist *vertices, int *indices,
			   int *facet_lengths, int nfacets) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: def_mesh");
  }
  METHOD_OUT
  return NULL;
}

static P_Void_ptr def_light(char *name, P_Point *point, P_Color *color) {    
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: def_light");
  }
  METHOD_OUT
  return NULL;
}

static P_Void_ptr def_ambient(char *name, P_Color *color) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: def_ambient");
  }
  METHOD_OUT
  return NULL;
}    

static void hold_gob(P_Void_ptr gob) {
  /*This doesn't do anything*/

  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: hold_gob\n");
  }

  METHOD_OUT
}

static void unhold_gob(P_Void_ptr gob) {
  /*This doesn't do anything*/

  P_Renderer *self = (P_Renderer *)po_this;
  METHOD_IN
	
  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: unhold_gob\n");
  }

  METHOD_OUT
}

static P_Void_ptr def_camera(struct P_Camera_struct *thecamera) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: def_camera");
  }
  METHOD_OUT
  return NULL;
}

static void set_camera(P_Void_ptr thecamera) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: set_camera");
  }
  METHOD_OUT
}    
    
static void destroy_camera(P_Void_ptr thecamera) {
  /* This method must not refer to "self", because the renderer for which
   * the camera was defined may already have been destroyed.
   */
  if (thecamera) {
    ger_debug("lvr_ren_mthd: set_camera");
    if (thecamera) free(thecamera);
  }
}    
    
static P_Void_ptr def_text(char *name, char *tstring, P_Point *location,
		    P_Vector *u, P_Vector *v) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("lvr_ren_mthd: def_text");
  }
  METHOD_OUT
  return NULL;
}

static void ren_print() {
  P_Renderer *self=(P_Renderer *)po_this;
  METHOD_IN
  ger_debug("lvr_ren_mthd: ren_print");
  ind_write("RENDERER: LVR controller '%s', %s",
	    self->name, 
	    OPEN(self) ? "open" : "closed");
  ind_eol();
  METHOD_OUT
}

static void ren_open() {
  /*opening the renderer.*/
  
  P_Renderer *self=(P_Renderer *)po_this;
  
  METHOD_IN

  ger_debug("lvr_ren_mthd: ren_open\n");
  RENDATA(self)->open=1;
    
  METHOD_OUT
}

static void ren_close() {
    P_Renderer *self= (P_Renderer *)po_this;
    METHOD_IN
    ger_debug("lvr_ren_mthd: ren_close");
    RENDATA(self)->open= 0;
    METHOD_OUT
}

static void ren_destroy() {
    /*Kill everything. Everything.*/
    P_Renderer *self=(P_Renderer *)po_this;
    METHOD_IN

    ger_debug("gl_ren_mthd: ren_destroy\n");

    if (system(LVR_SHUTDOWN_CMD)==-1) perror("lvr_ren_mthd: ren_destroy");

    free ((void *)self);
    METHOD_DESTROYED
}

static P_Void_ptr def_cmap(char *name, double min, double max, void(*mapfun)(float *, float *, float *, float *, float *)) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (OPEN(self)) {
    ger_debug("lvr_ren_mthd: def_cmap");
  }

  METHOD_OUT
  return NULL;
}

static void install_cmap(P_Void_ptr mapdata) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (OPEN(self)) {
    ger_debug("lvr_ren_mthd: install_cmap");
  }

  METHOD_OUT
}

static void destroy_cmap(P_Void_ptr mapdata) {
  /* This method must not refer to "self", because the renderer which
   * created this cmap may already have been destroyed.
   */
  ger_debug("lvr_ren_mthd: destroy_cmap");
  if (mapdata) free(mapdata);
}

static P_Void_ptr def_gob(char *name, struct P_Gob_struct *gob) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
    ger_debug("lvr_ren_mthd: def_gob");
  METHOD_OUT
  return NULL;
}

P_Renderer *po_create_lvr_renderer( char *device, char *datastr )
/* This routine creates a ptr-generating renderer object */
{
  P_Renderer *self;
  int length,i;
  P_Renderer_data *rdata;
  long xsize, ysize;
  register short lupe;
  register char *where;
  int x,y,result; /*For prefposition*/
  unsigned int width, height; /*For prefsize*/
  static int sequence_number;
  
  ger_debug("po_create_lvr_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_lvr_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  if ( !(self->object_data= 
	 (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_lvr_renderer: unable to allocate %d bytes!",
	      sizeof(P_Renderer_data));
  NAME(self) = "p3d-lvr";
  sprintf(self->name,"lvr%d",sequence_number);
  OPEN(self)= 0;

  if (system(LVR_SETUP_CMD)==-1) perror("po_create_lvr_renderer");
  INITIALIZED(self)= 1;

  /* Fill in all the methods */
  self->def_sphere= def_sphere;
  self->ren_sphere= ren_object;
  self->destroy_sphere= destroy_object;
  
  self->def_cylinder= def_cylinder;
  self->ren_cylinder= ren_object;
  self->destroy_cylinder= destroy_object;
  
  self->def_torus= def_torus;
  self->ren_torus= ren_object;
  self->destroy_torus= destroy_object;
  
  self->def_polymarker= def_polything;
  self->ren_polymarker= ren_object;
  self->destroy_polymarker= destroy_object;
  
  self->def_polyline= def_polything;
  self->ren_polyline= ren_object;
  self->destroy_polyline= destroy_object;
  
  self->def_polygon= def_polything;
  self->ren_polygon= ren_object;
  self->destroy_polygon= destroy_object;
  
  self->def_tristrip= def_polything;
  self->ren_tristrip= ren_object;
  self->destroy_tristrip= destroy_object;
  
  self->def_bezier= def_polything;
  self->ren_bezier= ren_object;
  self->destroy_bezier= destroy_object;
  
  self->def_mesh= def_mesh;
  self->ren_mesh= ren_object;
  self->destroy_mesh= destroy_object;
  
  self->def_text= def_text;
  self->ren_text= ren_object;
  self->destroy_text= destroy_object;
  
  self->def_light= def_light;
  self->ren_light= ren_object;
  self->light_traverse_light= traverse_gob;
  self->destroy_light= destroy_object;
  
  self->def_ambient= def_ambient;
  self->ren_ambient= ren_object;
  self->light_traverse_ambient= traverse_gob;
  self->destroy_ambient= destroy_object;
  
  self->def_gob= def_gob;
  self->ren_gob= ren_gob;
  self->light_traverse_gob= traverse_gob;
  self->hold_gob= hold_gob;
  self->unhold_gob= unhold_gob;
  self->destroy_gob= destroy_gob;
  
  self->print= ren_print;
  self->open= ren_open;
  self->close= ren_close;
  self->destroy_self= ren_destroy;
  
  self->def_camera= def_camera;
  self->set_camera= set_camera;
  self->destroy_camera= destroy_camera;
  
  self->def_cmap= def_cmap;
  self->install_cmap= install_cmap;
  self->destroy_cmap= destroy_cmap;
  
  return self;
}

