/****************************************************************************
 * p3dgen.c
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
This module provides the core routines for p3dgen.  It specifies the
behavior of p3dgen by imposing a set of rules on the object structure.
*/

/* Notes-
 */

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "std_cmap.h"
#include "ge_error.h"
#include "indent.h"

/* Hash table sizes */
#define GOB_HASH_SIZE 1009
#define CAMERA_HASH_SIZE 37

/* Pointer on which to hang current object, so methods can access their
 * object data.
 */
P_Void_ptr po_this;

/* Table of possible renderer generators */
typedef struct ren_table_struct {
  char *name;
  P_Renderer *(*generator)(char *, char *);
} ren_table_cell;

static ren_table_cell renderer_table[]= {
  {"p3d", po_create_p3d_renderer},
  {"painter", po_create_painter_renderer},
  {"xpainter", po_create_xpainter_renderer},
  {"gl", po_create_gl_renderer},
  {"pvm", po_create_pvm_renderer},
  {"iv", po_create_iv_renderer},
  {"vrml", po_create_vrml_renderer},
  {"lvr", po_create_lvr_renderer},
  {(char *)0, NULL}         /* add new renderers before this line */
};

/* Pointer on which to hang the list of initialized renderers. */
P_Ren_List_Cell *pg_renderer_list= (P_Ren_List_Cell *)0;

/* Default attribute list */
P_Attrib_List *po_default_attributes;

/* Gob hash table, and currently open gob, and name of current named gob */
static P_String_Hash *gob_hash;
static P_Gob_List *cur_gob= (P_Gob_List *)0;
static char cur_named_gob_name[P3D_NAMELENGTH];

/* Camera hash table */
static P_String_Hash *camera_hash;

/* Current color map */
static P_Color_Map *cur_cmap= (P_Color_Map *)0;

/* Information for standard camera and lights */
static P_Point std_cam_lookfrom= { 0.0, 0.0, 20.0 };
static P_Point std_cam_lookat= { 0.0, 0.0, 0.0 };
static P_Vector std_cam_up= { 0.0, 1.0, 0.0 };
#define STD_CAM_FOVEA 53.0
#define STD_CAM_HITHER -5.0
#define STD_CAM_YON -35.0
static P_Point std_light_loc= {0.0, 2.0, 20.0};
static P_Color std_light_color= { P3D_RGB, 0.7, 0.7, 0.7, 1.0 };
static P_Color std_ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 1.0 };

/* Initialization state, and macro to check initialization */
static int initialized= 0;
#define INIT_CHECK \
if (!initialized) { \
  ger_error("p3dgen: init_check: Not initialized; call pg_init_ren first!"); \
  return(P3D_FAILURE); \
} \

int pg_initialize( VOIDLIST )
/* This routine initializes various hash tables and symbols */
{
  ger_debug("p3dgen: initialize");

  if (!initialized) {
    ger_init("p3dgen"); /* harmless if repeated */
    ind_setup(); /* harmless if repeated */
    gob_hash= po_create_chash(GOB_HASH_SIZE);
    camera_hash= po_create_chash(CAMERA_HASH_SIZE);
    po_default_attributes= po_gen_default_attr();
    
    initialized= 1;
  }
  return( P3D_SUCCESS );
}

