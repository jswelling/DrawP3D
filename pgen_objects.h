/****************************************************************************
 * pgen_objects.h
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
This module provides object information for object structures internal
to p3dgen.
*/

#ifndef INCL_PGEN_OBJ_H
#define INCL_PGEN_OBJ_H

/* Make things smooth for C++ compilation */
#ifdef __cplusplus
extern "C" {
#endif

/* Include prototypes unless requested not to.  NO_SUB_PROTO causes
 * all function prototypes not explicitly associated with an 'extern'ed
 * function to be omitted.  NO_PROTO causes all prototypes to be omitted.
 */
#ifdef NO_PROTO
#define NO_SUB_PROTO
#endif

#ifdef NO_PROTO
#define ___(prototype) ()
#else
#define ___(prototype) prototype
#endif
#ifdef NO_SUB_PROTO
#define ____(prototype) ()
#else
#define ____(prototype) prototype
#endif

/* Some useful constants */
#define PI 3.14159265
#define DegtoRad PI/180.0 
#define RadtoDeg 180.0/PI
#define EPSILON 0.00001

/* Global current object pointer */
extern P_Void_ptr po_this;

/* These should appear at the top and just before the return of methods. 
 * The last is used if the object has been rendered invalid.
 */
#define METHOD_IN P_Void_ptr po_this_save= po_this;
#define METHOD_OUT po_this= po_this_save;
#define METHOD_DESTROYED po_this= (P_Void_ptr)0;
#define METHOD_RDY( object ) po_this= (P_Void_ptr)(object);

/* Function to plug into otherwise empty methods */
extern void null_method( void );

/* Types of attribute values */
#define P3D_INT 0
#define P3D_BOOLEAN 1
#define P3D_FLOAT 2
#define P3D_STRING 3
#define P3D_COLOR 4
#define P3D_POINT 5
#define P3D_VECTOR 6
#define P3D_TRANSFORM 7
#define P3D_MATERIAL 8
#define P3D_OTHER 9

/* Color methods */
#ifdef __cplusplus
extern "C" void dump_color( P_Color * );
extern "C" P_Color *allocate_color( void );
extern "C" void destroy_color( P_Color * );
extern "C" void copy_color ( P_Color *, P_Color * );
extern "C" P_Color *duplicate_color( P_Color * );
extern "C" void rgbify_color( P_Color * );
#else /* __cplusplus not defined */
extern void dump_color ___(( P_Color * ));
extern P_Color *allocate_color ___(( void ));
extern void destroy_color ___(( P_Color * ));
extern void copy_color ___(( P_Color *, P_Color * ));
extern P_Color *duplicate_color ___(( P_Color * ));
extern void rgbify_color ___(( P_Color * ));
#endif /* __cplusplus */

/* Material methods */
#ifdef __cplusplus
extern "C" void dump_material( P_Material * );
extern "C" P_Material *allocate_material( void );
extern "C" void destroy_material( P_Material * );
extern "C" void copy_material( P_Material *, P_Material * );
extern "C" P_Material *duplicate_material( P_Material * );
#else /* __cplusplus not defined */
extern void dump_material ___(( P_Material * ));
extern P_Material *allocate_material ___(( void ));
extern void destroy_material ___(( P_Material * ));
extern void copy_material ___(( P_Material *, P_Material * ));
extern P_Material *duplicate_material ___(( P_Material * ));
#endif /* __cplusplus */

#define VEC_CREATE 0

/* Vector manipulation functions */
#ifdef __cplusplus
extern "C" P_Vector *vec_create( );
extern "C" void vec_destroy( P_Vector * );
extern "C" P_Vector *vec_copy( P_Vector *, P_Vector * );
extern "C" P_Vector *vec_convert_pt_to_vec( P_Point * );
extern "C" P_Point *vec_convert_vec_to_pt( P_Vector * );
extern "C" P_Vector *vec_add( P_Vector *, P_Vector *, P_Vector * );
extern "C" P_Vector *vec_sub( P_Vector *, P_Vector *, P_Vector * );
extern "C" P_Vector *vec_make_unit( P_Vector *, P_Vector * );
extern "C" P_Vector *vec_scale (P_Vector *, P_Vector *, float );
extern "C" float vec_length( P_Vector * );
extern "C" float vec_dot_product( P_Vector *, P_Vector * );
extern "C" P_Vector *normal_component_wrt( P_Vector *, P_Vector * );
#else /* __cplusplus not defined */
extern P_Vector *vec_create( void );
extern void vec_destroy( P_Vector * );
extern P_Vector *vec_copy( P_Vector *, P_Vector * );
extern P_Vector *vec_convert_pt_to_vec( P_Point * );
extern P_Point *vec_convert_vec_to_pt( P_Vector * );
extern P_Vector *vec_add( P_Vector *, P_Vector *, P_Vector * );
extern P_Vector *vec_sub( P_Vector *, P_Vector *, P_Vector * );
extern P_Vector *vec_make_unit( P_Vector *, P_Vector * );
extern P_Vector *vec_scale (P_Vector *, P_Vector *, float );
extern float vec_length( P_Vector * );
extern float vec_dot_product( P_Vector *, P_Vector * );
extern P_Vector *normal_component_wrt( P_Vector *, P_Vector * );
#endif /* __cplusplus */

/* Transform methods */
#ifdef __cplusplus
extern P_Transform *Identity_trans;
extern "C" void dump_trans_type( P_Transform_type * );
extern "C" void dump_trans( P_Transform * );
extern "C" P_Transform_type *allocate_trans_type( void );
extern "C" P_Transform *allocate_trans( void );
extern "C" void destroy_trans_type( P_Transform_type * );
extern "C" void destroy_trans( P_Transform * );
extern "C" void copy_trans_type( P_Transform_type *, P_Transform_type * );
extern "C" void copy_trans( P_Transform *, P_Transform * );
extern "C" P_Transform_type *duplicate_trans_type( P_Transform_type * );
extern "C" P_Transform *duplicate_trans( P_Transform * );
extern "C" P_Transform *transpose_trans( P_Transform * );
extern "C" P_Transform *premult_trans( P_Transform *, P_Transform * );
extern "C" P_Transform *postmult_trans( P_Transform *, P_Transform * );
extern "C" P_Transform *translate_trans( double, double, double );
extern "C" P_Transform *rotate_trans( P_Vector *, double );
extern "C" P_Transform *scale_trans( double, double, double );
extern "C" P_Vector *matrix_vector_mult( P_Transform *, P_Vector * );
extern "C" P_Transform_type *add_type_to_list( P_Transform_type *,
					       P_Transform_type * );
extern "C" P_Transform *flip_vec( P_Vector * );
extern "C" P_Transform *make_aligning_rotation( P_Vector *, P_Vector * );

#else /* __cplusplus not defined */
extern P_Transform *Identity_trans;
extern void dump_trans_type ___(( P_Transform_type * ));
extern void dump_trans ___(( P_Transform * ));
extern P_Transform_type *allocate_trans_type ___(( void ));
extern P_Transform *allocate_trans ___(( void ));
extern void destroy_trans_type ___(( P_Transform_type * ));
extern void destroy_trans ___(( P_Transform * ));
extern void copy_trans_type ___(( P_Transform_type *, P_Transform_type *));
extern void copy_trans ___(( P_Transform *, P_Transform *));
extern P_Transform_type *duplicate_trans_type ___(( P_Transform_type *));
extern P_Transform *duplicate_trans ___(( P_Transform *));
extern P_Transform *transpose_trans ___(( P_Transform * ));
extern P_Transform *premult_trans ___(( P_Transform *, P_Transform *));
extern P_Transform *postmult_trans ___(( P_Transform *, P_Transform *));
extern P_Transform *translate_trans ___(( double, double, double ));
extern P_Transform *rotate_trans ___(( P_Vector *, double ));
extern P_Transform *scale_trans ___(( double, double, double ));
extern P_Vector *matrix_vector_mult ___(( P_Transform *, P_Vector * ));
extern P_Transform_type *add_type_to_list ___(( P_Transform_type *,
						P_Transform_type * ));
extern P_Transform *flip_vec ___(( P_Vector * ));
extern P_Transform *make_aligning_rotation ___(( P_Vector *, P_Vector * ));
#endif /* __cplusplus */

/* Symbol manipulation functions and macros */
#ifdef __cplusplus
extern "C" P_Symbol create_symbol( char * );
extern "C" void dump_symbol( P_Symbol );
#else /* __cplusplus not defined */
extern P_Symbol create_symbol ___(( char * ));
extern void dump_symbol ___(( P_Symbol ));
#endif
#define symbol_equality( sym1, sym2 ) ( sym1 == sym2 )
#define symbol_string( symbol ) ((char *)symbol)

/* Attribute objects */
typedef struct P_Attrib_List_struct {
  P_Symbol attribute;                    /* attribute */
  int type;                              /* value type */
  P_Void_ptr value;                      /* value */
  struct P_Attrib_List_struct *next;     /* next in list */
  struct P_Attrib_List_struct *prev;     /* previous in list */
} P_Attrib_List;

/* Attribute methods */
extern P_Attrib_List *po_default_attributes;
#ifdef __cplusplus
extern "C" P_Attrib_List *po_gen_default_attr();
extern "C" P_Attrib_List *add_attr( P_Attrib_List *, char *, 
				    int, P_Void_ptr ); 
                                                /* prepends new attribute */
extern "C" void print_attr(P_Attrib_List *attr);   /* prints entire list */
extern "C" void destroy_attr(P_Attrib_List *attr); /* destroys entire list */
#else /* __cplusplus not defined */
extern P_Attrib_List *po_gen_default_attr ___(( void ));
extern P_Attrib_List *add_attr ___(( P_Attrib_List *, char *, 
				    int, P_Void_ptr )); 
                                                /* prepends new attribute */
extern void print_attr ___((P_Attrib_List *attr));   /* prints entire list */
extern void destroy_attr ___((P_Attrib_List *attr)); /* destroys entire list */
#endif /* __cplusplus */

/* Attribute access functions */
#define attribute_boolean( attr ) ( (*(int *)attr->value) ? 1 : 0 )
#define attribute_int( attr ) ( *(int *)attr->value )
#define attribute_float( attr ) ( *(float *)attr->value )
#define attribute_string( attr ) ( (char *)attr->value )
#define attribute_color( attr ) ( (P_Color *)attr->value )
#define attribute_point( attr ) ( (P_Point *)attr->value )
#define attribute_vector( attr ) ( (P_Vector *)attr->value )
#define attribute_transform( attr ) ( (P_Transform *)attr->value )
#define attribute_material( attr ) ( (P_Material *)attr->value )

/* forward definition */
struct P_Gob_struct;
struct P_Camera_struct;

/* renderer object */
typedef struct P_Renderer_struct {
  char name[P3D_NAMELENGTH];            /* renderer name */
  void (*print) ____(( void ));           /* print method */
  void (*open) ____(( void ));            /* open method- activate renderer */
  void (*close) ____(( void ));           /* close method- deactivate renderer */
  void (*destroy_self) ____(( void ));    /* destroy method */

  /* primitive methods */
  P_Void_ptr (*def_sphere) ____((char *));
  void (*ren_sphere) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_sphere) ____(( P_Void_ptr ));
  P_Void_ptr (*def_cylinder) ____((char *));
  void (*ren_cylinder) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_cylinder) ____(( P_Void_ptr ));
  P_Void_ptr (*def_torus) ____(( char *, double, double ));
  void (*ren_torus) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_torus) ____(( P_Void_ptr ));
  P_Void_ptr (*def_polymarker) ____(( char *, P_Vlist * ));
  void (*ren_polymarker) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_polymarker) ____(( P_Void_ptr ));
  P_Void_ptr (*def_polyline) ____(( char *, P_Vlist * ));
  void (*ren_polyline) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_polyline) ____(( P_Void_ptr ));
  P_Void_ptr (*def_polygon) ____(( char *, P_Vlist * ));
  void (*ren_polygon) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_polygon) ____(( P_Void_ptr ));
  P_Void_ptr (*def_tristrip) ____(( char *, P_Vlist * ));
  void (*ren_tristrip) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_tristrip) ____(( P_Void_ptr ));
  P_Void_ptr (*def_mesh) ____(( char *, P_Vlist *, 
			     int *, int *, int ));
  void (*ren_mesh) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_mesh) ____(( P_Void_ptr ));
  P_Void_ptr (*def_bezier) ____(( char *, P_Vlist * ));
  void (*ren_bezier) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*destroy_bezier) ____(( P_Void_ptr ));
  P_Void_ptr (*def_text) ____(( char *, char *, P_Point *, 
			     P_Vector *, P_Vector * ));
  void (*ren_text) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * )); 
  void (*destroy_text) ____(( P_Void_ptr ));
  P_Void_ptr (*def_light)
    ____(( char *, P_Point *, P_Color * ));
  void (*ren_light) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*light_traverse_light) ____(( P_Void_ptr, 
				   P_Transform *, P_Attrib_List * ));
  void (*destroy_light) ____(( P_Void_ptr ));
  P_Void_ptr (*def_ambient) ____(( char *, P_Color * ));
  void (*ren_ambient) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*light_traverse_ambient) ____(( P_Void_ptr, 
				     P_Transform *, P_Attrib_List * ));
  void (*destroy_ambient) ____(( P_Void_ptr ));

  /* general gob methods */
  P_Void_ptr (*def_gob) ____(( char *, struct P_Gob_struct * ));
  void (*hold_gob) ____((P_Void_ptr ));      /* hold- applies to prims also */
  void (*unhold_gob) ____((P_Void_ptr ));    /* unhold- applies to prims also */
  void (*ren_gob) ____(( P_Void_ptr, P_Transform *, P_Attrib_List * ));
  void (*light_traverse_gob) ____(( P_Void_ptr, 
				 P_Transform *, P_Attrib_List * ));
  void (*destroy_gob) ____(( P_Void_ptr ));

  /* camera handling methods */
  P_Void_ptr (*def_camera) ____(( struct P_Camera_struct * ));
  void (*set_camera) ____(( P_Void_ptr ));
  void (*destroy_camera) ____(( P_Void_ptr ));

  /* color map handling methods */
  P_Void_ptr (*def_cmap) ____(( char *, double, double, 
		    void (*)( float *, float *, float *, float *, float * ) ));
                                             /* define map */
  void (*install_cmap) ____(( P_Void_ptr ));   /* make this map active */
  void (*destroy_cmap) ____(( P_Void_ptr ));   /* destroy this map */   

  P_Void_ptr object_data;  /* object data */
} P_Renderer;

