/****************************************************************************
 * assist.h
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
/* This include file provides the interface to the rendering assist object */

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
#define __(prototype) ()
#else
#define __(prototype) prototype
#endif

/* Characters in a text font cache */
#define MAX_FONT_CHARS 128

typedef struct P_Assist_struct {

  /* attribute-value pair handling facilities */
  void (*push_attributes) __(( P_Attrib_List *));  /* push new attributes */
  void (*pop_attributes) __(( P_Attrib_List *));   /* pop attributes */
  int (*bool_attribute) __(( P_Symbol ));          /* get boolean attribute */
  P_Color *(*color_attribute) __(( P_Symbol ));    /* get color attribute */
  P_Material *(*material_attribute) __(( P_Symbol )); /* get material attr */
  float (*float_attribute) __(( P_Symbol ));       /* get float attribute */
  int (*int_attribute) __(( P_Symbol ));           /* get integer attribute */
  char *(*string_attribute) __(( P_Symbol ));      /* get string attribute */

  /* Primitive assist facility */
  P_Void_ptr (*def_sphere) __(( void ));             /* define a sphere */
  void (*ren_sphere) __(( P_Void_ptr, P_Transform *, 
			 P_Attrib_List *));          /* render it */
  void (*destroy_sphere) __(( P_Void_ptr ));         /* destroy it */
  P_Void_ptr (*def_cylinder) __(( void ));           /* define a cylinder */
  void (*ren_cylinder) __(( P_Void_ptr, P_Transform *, 
			 P_Attrib_List *));          /* render it */
  void (*destroy_cylinder) __(( P_Void_ptr ));       /* destroy it */
  P_Void_ptr (*def_torus) __(( double,double ));     /* define a torus */
  void (*ren_torus) __(( P_Void_ptr, P_Transform *, 
			 P_Attrib_List *));          /* render it */
  void (*destroy_torus) __(( P_Void_ptr ));          /* destroy it */

  /* Spline assist facility */
  P_Void_ptr (*def_bezier) __(( P_Vlist * ));      /* define a bezier patch */
  void (*ren_bezier) __(( P_Void_ptr, P_Transform *, 
			 P_Attrib_List *));        /* render it */
  void (*destroy_bezier) __(( P_Void_ptr ));       /* destroy it */

  /* Text assist facility */
  P_Void_ptr (*def_text) __((char *, P_Point *, 
			     P_Vector *, P_Vector *)); /* define text string */
  void (*ren_text) __(( P_Void_ptr, P_Transform *,
		       P_Attrib_List *));          /* render it */
  void (*destroy_text) __(( P_Void_ptr ));         /* destroy it */

  /* Transform assist facility */
  P_Transform* (*clear_trans_stack) __((void));
  P_Transform* (*load_trans) __((P_Transform *));
  P_Transform* (*push_trans) __((P_Transform *));
  P_Transform* (*pop_trans) __((void));
  P_Transform* (*get_trans) __((void));

  /* Methods and storage for the whole object */
  void (*reset_self) __(( int ));              /* reset the assist module */
  void (*destroy_self) __((void));             /* destroy entire assist obj */
  P_Void_ptr object_data;                      /* object data */
} P_Assist;

/* Struct for object data, and access functions for it (not to be manipulated
 * by anyone but the assist object).
 */
typedef struct assist_data_struct {
  /* General stuff */
  int initialized;
  P_Renderer *renderer;

  /* Attribute handling */
  P_Int_Hash *attrhash;  /* attribute hash table */

  /* Primitive handling */
  P_Void_ptr spheredata; /* hook for predefined sphere */
  P_Void_ptr cyldata;    /* hook for predefined cylinder */

  /* Text emulation facility */
  struct font_cache_struct { 
    int ready;           /* non-zero if this character is loaded */
    int length;          /* number of strokes in the character */
    P_Void_ptr *data;    /* array of renderer data, one per stroke */
  } font_cache[ MAX_FONT_CHARS ];  /* text font cache */
  int text_symbols_ready;
  float oldu[3];
  float oldv[3];
  P_Symbol text_height_symbol;
  P_Symbol text_font_symbol;
  float text_height;
  int text_font_id;
  char text_font[P3D_NAMELENGTH];

  /* Transformation facility */
  P_Transform* trans_stack;
  int trans_stack_depth;
  P_Transform* current_trans;

} P_Assist_data;