int pg_init_ren( char *name, char *renderer, char *device, char *datastr )
/* This routine initializes the system, opening a renderer. */
{
  P_Ren_List_Cell *thiscell;
  ren_table_cell *thisgenerator;

  ger_debug(
"p3dgen: pg_init_ren: name= <%s>, renderer= <%s>, device= <%s>, datastr= <%s>",
     name, renderer, device, datastr);

  pg_initialize();

  /* Check to see if the same name has been used to open a previous
   * renderer.
   */
  thiscell= pg_renderer_list;
  while (thiscell) {
    if ( !strncmp(name, thiscell->name, P3D_NAMELENGTH) ) {
      ger_error("p3dgen: pg_init_ren: \n\
Renderer name <%s> used twice; second use ignored.",name);
      return( P3D_FAILURE );
    }
    thiscell= thiscell->next;
  }

  /* Create the new renderer and add it to the list. 
   * This involves searching the table of renderers for a match.
   */
  if ( !(thiscell= (P_Ren_List_Cell *)malloc( sizeof(P_Ren_List_Cell) ) ) )
    ger_fatal("p3dgen: pg_init_ren: unable to allocate %d bytes!",
	      sizeof( P_Ren_List_Cell ) );
  thiscell->next= pg_renderer_list;
  thiscell->prev= (P_Ren_List_Cell *)0;
  if (pg_renderer_list) pg_renderer_list->prev= thiscell;
  pg_renderer_list= thiscell;
  strncpy(thiscell->name, name, P3D_NAMELENGTH);
  thisgenerator= renderer_table;
  while (thisgenerator->name) {
    if ( !strcmp(renderer,thisgenerator->name) ) {
      thiscell->renderer= (*(thisgenerator->generator))(device, datastr);
      break;
    }
    thisgenerator++;
  }

  if ( !(thisgenerator->name) ) { /* failed to find a match */
    ger_error("p3dgen: pg_init_ren: \n\
Sorry, unsupported renderer type \"%s\" requested; using \"%s\" instead.",
	      renderer, renderer_table[0].name);
    thiscell->renderer= (*(renderer_table[0].generator))(device, datastr);
  }

  /* If a valid renderer was not produced, clip it from the renderer list
   * and return failure.
   */
  if (!(thiscell->renderer)) {
    ger_error("p3dgen: pg_init_ren: renderer \"%s\" could not be created!",
	      name);
    pg_renderer_list= thiscell->next;
    free( (P_Void_ptr)thiscell );
    pg_renderer_list->prev= (P_Ren_List_Cell *)0;
    return( P3D_FAILURE );
  }

  /* Open the renderer */
  if ( pg_open_ren( name ) != P3D_SUCCESS ) {
    ger_error("p3dgen: pg_init_ren: unable to open new renderer!");
    return( P3D_FAILURE );
  }

  /* Create default color map, standard camera, and standard lights.
   * If this is not the first renderer, the old defaults will be
   * deleted from the earlier renderers and new ones will be
   * defined for all renderers.
   */
  (void)pg_camera( "standard_camera", &std_cam_lookfrom, &std_cam_lookat,
	    &std_cam_up, STD_CAM_FOVEA, STD_CAM_HITHER, STD_CAM_YON );
  (void)pg_open("standard_lights");
  (void)pg_light( &std_light_loc, &std_light_color );
  (void)pg_ambient( &std_ambient_color );
  (void)pg_close();
  if (!cur_cmap) return( pg_std_cmap(0.0, 1.0, 0) );
  else return( P3D_SUCCESS );
}

int pg_open_ren( char *renderer )
/* This routine opens the current renderer */
{
  P_Ren_List_Cell *thiscell;

  ger_debug("p3dgen: pg_open_ren: opening renderer <%s>",renderer);

  INIT_CHECK;

  thiscell= pg_renderer_list;
  while (thiscell) {
    if ( !strncmp(renderer, thiscell->name, P3D_NAMELENGTH) ) {
      METHOD_RDY(thiscell->renderer);
      (*(thiscell->renderer->open))();
      return( P3D_SUCCESS );
    }
    thiscell= thiscell->next;
  }

  /* If we made it to here, it's not a known renderer. */
  ger_error("p3dgen: pg_open_ren: renderer <%s> not initialized",renderer);
  return( P3D_FAILURE );
}

int pg_close_ren( char *renderer )
/* This routine closes the given renderer */
{
  P_Ren_List_Cell *thiscell;

  ger_debug("p3dgen: pg_close_ren: closing renderer <%s>",renderer);

  INIT_CHECK;

  thiscell= pg_renderer_list;
  while (thiscell) {
    if ( !strncmp(renderer, thiscell->name, P3D_NAMELENGTH) ) {
      METHOD_RDY(thiscell->renderer);
      (*(thiscell->renderer->close))();
      return( P3D_SUCCESS );
    }
    thiscell= thiscell->next;
  }

  /* If we made it to here, it's not a known renderer. */
  ger_error("p3dgen: pg_close_ren: renderer <%s> not initialized",renderer);
  return( P3D_FAILURE );
}

int pg_shutdown_ren( char *renderer )
/* This routine closes the given renderer */
{
  P_Ren_List_Cell *thiscell;

  ger_debug("p3dgen: pg_shutdown: shutting down renderer <%s>",renderer);

  INIT_CHECK;

  /* Find the renderer, destroy it, and cut it out of the renderer list. */
  thiscell= pg_renderer_list;
  while (thiscell) {
    if ( !strncmp(renderer, thiscell->name, P3D_NAMELENGTH) ) {
      METHOD_RDY(thiscell->renderer);
      (*(thiscell->renderer->destroy_self))();
      if (thiscell->prev) {
	thiscell->prev->next= thiscell->next;
	if (thiscell->next) thiscell->next->prev= thiscell->prev;
      }
      else {
	pg_renderer_list= thiscell->next;
	if (thiscell->next) thiscell->next->prev= (P_Ren_List_Cell *)0;
      }
      free( (P_Void_ptr)thiscell );
      return( P3D_SUCCESS );
    }
    thiscell= thiscell->next;
  }

  /* If we made it to here, it's not a known renderer. */
  ger_error("p3dgen: pg_shutdown_ren: renderer <%s> not initialized",renderer);
  return( P3D_FAILURE );
}