/* Create a p3d-generator renderer */
#ifdef __cplusplus
extern "C" P_Renderer *po_create_p3d_renderer( char *, char * );
extern "C" P_Renderer *po_create_painter_renderer( char *, char * );
extern "C" P_Renderer *po_create_gl_renderer( char *, char * );
extern "C" P_Renderer *po_create_xpainter_renderer( char *, char * );
extern "C" P_Renderer *po_create_pvm_renderer( char *, char * );
extern "C" P_Renderer *po_create_iv_renderer( char *, char * );
extern "C" P_Renderer *po_create_vrml_renderer( char *, char * );
extern "C" P_Renderer *po_create_lvr_renderer( char *, char * );
#else
extern P_Renderer *po_create_p3d_renderer ___(( char *, char * ));
extern P_Renderer *po_create_painter_renderer ___(( char *, char * ));
extern P_Renderer *po_create_gl_renderer ___(( char *, char * ));
extern P_Renderer *po_create_xpainter_renderer ___(( char *, char * ));
extern P_Renderer *po_create_pvm_renderer ___(( char *, char * ));
extern P_Renderer *po_create_iv_renderer ___(( char *, char * ));
extern P_Renderer *po_create_vrml_renderer ___(( char *, char * ));
extern P_Renderer *po_create_lvr_renderer ___(( char *, char * ));
#endif /* __cplusplus */