#define ASTDATA( self ) ((P_Assist_data *)(self->object_data))
#define RENDERER( self ) (ASTDATA(self)->renderer)
#define ATTRHASH( self ) (ASTDATA(self)->attrhash)
#define SPHEREDATA( self ) (ASTDATA(self)->spheredata)
#define CYLDATA( self ) (ASTDATA(self)->cyldata)
#define FONT_CACHE( self ) (ASTDATA(self)->font_cache)
#define TEXT_SYMBOLS_READY( self ) (ASTDATA(self)->text_symbols_ready)
#define OLDU( self ) (ASTDATA(self)->oldu)
#define OLDV( self ) (ASTDATA(self)->oldv)
#define TEXT_HEIGHT_SYMBOL(self) (ASTDATA(self)->text_height_symbol)
#define TEXT_FONT_SYMBOL(self) (ASTDATA(self)->text_font_symbol)
#define TEXT_HEIGHT(self) (ASTDATA(self)->text_height)
#define TEXT_FONT_ID(self) (ASTDATA(self)->text_font_id)
#define TEXT_FONT(self) (ASTDATA(self)->text_font)
#define TRANS_STACK(self) (ASTDATA(self)->trans_stack)
#define TRANS_STACK_DEPTH(self) (ASTDATA(self)->trans_stack_depth)
#define TRANS(self) (ASTDATA(self)->current_trans)

/* Attribute assist module entry points (not to be called directly) */
#ifdef __cplusplus
extern "C" void ast_attr_reset( int );
extern "C" void ast_attr_destroy(void);
extern "C" void ast_attr_init( P_Assist * );
#else
extern void ast_attr_reset ___(( int ));
extern void ast_attr_destroy ___((void));
extern void ast_attr_init ___(( P_Assist * ));
#endif

/* Geometrical primitive assist module entry points (not to be 
 * called directly) 
 */
#ifdef __cplusplus
extern "C" void ast_prim_reset( int );
extern "C" void ast_prim_destroy(void);
extern "C" void ast_prim_init( P_Assist * );
#else
extern void ast_prim_reset ___(( int ));
extern void ast_prim_destroy ___((void));
extern void ast_prim_init ___(( P_Assist * ));
#endif

/* Spline primitive assist module entry points (not to be
 * called directly)
 */
#ifdef __cplusplus
extern "C" void ast_spln_reset( int );
extern "C" void ast_spln_destroy(void);
extern "C" void ast_spln_init( P_Assist * );
#else
extern void ast_spln_reset ___(( int ));
extern void ast_spln_destroy ___((void));
extern void ast_spln_init ___(( P_Assist * ));
#endif

/* Text primitive assist module entry points (not to be
 * called directly)
 */
#ifdef __cplusplus
extern "C" void ast_text_reset( int );
extern "C" void ast_text_destroy(void);
extern "C" void ast_text_init( P_Assist * );
#else
extern void ast_text_reset ___(( int ));
extern void ast_text_destroy ___((void));
extern void ast_text_init ___(( P_Assist * ));
#endif

/* Transformation assist module entry points (not to be
 * called directly)
 */
#ifdef __cplusplus
extern "C" void ast_trns_reset( int );
extern "C" void ast_trns_destroy(void);
extern "C" void ast_trns_init( P_Assist * );
#else
extern void ast_trns_reset ___(( int ));
extern void ast_trns_destroy ___((void));
extern void ast_trns_init ___(( P_Assist * ));
#endif

#ifdef __cplusplus
extern "C" P_Assist *po_create_assist( P_Renderer * );
#else
extern P_Assist *po_create_assist ___(( P_Renderer * ));
#endif

/* Clean up the prototyping macros */
#undef __
#undef ___