int pg_shutdown()
/* This routine completely shuts down P3DGen, closing and shutting down
 * all initialized renderers.
 */
{
  P_Ren_List_Cell *thiscell;
  P_Camera* thiscam;
  P_Gob* thisgob;

  ger_debug("p3dgen: pg_shutdown");

  if (!initialized) /* never started up */
    return P3D_SUCCESS;

  /* Free everything in the gob and camera hash tables.  If this isn't 
   * done, problems result if the user tries to restart P3DGen.
   * We must first open all the renderers and unhold all the gobs.
   */
  METHOD_RDY(camera_hash);
  (*(camera_hash->walk_start))();
  thiscam= (P_Camera*)(*(camera_hash->walk))();
  while (thiscam) {
    METHOD_RDY(thiscam);
    (*(thiscam->destroy_self))();
    METHOD_RDY(camera_hash);
    thiscam= (P_Camera*)(*(camera_hash->walk))();
  }
  METHOD_RDY(gob_hash);
  (*(gob_hash->walk_start))();
  thisgob= (P_Gob*)(*(gob_hash->walk))();
  while (thisgob) {
    METHOD_RDY(thisgob);
    (*(thisgob->unhold))();
    (*(thisgob->destroy_self))(1);
    METHOD_RDY(gob_hash);
    thisgob= (P_Gob*)(*(gob_hash->walk))();
  }

  /* Shut down all initialized renderers.  pg_shutdown_ren keeps updating
   * the renderer list, so we don't have to.
   */
  thiscell= pg_renderer_list;
  while (thiscell) {
    pg_shutdown_ren( thiscell->name );
    thiscell= pg_renderer_list;
  }
  pg_renderer_list= (P_Ren_List_Cell *)0;

  /* Destroy the default attribute list */
  destroy_attr( po_default_attributes );
  po_default_attributes= (P_Attrib_List *)0;

  /* Free the gob and camera hash tables themselves */
  METHOD_RDY(camera_hash);
  (*(camera_hash->destroy_self))();
  camera_hash= (P_String_Hash *)0;
  METHOD_RDY(gob_hash);
  (*(gob_hash->destroy_self))();
  gob_hash= (P_String_Hash *)0;

  /* Destroy the current cmap */
  METHOD_RDY(cur_cmap);
  (*(cur_cmap->destroy_self))();
  cur_cmap= (P_Color_Map *)0;

  /* Mark state uninitialized */
  initialized= 0;

  return( P3D_SUCCESS );
}

int pg_print_ren( char *renderer )
/* This routine dumps a description of the given renderer */
{
  P_Ren_List_Cell *thiscell;

  ger_debug("p3dgen: pg_print_ren");

  INIT_CHECK;

  thiscell= pg_renderer_list;
  while (thiscell) {
    if ( !strncmp(renderer, thiscell->name, P3D_NAMELENGTH) ) {
      METHOD_RDY(thiscell->renderer);
      (*(thiscell->renderer->print))();
      return( P3D_SUCCESS );
    }
    thiscell= thiscell->next;
  }

  /* If we made it to here, it's not a known renderer. */
  ger_error("p3dgen: pg_print_ren: renderer <%s> not initialized",renderer);
  return( P3D_FAILURE );
}

