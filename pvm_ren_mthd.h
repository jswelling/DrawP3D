/****************************************************************************
 * pvm_ren_mthd.h
 * Copyright 1995, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Joel Welling
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
/* This module provides includes for the PVM-mediated distributed renderer */

#define MAXSYMBOLLENGTH P3D_NAMELENGTH

#define TRANSFORM_BUF_TRIPLES 1000

/* Struct to hold info for a cached vertex list */
typedef struct vlist_cache_struct {
  int type;
  int length;
  int info_word;
  float *coords;
  float *colors;
  float *opacities;
  float *normals;
} P_Cached_Vlist;

typedef struct mesh_cache_struct {
  int nfacets;
  int nindices;
  int *facet_lengths;
  int *indices;
  P_Cached_Vlist* cached_vlist;
} P_Cached_Mesh;

typedef struct text_cache_struct {
  char* tstring;
  float coords[9];
} P_Cached_Text;

/* Struct to hold info for a color map */
typedef struct renderer_cmap_struct {
  char name[MAXSYMBOLLENGTH];
  double min;
  double max;
  void (*mapfun)( float *, float *, float *, float *, float * );
} P_Renderer_Cmap;

/* Struct for object data, and access functions for it */
typedef struct renderer_data_struct {
  char *name;
  int open;
  int initialized;
  int instance;
  int tid;
  int frame_number;
  int renserver_tid;
  P_Renderer_Cmap *current_cmap;
  int attrs_set;
  int current_backcull;
  P_Color current_color;
  P_Material current_material;
  float current_text_height;
  P_Camera *current_camera;
  P_Symbol backcull_symbol;     /* backcull symbol */
  P_Symbol text_height_symbol;  /* text height symbol */
  P_Symbol color_symbol;        /* color symbol */
  P_Symbol material_symbol;     /* material symbol */
  P_Assist *assist;             /* renderer assist object */
} P_Renderer_data;

#define RENDATA( self ) ((P_Renderer_data *)(self->object_data))
#define INSTANCE( self ) (RENDATA(self)->instance)
#define TID( self ) (RENDATA(self)->tid)
#define FRAME_NUMBER( self ) (RENDATA(self)->frame_number)
#define RENSERVER_TID( self ) (RENDATA(self)->renserver_tid)
#define NAME( self ) (RENDATA(self)->name)
#define CUR_MAP( self ) (RENDATA(self)->current_cmap)
#define MAP_NAME( self ) (CUR_MAP(self)->map_name)
#define MAP_MIN( self ) (CUR_MAP(self)->min)
#define MAP_MAX( self ) (CUR_MAP(self)->max)
#define MAP_FUN( self ) (CUR_MAP(self)->mapfun)
#define ATTRS_SET( self ) (RENDATA(self)->attrs_set)
#define CURRENT_BACKCULL(self) (RENDATA(self)->current_backcull)
#define CURRENT_COLOR(self) (RENDATA(self)->current_color)
#define CURRENT_MATERIAL(self) (RENDATA(self)->current_material)
#define CURRENT_TEXT_HEIGHT(self) (RENDATA(self)->current_text_height)
#define CURRENT_CAMERA(self) (RENDATA(self)->current_camera)
#define BACKCULLSYMBOL(self) (RENDATA(self)->backcull_symbol)
#define TEXTHEIGHTSYMBOL(self) (RENDATA(self)->text_height_symbol)
#define COLORSYMBOL(self) (RENDATA(self)->color_symbol)
#define MATERIALSYMBOL(self) (RENDATA(self)->material_symbol)
#define ASSIST(self) (RENDATA(self)->assist)