/* List of renderers, and how to walk it */
typedef struct ren_list_cell_struct {
  P_Renderer *renderer;
  char name[P3D_NAMELENGTH];
  struct ren_list_cell_struct *next, *prev;
} P_Ren_List_Cell;

extern P_Ren_List_Cell *pg_renderer_list;

#define APPLY_TO_ALL_RENDERERS( operation ) { \
  P_Ren_List_Cell *thiscell= pg_renderer_list; \
  while (thiscell) { \
    (*(operation))( thiscell->renderer ); \
    thiscell= thiscell->next; \
  } \
}

/* camera objects */
typedef struct P_Camera_struct {
  char name[P3D_NAMELENGTH];                 /* camera name string */
  int changed;                               /* changed since last defined */
  P_Point lookfrom;                          /* lookfrom point */
  P_Point lookat;                            /* lookat point */
  P_Vector up;                               /* up direction */
  P_Color background;                        /* background color */
  double fovea;                              /* fovea angle */
  double hither;                             /* hither distance */
  double yon;                                /* yon distance */
  void (*print) ____(( void ));                /* print method */
  void (*define) ____((P_Renderer *));         /* define self to renderer */
  void (*set) ____(( void ));                  /* render method */
  void (*destroy_self) ____(( void ));         /* destroy method */
  void (*set_background) ____((P_Color *));    /* set background color */
  P_Void_ptr object_data;                    /* object data */
} P_Camera;