int pg_open( char *gobname )
/* This routine creates a new open gob.  It can be used in two ways.
 *
 * If the parameter string is valid and points to a non-empty string,
 * a named gob is created.  The gob is immediately 'held', which 
 * means that it's destroy method won't do anything until its unhold 
 * method has been activated.  If an old gob of the same name exists,
 * it is unheld and destroyed (unless it has parents) before the new 
 * gob is created.  There can only be one open named gob at a time.
 *
 * On the other hand, if the parameter string is null or empty or a
 * single space (" "), an unnamed gob is created.  This can only occur 
 * within the context of already having an open named gob;  unnamed 
 * gobs can be nested arbitrarily deeply.  Unnamed gobs are not held;  
 * they get destroyed as soon as their named ancestor is destroyed.
 */
{
  P_Gob_List *newcell;
  P_Gob *oldgob;

  if (!gobname) gobname= ""; /* In case it came in null */
  if ( !strcmp(gobname," ") ) gobname= ""; /* In case it's a single space */

  ger_debug("p3dgen: pg_open: creating new open gob <%s>",gobname);

  INIT_CHECK;

  if ( *gobname ) { /* Creating named gob */    
    /* Check to see that this is top-level open gob */
    if (cur_gob) {
      ger_error("p3dgen: pg_open: gob <%s> is already open; call ignored",
		cur_named_gob_name);
      return(P3D_FAILURE);
    }
    
    /* Free any existing gob of the same name */
    METHOD_RDY(gob_hash);
    if ( oldgob= (P_Gob *)(*(gob_hash->lookup))(gobname) ) {
      /* Old gob of same name exists; unhold it and try to free it.
       * If it has any parents, it won't get freed, but will live
       * on deprived of its name.
       */
      METHOD_RDY( oldgob );
      (*(oldgob->unhold))();
      (*(oldgob->destroy_self))(1);
      METHOD_RDY( gob_hash );
      (*(gob_hash->free))(gobname);
    }
      
    /* Hang on to the gob name */
    strncpy(cur_named_gob_name, gobname, P3D_NAMELENGTH-1);
    cur_named_gob_name[P3D_NAMELENGTH-1]= '\0';
  }

  /* Construct the new gob list cell and splice it into the list */
  if ( !(newcell= (P_Gob_List *)malloc(sizeof(P_Gob_List))) )
    ger_fatal("p3dgen: pg_open: unable to allocate %d bytes!",
	      sizeof(P_Gob_List));
  newcell->gob= po_create_gob(gobname);
  newcell->next= cur_gob;  /* leave prev blank; it's never used */
  cur_gob= newcell;
  
  if (*gobname) {
    /* Hold and hash the new gob */
    METHOD_RDY( cur_gob->gob );
    (*(cur_gob->gob->hold))();  /* mark it 'held' immediately. */
    METHOD_RDY( gob_hash );
    (*(gob_hash->add))(gobname,(P_Void_ptr)(cur_gob->gob));
  }

  return(P3D_SUCCESS);
}

static int add_child_gob( P_Gob *child )
/* This routine adds a child gob to the currently open gob. */
{
  ger_debug("p3dgen: add_child_gob");

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_child))(child);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: add_child_gob: no gob currently open- call ignored.");
    return(P3D_FAILURE);
  }
}

int pg_close( VOIDLIST )
/* This routine closes the currently open gob, popping back up the next
 * higher level open gob (if any).  If there is a higher level of open
 * gob, the gob which is closed becomes a child of the higher gob.
 */
{
  P_Gob_List *nextcell;
  P_Gob *newgob;

  ger_debug("p3dgen: pg_close: closing gob");

  INIT_CHECK;

  if (cur_gob) {
    if (pg_renderer_list) {
      METHOD_RDY(cur_gob->gob);
      APPLY_TO_ALL_RENDERERS( cur_gob->gob->define );
      nextcell= cur_gob->next;
      newgob= cur_gob->gob;
      free( (P_Void_ptr)cur_gob );
      cur_gob= nextcell;
      if (cur_gob) return( add_child_gob(newgob) ); /* add the new child */
      else return( P3D_SUCCESS );
    }
    else {
      ger_error("p3dgen: pg_close: must open a renderer first; call ignored");
      return(P3D_FAILURE);
    }
  }
  else {
    ger_error("p3dgen: pg_close: no gob is open");
    return( P3D_FAILURE );
  }
}

