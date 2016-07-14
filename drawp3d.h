/****************************************************************************
 * drawp3d.h
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
This include file provides definitions for the C language interface
to drawp3d.
*/

#ifndef INCL_DRAWP3D_H
#define INCL_DRAWP3D_H

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
#define __(prototype) ()
#else
#define __(prototype) prototype
#endif
  
/* DrawP3D version */
#define P3D_DRAWP3D_VERSION 3.3
  
/* Overall control routines */
extern int dp_shutdown ___(( void ));

/* Renderer manipulation routines */
extern int dp_init_ren ___(( char *, char *, char *, char * ));
extern int dp_open_ren ___(( char * ));
extern int dp_close_ren ___(( char * ));
extern int dp_shutdown_ren ___(( char * ));
extern int dp_print_ren ___(( char * ));

/* Gob manipulation routines */
extern int dp_open ___(( char * ));
extern int dp_close ___(( void ));
extern int dp_free ___(( char * ));
extern int dp_int_attr ___(( char *, int ));
extern int dp_bool_attr ___(( char *, int ));
extern int dp_float_attr ___(( char *, double ));
extern int dp_string_attr ___(( char *, char * ));
extern int dp_color_attr ___(( char *, P_Color * ));
extern int dp_point_attr ___(( char *, P_Point * ));
extern int dp_vector_attr ___(( char *, P_Vector * ));
extern int dp_trans_attr ___(( char *, P_Transform * ));
extern int dp_material_attr ___(( char *, P_Material * ));
extern int dp_gobcolor ___(( P_Color * ));
extern int dp_textheight ___(( double ));
extern int dp_backcull ___(( int ));
extern int dp_gobmaterial ___(( P_Material * ));
extern int dp_transform ___(( P_Transform * ));
extern int dp_translate ___(( double, double, double ));
extern int dp_rotate ___(( P_Vector *, double ));
extern int dp_scale ___(( double ));
extern int dp_ascale ___(( double, double, double ));
extern int dp_child ___(( char * ));
extern int dp_print_gob ___(( char * ));

/* Primitive gob routines */
extern int dp_cylinder ___(( void ));
extern int dp_sphere ___(( void ));
extern int dp_torus ___(( double, double ));
extern int dp_polymarker ___(( int, int, float *, int ));
extern int dp_polyline ___(( int, int, float *, int ));
extern int dp_polygon ___(( int, int, float *, int ));
extern int dp_tristrip ___(( int, int, float *, int ));
extern int dp_mesh ___(( int, int, float *, int, int *, int *, int ));
extern int dp_bezier ___(( int, int, float * ));
extern int dp_text ___(( char *, P_Point *, P_Vector *, P_Vector * ));
extern int dp_light ___(( P_Point *, P_Color * ));
extern int dp_ambient ___(( P_Color * ));

/* Composite gob routines */
extern int dp_boundbox ___(( P_Point *, P_Point * ));
extern int dp_axis ___(( P_Point *, P_Point *, P_Vector *, double, double,
                         int, char *, double, int));
extern int dp_isosurface ___(( int type, float *data, float *valdata,
		  int nx, int ny, int nz, double value,
		  P_Point *corner1, P_Point *corner2,
		  int show_inside ));
extern int dp_zsurface ___(( int, float *, float *, int, int, P_Point *, 
                  P_Point *, void (*) __(( int *, float *, int *, int * )) ));
extern int dp_rand_zsurf ___(( int, int, float *, int,
		  void (*) __(( int *, float *, float*, float *, int *  )) ));
extern int dp_rand_isosurf ___(( int, int, float *, int, double, int ));
extern int dp_irreg_zsurf ___(( int, float *, float *, int, int,
			       void (*) __((float *, float *, int *, int *)),
			       void (*) __((int *, float *, int *, int *)) ));
extern int dp_irreg_isosurf ___(( int type, float *data, float *valdata, 
				 int nx_in, int ny_in, int nz_in, double value,
				 void (*coordfun)(float *, float *, float *,
						  int *, int *, int *),
				 int show_inside ));
extern int dp_spline_tube ___(( int, int, float*, int, int*, int, int ));

/* Camera routines */
extern int dp_camera ___(( char *, P_Point *, P_Point *,P_Vector *, 
	      double, double, double ));
extern int dp_print_camera ___(( char * ));
extern int dp_camera_background ___(( char *, P_Color * ));

/* Snap */
extern int dp_snap ___(( char *, char *, char * ));

/* Color map manipulation routines */
extern int dp_set_cmap ___((double, double, 
		void (*) __(( float *, float *, float *, float *, float * ))));
extern int dp_std_cmap ___((double, double, int));

/* Debugging control */
extern int dp_debug ___(( void ));

/* Clean up the prototyping macros */
#undef ___
#undef __

/* Matches extern C block above */
#ifdef __cplusplus
};
#endif

/* This endif matches the test for INCL_DRAWP3D_H */
#endif