#ifdef __cplusplus
extern "C" P_Camera *po_create_camera( char *, P_Point *, P_Point *, 
				       P_Vector *, double, double, double );
#else
extern P_Camera *po_create_camera ___(( char *, P_Point *, P_Point *, 
				       P_Vector *, double, double, double ));
#endif

/* gob object and gob list */
struct P_Gob_List_struct;

typedef struct P_Gob_struct {
  char name[P3D_NAMELENGTH];               /* gob name */
  int held;                                /* is it held? */
  int parents;                             /* count of parents */
  struct P_Gob_List_struct *children;      /* children (possibly null) */
  P_Attrib_List *attr;                     /* attribute list (possibly null) */
  int has_transform;                       /* flag for transform */
  int geomIndex;                           /* used in vrml renderer  */
  P_Transform trans;                       /* transformation (possibly null) */
  void (*define) ____((P_Renderer *));     /* define self to given renderer */
  void (*render) ____(( P_Transform *, P_Attrib_List * ));  /* render method */
  void (*render_to_ren) ____((P_Renderer *, P_Transform *, P_Attrib_List *));
                                           /* render to a single renderer */
  void (*traverselights) ____(( P_Transform *, P_Attrib_List * ));
                                           /* lighting traversal method */
  void (*traverselights_to_ren) ____(( P_Renderer *,
				    P_Transform *, P_Attrib_List * ));
                                           /* lighting traversal to one ren */
#ifdef __cplusplus
  void (*add_attribute)( const char *, const int, const P_Void_ptr );
                                           /* add an attribute-value pair */
#else
  void (*add_attribute) ____(( char *, int, P_Void_ptr ));
                                           /* add an attribute-value pair */
#endif
  void (*add_transform) ____(( P_Transform * ));    /* premultiply a transform */
  void (*add_child) ____(( struct P_Gob_struct * ));/* add a child */
  void (*print) ____(( void ));              /* print method */
  void (*hold) ____(( void ));               /* hold the gob */
  void (*unhold) ____(( void ));             /* unhold the gob */
  void (*destroy_self) ____(( int ));        /* destroy method */
  P_Void_ptr (*get_ren_data) ____((P_Renderer *)); /* returns renderer data */
  P_Void_ptr object_data;                  /* object data */
} P_Gob;