int pg_free( char *gobname )
/* This routine frees the given gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_free: freeing gob <%s>",gobname);

  INIT_CHECK;

  METHOD_RDY(gob_hash);
  thisgob= (P_Gob *)(*(gob_hash->lookup))(gobname);
  if (thisgob) {
    (*(gob_hash->free))(gobname);
    METHOD_RDY(thisgob);
    (*(thisgob->unhold))();
    (*(thisgob->destroy_self))(1);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_free: gob <%s> not defined; can't free it",gobname);
    return(P3D_FAILURE);
  }
}

int pg_int_attr( char *attribute, int value )
/* This routine adds an int-valued attribute to the currently open gob */
{
  ger_debug("p3dgen: pg_int_attr: adding pair <%s> -> %d",attribute,value);

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))( attribute, P3D_INT, (P_Void_ptr)&value );
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_int_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_bool_attr( char *attribute, int flag )
/* This routine adds a boolean-valued attribute to the currently open gob */
{
  ger_debug("p3dgen: pg_bool_attr: adding pair <%s> -> %d",attribute,flag);

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))(attribute,P3D_BOOLEAN,(P_Void_ptr)&flag);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_bool_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_float_attr( char *attribute, double value )
{
  float fvalue;

  ger_debug("p3dgen: pg_float_attr: adding pair <%s> -> %f",attribute,value);

  INIT_CHECK;

  fvalue= value;
  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))(attribute,P3D_FLOAT,(P_Void_ptr)&fvalue);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_float_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_string_attr( char *attribute, char *string )
{
  ger_debug("p3dgen: pg_string_attr: adding pair <%s> -> <%s>",
	    attribute,string);

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))(attribute,P3D_STRING,(P_Void_ptr)string);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_string_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_color_attr( char *attribute, P_Color *color )
{
  ger_debug("p3dgen: pg_color_attr: adding pair <%s> -> (%f %f %f %f)",
	    attribute, color->r, color->g, color->b, color->a);

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))(attribute,P3D_COLOR,(P_Void_ptr)color);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_color_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_point_attr( char *attribute, P_Point *point )
{
  ger_debug("p3dgen: pg_point_attr: adding pair <%s> -> (%f %f %f)",
	    attribute, point->x, point->y, point->z );

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))(attribute,P3D_POINT,(P_Void_ptr)point);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_point_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_vector_attr( char *attribute, P_Vector *vector )
{
  ger_debug("p3dgen: pg_vector_attr: adding pair <%s> -> (%f %f %f)",
	    attribute, vector->x, vector->y, vector->z );

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))(attribute,P3D_VECTOR,(P_Void_ptr)vector);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_vector_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_trans_attr( char *attribute, P_Transform *trans )
{
  ger_debug("p3dgen: pg_trans_attr: adding pair <%s> -> transform",attribute);

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))
      (attribute,P3D_TRANSFORM,(P_Void_ptr)trans);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_trans_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_material_attr( char *attribute, P_Material *material )
{
  ger_debug("p3dgen: pg_material_attr: adding pair <%s> -> (%d)",
	    attribute, material->type);

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_attribute))
      (attribute,P3D_MATERIAL,(P_Void_ptr)material);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_material_attr: no gob currently open!");
    return(P3D_FAILURE);
  }
}

int pg_gobcolor( P_Color *color )
/* This routine sets the color of the open gob */
{
  ger_debug("p3dgen: pg_gobcolor");
  INIT_CHECK;
  return( pg_color_attr("color",color) );
}

int pg_textheight( double value )
/* This routine sets the text height of the open gob */
{
  ger_debug("p3dgen: pg_textheight");
  INIT_CHECK;
  return( pg_float_attr("text-height",(float)value) );
}

int pg_backcull( int flag )
/* This routine sets the backcull flag of the open gob */
{
  ger_debug("p3dgen: pg_backcull");
  INIT_CHECK;
  return( pg_bool_attr("backcull",flag));
}

int pg_gobmaterial( P_Material *material )
/* This routines sets the material type of the open gob */
{
  ger_debug("p3dgen: pg_gobmaterial");
  INIT_CHECK;
  return( pg_material_attr("material",material) );
}

int pg_transform( P_Transform *transform )
/* This routine adds a transform to the open gob, possibly premultiplying
 * it with the existing transform.
 */
{
  ger_debug("p3dgen: pg_transform");

  INIT_CHECK;

  if (cur_gob) {
    METHOD_RDY(cur_gob->gob);
    (*(cur_gob->gob->add_transform))(transform);
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_transform: no gob currently open- call ignored");
    return(P3D_FAILURE);
  }  
}

int pg_translate( double x, double y, double z )
/* This routine adds a translation to the open gob, possibly premultiplying
 * it with the existing transform.
 */
{
  P_Transform *thistrans;
  P_Transform_type *trans_type;
  int retcode;

  ger_debug("p3dgen: pg_translate: translate by (%f %f %f)", x, y, z);

  INIT_CHECK;
  
  trans_type = allocate_trans_type();
  trans_type->type=P3D_TRANSLATION;
  trans_type->generators[0] = x;
  trans_type->generators[1] = y;
  trans_type->generators[2] = z;
  trans_type->generators[3] = 0;
  trans_type->next = NULL;
  thistrans= translate_trans(x,y,z);
  thistrans->type_front = trans_type;
  
  retcode= pg_transform(thistrans);
  destroy_trans_type(thistrans->type_front);
  destroy_trans(thistrans);
  return(retcode);
}

