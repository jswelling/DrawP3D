/****************************************************************************
 * iv_ren_mthd.h
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Joel Welling, Damerlin Thompson
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
/* This module provides includes for the Open Inventor renderer */

#define MAXSYMBOLLENGTH P3D_NAMELENGTH

#define TRANSFORM_BUF_TRIPLES 1000

#define MAX_DEPTH 100

#define MAX_TABS 30
#define TABSPACE 3

#define LIGHTS 1
#define MODEL 2

#define ROWS 1
#define COLS 2

#define NON_INDEXED 2
#define INDEXED 3

#define INCREASE 1
#define DECREASE 0

typedef struct def_list_struct {
  P_Gob *gob;
  struct def_list_struct *next;
  char* use_name;
} Def_list;

typedef struct light_cache_struct {
  P_Point location;
  P_Color color;
} P_Cached_Light;

typedef struct torus_cache_struct {
  float major, minor;
} P_Cached_Torus;

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
  FILE *outfile;
  int file_num;
  int open;
  int initialized;
  P_Renderer_Cmap *current_cmap;
  int attrs_set;
  P_Camera *current_camera;
  P_Symbol backcull_symbol;     /* backcull symbol */
  P_Symbol text_height_symbol;  /* text height symbol */
  P_Symbol color_symbol;        /* color symbol */
  P_Symbol material_symbol;     /* material symbol */
  P_Assist *assist;             /* renderer assist object */
} P_Renderer_data;

#define RENDATA( self ) ((P_Renderer_data *)(self->object_data))
#define OUTFILE( self ) (RENDATA(self)->outfile)
#define FILENUM( self ) (RENDATA(self)->file_num)
#define NAME( self ) (RENDATA(self)->name)
#define CUR_MAP( self ) (RENDATA(self)->current_cmap)
#define MAP_NAME( self ) (CUR_MAP(self)->map_name)
#define MAP_MIN( self ) (CUR_MAP(self)->min)
#define MAP_MAX( self ) (CUR_MAP(self)->max)
#define MAP_FUN( self ) (CUR_MAP(self)->mapfun)
#define ATTRS_SET( self ) (RENDATA(self)->attrs_set)
#define CURRENT_CAMERA(self) (RENDATA(self)->current_camera)
#define BACKCULLSYMBOL(self) (RENDATA(self)->backcull_symbol)
#define TEXTHEIGHTSYMBOL(self) (RENDATA(self)->text_height_symbol)
#define COLORSYMBOL(self) (RENDATA(self)->color_symbol)
#define MATERIALSYMBOL(self) (RENDATA(self)->material_symbol)
#define ASSIST(self) (RENDATA(self)->assist)