typedef struct P_Gob_List_struct {
  struct P_Gob_List_struct *next, *prev; /* adjacent members of list */
  P_Gob *gob;                            /* this list element */
} P_Gob_List;

#ifdef __cplusplus
extern "C" P_Gob *po_create_gob( char * );  /* creates generic gob */
#else
extern P_Gob *po_create_gob ___(( char * ));  /* creates generic gob */
#endif

/* po_create_primitive creates base-type primitive gobs, which may lack
 * some necessary methods.  Use one of the primitive creation routines
 * below instead of this routine.
 */
#ifdef __cplusplus
extern "C" P_Gob *po_create_primitive( char * );  
#else
extern P_Gob *po_create_primitive ___(( char * ));  
#endif

/* Different sub-types of primitives */
#ifdef __cplusplus
extern "C" P_Gob *po_create_cylinder( char * );
extern "C" P_Gob *po_create_sphere( char * );
extern "C" P_Gob *po_create_torus( char *, double, double );
extern "C" P_Gob *po_create_polymarker( char *, P_Vlist * );
extern "C" P_Gob *po_create_polyline( char *, P_Vlist * );
extern "C" P_Gob *po_create_polygon( char *, P_Vlist * );
extern "C" P_Gob *po_create_tristrip( char *, P_Vlist * );
extern "C" P_Gob *po_create_mesh( const char *, const P_Vlist *, 
				 const int *, const int *, const int );