int pg_rotate( P_Vector *axis, double angle )
/* This routine adds a rotation to the open gob, possibly premultiplying
 * it with the existing transform.
 */
{
  P_Transform *thistrans;
  P_Transform_type *trans_type;
  int retcode;

  ger_debug("p3dgen: pg_rotate: rotate by %f about (%f %f %f)",
	    angle, axis->x, axis->y, axis->z);

  INIT_CHECK;

  trans_type = allocate_trans_type();
  trans_type->type=P3D_ROTATION;
  trans_type->generators[0] = axis->x;
  trans_type->generators[1] = axis->y;
  trans_type->generators[2] = axis->z;
  if(axis->x==0.0 && axis->y==0.0 && axis->z==0.0) {
    trans_type->generators[0] = 0;
    trans_type->generators[1] = 0;
    trans_type->generators[2] = 1;
  }
  trans_type->generators[3] = angle;
  trans_type->next = NULL;
  thistrans= rotate_trans( axis, angle );
  thistrans->type_front = trans_type;

  retcode= pg_transform(thistrans);
  /* Return an appropriate error code if axis was degenerate */
  if ((axis->x==0.0) && (axis->y==0.0) && (axis->z==0.0))
    retcode= P3D_FAILURE;
  destroy_trans_type(thistrans->type_front);
  destroy_trans(thistrans);
  return(retcode);
}

int pg_scale( double factor )
/* This routine adds a uniform rescaling transformation to the open gob,
 * possibly premultiplying it with the existing transform.
 */
{
  P_Transform *thistrans;
  P_Transform_type *trans_type;
  int retcode;

  ger_debug("p3dgen: pg_scale: scale uniformly by %f",factor);

  INIT_CHECK;

  trans_type = allocate_trans_type();
  trans_type->type=P3D_ASCALE;
  trans_type->generators[0] = factor;
  trans_type->generators[1] = factor;
  trans_type->generators[2] = factor;
  trans_type->generators[3] = 0;
  trans_type->next = NULL;
  thistrans= scale_trans( factor, factor, factor );
  thistrans->type_front = trans_type;

  retcode= pg_transform(thistrans);
  destroy_trans_type(thistrans->type_front);
  destroy_trans(thistrans);
  return(retcode);
}

int pg_ascale( double xfactor, double yfactor, double zfactor )
/* This routine adds a nonuniform rescaling transformation to the open gob,
 * possibly premultiplying it with the existing transform.
 */
{
  P_Transform *thistrans;
  P_Transform_type *trans_type;
  int retcode;

  ger_debug("p3dgen: pg_ascale: scale uniformly by (%f %f %f)",
	    xfactor, yfactor, zfactor);

  INIT_CHECK;

  trans_type = allocate_trans_type();
  trans_type->type=P3D_ASCALE;
  trans_type->generators[0] = xfactor;
  trans_type->generators[1] = yfactor;
  trans_type->generators[2] = zfactor;
  trans_type->generators[3] = 0;
  trans_type->next = NULL;
  thistrans= scale_trans( xfactor, yfactor, zfactor );
  thistrans->type_front = trans_type;

  retcode= pg_transform(thistrans);
  destroy_trans_type(thistrans->type_front);
  destroy_trans(thistrans);
  return(retcode);
}

int pg_child( char *gobname )
/* This routine adds a child to the currently open gob. */
{
  P_Gob *child;

  ger_debug("p3dgen: pg_child: adding child <%s>",gobname);

  INIT_CHECK;

  /* Check to make sure the user isn't adding the currently open named
   * GOB to itself, creating a cyclic graph of GOBs, which would cause
   * an infinite loop at rendering traversal time.
   */
  if (!strncmp(gobname,cur_named_gob_name,P3D_NAMELENGTH)) {
    ger_error(
  "p3dgen: pg_child: tried to make gob <%s> a child of itself- call ignored.",
	      gobname);
    return(P3D_FAILURE);
  }

  METHOD_RDY(gob_hash);
  child= (P_Gob *)(*(gob_hash->lookup))(gobname);
  if (child) return( add_child_gob(child) );
  else {
    ger_error("p3dgen: pg_child: child <%s> not defined- call ignored.",
	      gobname);
    return(P3D_FAILURE);
  }
}

int pg_print_gob( char *name )
/* This routine prints a description of the currently open top-level gob
 * to the standard output.
 */
{
  P_Gob *gob;

  ger_debug("p3dgen: pg_print_gob");

  INIT_CHECK;

  METHOD_RDY(gob_hash);
  if ( gob= (P_Gob *)(*(gob_hash->lookup))(name) ) {
    METHOD_RDY(gob);
    (*(gob->print))();
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_print_gob: gob <%s> not defined",name);
    return(P3D_FAILURE);
  }
}

int pg_cylinder( VOIDLIST )
/* This routine adds a cylinder primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_cylinder: adding cylinder");

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_cylinder("");
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_cylinder: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_sphere( VOIDLIST )
/* This routine adds a sphere primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_sphere: adding sphere");

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_sphere("");
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_sphere: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_torus( double major, double minor )
/* This routine adds a torus primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_torus: adding torus");

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_torus("",major,minor);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_torus: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_polymarker( P_Vlist *vlist )
/* This routine adds a polymarker primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_polymarker: adding polymarker of %d vertices",
	    vlist->length);

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_polymarker("",vlist);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    vlist->data_valid= 0; /* since user program may change or free it */
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_polymarker: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_polyline( P_Vlist *vlist )
/* This routine adds a polyline primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  INIT_CHECK;

  ger_debug("p3dgen: pg_polyline: adding polyline of %d vertices",
	    vlist->length);

  if (pg_renderer_list) {
    thisgob= po_create_polyline("",vlist);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    vlist->data_valid= 0; /* since user program may change or free it */
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_polyline: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_polygon( P_Vlist *vlist )
/* This routine adds a polygon primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_polygon: adding polygon of %d vertices",
	    vlist->length);

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_polygon("",vlist);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    vlist->data_valid= 0; /* since user program may change or free it */
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_polygon: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_tristrip( P_Vlist *vlist )
/* This routine adds a tristrip primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_tristrip: adding tristrip of %d vertices",
	    vlist->length);

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_tristrip("",vlist);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    vlist->data_valid= 0; /* since user program may change or free it */
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_tristrip: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_mesh( P_Vlist *vlist, int *vertices, int *facet_lengths, int nfacets )
/* This routine adds a mesh primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_mesh: adding mesh of %d vertices, %d facets",
	    vlist->length, nfacets);

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= 
      po_create_mesh("",vlist, vertices, facet_lengths, nfacets);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    vlist->data_valid= 0; /* since user program may change or free it */
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_mesh: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_bezier( P_Vlist *vlist )
/* This routine adds a bezier patch primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_bezier: adding bezier patch");

  INIT_CHECK;

  if (vlist->length != 16) {
    ger_error("p3dgen: pg_bezier: need 16 vertices for Bezier patch; got %d",
	      vlist->length);
    return(P3D_FAILURE);
  }

  if (pg_renderer_list) {
    thisgob= po_create_bezier("",vlist);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    vlist->data_valid= 0; /* since user program may change or free it */
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_bezier: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_text( char *text, P_Point *location, P_Vector *u, P_Vector *v )
/* This routine adds a text primitive gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_text: adding text <%s>",text);

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_text("", text, location, u, v);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_text: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_light( P_Point *location, P_Color *color )
/* This routine adds a light gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_light: adding light");

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_light("", location, color);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_light: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_ambient( P_Color *color )
/* This routine adds an ambient light gob to the currently open gob */
{
  P_Gob *thisgob;

  ger_debug("p3dgen: pg_ambient: adding ambient light");

  INIT_CHECK;

  if (pg_renderer_list) {
    thisgob= po_create_ambient("", color);
    METHOD_RDY(thisgob);
    APPLY_TO_ALL_RENDERERS( thisgob->define );
    return( add_child_gob(thisgob) );
  }
  else {
    ger_error("p3dgen: pg_ambient: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

int pg_camera( char *name, P_Point *lookfrom, P_Point *lookat,
	      P_Vector *up, double fovea, double hither, double yon )
/* This routine creates a camera.  If a camera of the same name already
 * exists, it gets replaced with the new camera.
 */
{
  P_Camera *oldcam, *thiscam;

  ger_debug("p3dgen: pg_camera: camera looking from (%f %f %f) to (%f %f %f)",
	    lookfrom->x, lookfrom->y, lookfrom->z, 
	    lookat->x, lookat->y, lookat->z);

  INIT_CHECK;

  if (pg_renderer_list) {

    METHOD_RDY(camera_hash);
    if ( oldcam= (P_Camera *)(*(camera_hash->lookup))(name) ) {
      (*(camera_hash->free))(name);
      METHOD_RDY(oldcam);
      (*(oldcam->destroy_self))();
    }

    thiscam= po_create_camera(name, lookfrom, lookat, up, fovea, hither, yon);
    METHOD_RDY(thiscam);
    APPLY_TO_ALL_RENDERERS( thiscam->define );
    METHOD_RDY(camera_hash);
    (*(camera_hash->add))
      ( name, (P_Void_ptr)thiscam );
    return(P3D_SUCCESS);  

  }
  else {
    ger_error("p3dgen: pg_camera: must open a renderer first; call ignored");
    return(P3D_FAILURE);
  }
}

int pg_print_camera( char *name )
/* This routine prints a description of the given camera to the standard
 * output.
 */
{
  P_Camera *thiscam;

  ger_debug("p3dgen: pg_print_camera");

  INIT_CHECK;

  METHOD_RDY(camera_hash);
  if (thiscam= (P_Camera *)(*(camera_hash->lookup))(name) ) {
    METHOD_RDY(thiscam);
    (*(thiscam->print))();
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_print_camera: camera <%s> not defined.",name);
    return(P3D_FAILURE);
  }
}

int pg_camera_background( char *name, P_Color *color )
/* This routine sets the background color of the given camera. */
{
  P_Camera *thiscam;

  ger_debug("p3dgen: pg_camera_background");

  INIT_CHECK;

  METHOD_RDY(camera_hash);
  if (thiscam= (P_Camera *)(*(camera_hash->lookup))(name) ) {
    METHOD_RDY(thiscam);
    (*(thiscam->set_background))( color );
    return(P3D_SUCCESS);
  }
  else {
    ger_error("p3dgen: pg_camera_background: camera <%s> not defined.",name);
    return(P3D_FAILURE);
  }
}

int pg_snap( char *gobname, char *lightname, char *cameraname )
/* This routine causes rendering of the given model gob with lights
 * provided by the given lighting gob and with the given camera.
 */
{
  P_Gob *model, *lights;
  P_Camera *camera;

  ger_debug("p3dgen: pg_snap: snap of <%s> with lights <%s> and camera <%s>",
	    gobname, lightname, cameraname );

  INIT_CHECK;

  METHOD_RDY(gob_hash);
  if ( !(model= (P_Gob *)(*(gob_hash->lookup))(gobname)) ) {
    ger_error("p3dgen: pg_snap: model gob <%s> not defined; call ignored.",
	      gobname);
    return( P3D_FAILURE );
  }
  if ( !(lights= (P_Gob *)(*(gob_hash->lookup))(lightname)) ) {
    ger_error("p3dgen: pg_snap: lighting gob <%s> not defined; call ignored.",
	      lightname);
    return( P3D_FAILURE );
  }

  METHOD_RDY(camera_hash);
  if ( !(camera= (P_Camera *)(*(camera_hash->lookup))(cameraname)) ) {
    ger_error("p3dgen: pg_snap: camera <%s> not defined; call ignored.",
	      cameraname);
    return( P3D_FAILURE );
  }

  METHOD_RDY(camera);
  (*(camera->set))();
  METHOD_RDY(lights);
  (*(lights->traverselights))( Identity_trans, po_default_attributes );
  METHOD_RDY(model);
  (*(model->render))( Identity_trans, po_default_attributes );

  return( P3D_SUCCESS );
}

int pg_set_cmap(double min, double max, 
		       void (*mapfun)( float *, float *, float *,
				      float *, float * ) )
/* This function establishes the given map function to be the current
 * color map function, and the given bounds to be the current bounds.
 */
{
  ger_debug("p3dgen: pg_set_cmap: range= %f to %f", min, max);

  INIT_CHECK;

  if (cur_cmap) {
    METHOD_RDY(cur_cmap);
    (*(cur_cmap->destroy_self))();
  }

  cur_cmap= po_create_color_map( "cur_cmap", min, max, mapfun );
  METHOD_RDY(cur_cmap);
  APPLY_TO_ALL_RENDERERS( cur_cmap->define );
  (*(cur_cmap->install))();

  return( P3D_SUCCESS );
}

int pg_std_cmap(double min, double max, int whichmap)
/* This function establishes the given bounds to be the current color
 * map bounds, and establishes the 'whichmap'th standard color map
 * function to be the current map function.
 */
{
  ger_debug("p3dgen: pg_std_cmap: map %d, range= %f to %f",whichmap,min,max);

  INIT_CHECK;

  if (whichmap<NUM_STANDARD_MAPFUNS)
    return( pg_set_cmap( min, max, po_standard_mapfuns[whichmap] ) );
  else {
    ger_error("p3dgen: pg_std_cmap: only %d color maps exist, call ignored",
	      NUM_STANDARD_MAPFUNS);
    return( P3D_FAILURE );
  }
}

int pg_cmap_color( float *val, float *r, float *g, float *b, float *a )
/* This routine uses the current color map and range to calculate color
 * values.
 */
{
  float lclval;

  ger_debug("p3dgen: pg_cmap_color: converting value %f", *val);

  /* Scale, clip, and map. */
  lclval= ( *val - cur_cmap->min )/( cur_cmap->max - cur_cmap->min );
  if ( lclval > 1.0 ) lclval= 1.0;
  if ( lclval < 0.0 ) lclval= 0.0;
  (*(cur_cmap->mapfun))( &lclval, r, g, b, a );
  return P3D_SUCCESS;
}

int pg_gob_open( VOIDLIST )
/* Checks for if a renderer is open */
{
  INIT_CHECK;

  if (cur_gob)
    return( P3D_SUCCESS );
  else 
    return( P3D_FAILURE );
}