extern "C" P_Gob *po_create_bezier( char *, P_Vlist * );
extern "C" P_Gob *po_create_text( const char *, const char *, const P_Point *,
                             const P_Vector *, const P_Vector * );
extern "C" P_Gob *po_create_light( char *, P_Point *, P_Color * );
extern "C" P_Gob *po_create_ambient( char *, P_Color * );
#else /* __cplusplus not defined */
extern P_Gob *po_create_cylinder ___(( char * ));
extern P_Gob *po_create_sphere ___(( char * ));
extern P_Gob *po_create_torus ___(( char *, double, double ));
extern P_Gob *po_create_polymarker ___(( char *, P_Vlist * ));
extern P_Gob *po_create_polyline ___(( char *, P_Vlist * ));
extern P_Gob *po_create_polygon ___(( char *, P_Vlist * ));
extern P_Gob *po_create_tristrip ___(( char *, P_Vlist * ));
extern P_Gob *po_create_mesh ___(( char *, P_Vlist *, int *, int *, int ));
extern P_Gob *po_create_bezier ___(( char *, P_Vlist * ));
extern P_Gob *po_create_text ___(( char *, char *, P_Point *,
                             P_Vector *, P_Vector * ));
extern P_Gob *po_create_light ___(( char *, P_Point *, P_Color * ));
extern P_Gob *po_create_ambient ___(( char *, P_Color * ));
#endif

/* color map objects */
typedef struct P_Color_Map_struct {
  char name[P3D_NAMELENGTH];            /* color map name */
  double min, max;                      /* range of values to expect */
  void (*mapfun) ____(( float *, float *, float *, float *, float * ));
                                        /* map function */
  void (*define) ____(( P_Renderer * ));  /* define self to given renderer */
  void (*install) ____(( void ));         /* install method */
  void (*print) ____(( void ));           /* print method */
  void (*destroy_self) ____(( void ));    /* destroy method */
  P_Void_ptr object_data;               /* object data */
} P_Color_Map;

#ifdef __cplusplus
extern "C" P_Color_Map *po_create_color_map( char *, double, double,
                                        void (*) ____(( float *,
                                                  float *, float *,
                                                  float *, float * )) );
#else
extern P_Color_Map *po_create_color_map ___(( char *, double, double,
					     void (*) ____(( float *,
						     float *, float *,
						     float *, float * )) ));
#endif

/* Hash table objects */
typedef struct P_String_Hash_struct {
  int size;
  P_Void_ptr walk_ptr;
  int walk_offset;
  P_Void_ptr (*lookup) ____((char *));           /* hash lookup */
  P_Void_ptr (*add) ____((char *, P_Void_ptr));  /* hash add */
  void (*free) ____((char *));                   /* hash delete */
  void (*destroy_self) ____((void));             /* destroy entire table */
  void (*walk_start) ____((void));               /* start walk of hash table */
  P_Void_ptr (*walk) ____((void));               /* walk hash table 1 step */
  P_Void_ptr object_data;                        /* object data */
} P_String_Hash;

#ifdef __cplusplus
extern "C" P_String_Hash *po_create_chash(int);
#else
extern P_String_Hash *po_create_chash ___((int));
#endif

typedef struct P_Int_Hash_struct {
  int size;
  int mask;
  P_Void_ptr (*lookup) ____((int));              /* hash lookup */
  P_Void_ptr (*add) ____((int, P_Void_ptr));     /* hash add */
  void (*free) ____((int));                      /* hash delete */
  void (*destroy_self) ____((void));             /* destroy entire table */
  P_Void_ptr object_data;                      /* object data */
} P_Int_Hash;

#ifdef __cplusplus
extern "C" P_Int_Hash *po_create_ihash(int);
#else
extern P_Int_Hash *po_create_ihash ___((int));
#endif

/* Clean up the prototyping macros */
#undef ____
#undef ___

/* Matches extern C block above */
#ifdef __cplusplus
};
#endif

/* This endif matches the test for INCL_PGEN_OBJ_H */
#endif
