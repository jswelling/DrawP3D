/****************************************************************************
 * drawp3d_ci.c
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
This module provides the C language interface to drawp3d.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "drawp3d.h"
#include "pgen_objects.h"
#include "ge_error.h"

int dp_init_ren( char *name, char *renderer, char *device, char *datastr )
{
  return( pg_init_ren( name, renderer, device, datastr ) );
}

int dp_open_ren( char *renderer )
{
  return( pg_open_ren( renderer ) );
}

int dp_close_ren( char *renderer )
{
  return( pg_close_ren( renderer ) );
}

int dp_shutdown_ren( char *renderer )
{
  return( pg_shutdown_ren( renderer ) );
}

int dp_print_ren( char *renderer )
{
  return( pg_print_ren( renderer ) );
}

int dp_shutdown( VOIDLIST )
{
  return( pg_shutdown() );
}

int dp_open( char *gobname )
{
  return( pg_open( gobname ) );
}

int dp_close( VOIDLIST )
{
  return( pg_close() );
}

int dp_free( char *gobname )
{
  return( pg_free( gobname ) );
}

int dp_int_attr( char *attribute, int value )
{
  return( pg_int_attr( attribute, value ) );
}

int dp_bool_attr( char *attribute, int flag )
{
  return( pg_bool_attr( attribute, flag ) );
}

int dp_float_attr( char *attribute, double value )
{
  return( pg_float_attr( attribute, value ) );
}

int dp_string_attr( char *attribute, char *string )
{
  return( pg_string_attr( attribute, string ) );
}

int dp_color_attr( char *attribute, P_Color *color )
{
  return( pg_color_attr( attribute, color ) );
}

int dp_point_attr( char *attribute, P_Point *point )
{
  return( pg_point_attr( attribute, point ) );
}

int dp_vector_attr( char *attribute, P_Vector *vector )
{
  return( pg_vector_attr( attribute, vector ) );
}

int dp_trans_attr( char *attribute, P_Transform *trans )
{
  return( pg_trans_attr( attribute, trans ) );
}

int dp_material_attr( char *attribute, P_Material *material )
{
  return( pg_material_attr( attribute, material ) );
}

int dp_gobcolor( P_Color *color )
{
  return( pg_gobcolor( color ) );
}

int dp_textheight( double value )
{
  return( pg_textheight( value ) );
}

int dp_backcull( int flag )
{
  return( pg_backcull( flag ) );
}

int dp_gobmaterial( P_Material *material )
{
  return( pg_gobmaterial( material ) );
}

int dp_transform( P_Transform *transform )
{
  P_Transform *tmp;

  transform->type_front = allocate_trans_type();
  transform->type_front->type = P3D_TRANSFORMATION;
  tmp = duplicate_trans(transform);
  transform->type_front->trans = (P_Void_ptr)tmp;
  transform->type_front->generators[0] = 0;
  transform->type_front->generators[1] = 0;
  transform->type_front->generators[2] = 0;
  transform->type_front->generators[3] = 0;
  transform->type_front->next = NULL;
  return( pg_transform( transform ) );
}

int dp_translate( double x, double y, double z )
{
  return( pg_translate( x, y, z ) );
}

int dp_rotate( P_Vector *axis, double angle )
{
  return( pg_rotate( axis, angle ) );
}

int dp_scale( double factor )
{
  return( pg_scale( factor ) );
}

int dp_ascale( double xfactor, double yfactor, double zfactor )
{
  return( pg_ascale( xfactor, yfactor, zfactor ) );
}

int dp_child( char *gobname )
{
  return( pg_child( gobname ) );
}

int dp_print_gob( char *name )
{
  return( pg_print_gob( name ) );
}

int dp_axis( P_Point *start, P_Point *end, P_Vector *v, double startval, 
             double endval, int num_tics, char *label, double text_height,
             int precision )
{
  return( pg_axis( start, end, v, startval, endval, num_tics, label, 
          text_height, precision ) );
}

int dp_boundbox( P_Point *corner1, P_Point *corner2)
{
  return( pg_boundbox( corner1, corner2 ) );
}

int dp_isosurface( int type, float *data, float *valdata,
		  int nx, int ny, int nz, double value,
		  P_Point *corner1, P_Point *corner2,
		  int show_inside )
{
  return( pg_isosurface( type, data, valdata, nx, ny, nz, value,
			corner1, corner2, show_inside, 0 ) );
}

int dp_zsurface( int vtxtype, float *zdata, float *valdata, 
                 int nx, int ny, P_Point *corner1, P_Point *corner2, 
                 void (*testfun)( int *, float *, int *, int * ) )
{
  return( pg_zsurface( vtxtype, zdata, valdata, nx, ny, 
                       corner1, corner2, testfun, 0 ) );
}

int dp_rand_zsurf( int vtxtype, int ctype, float *data, int npts,
		  void (*testfun)( int *, float *, float*, float *, int * ))
{
  return( pg_rand_zsurf( po_create_cvlist( vtxtype, npts, data ), testfun ) );
}

int dp_rand_isosurf( int vtxtype, int ctype, float *data, int npts, 
		    double ival, int show_inside )
{
  int retval;
  P_Vlist* vlist= po_create_cvlist( vtxtype, npts, data );
  retval= pg_rand_isosurf( vlist, ival, show_inside );
  METHOD_RDY(vlist);
  (*(vlist->destroy_self))();
  return( retval );  
}

int dp_irreg_zsurf( int vtxtype, float *zdata, float *valdata,
		   int nx, int ny,
		   void (*xyfun) ( float *, float *, int *, int *),
		   void (*testfun)( int *, float *, int *, int *) )
{
  return( pg_irreg_zsurf( vtxtype, zdata, valdata, nx, ny, 
			 xyfun, testfun, 0 ) );
}

int dp_irreg_isosurf( int type, float *data, float *valdata, 
		     int nx, int ny, int nz, double value,
		     void (*coordfun)(float *, float *, float *,
				      int *, int *, int *),
		     int show_inside )
{
  return( pg_irreg_isosurf( type, data, valdata, nx, ny, nz, value,
			   coordfun, show_inside, 0 ) );
}

int dp_spline_tube( int vtxtype, int ctype, float *data, int npts,
		    int* which_cross, int bres, int cres )
{
  int retval;
  P_Vlist* vlist;
  if (vtxtype==P3D_CNVTX) vtxtype= P3D_CVTX;
  if (vtxtype==P3D_CCNVTX) vtxtype= P3D_CCVTX;
  if (vtxtype==P3D_CVNVTX) vtxtype= P3D_CVVTX;
  vlist= po_create_cvlist( vtxtype, npts, data );
  retval= pg_spline_tube( po_create_cvlist( vtxtype, npts, data ),
			  which_cross, bres, cres );
  METHOD_RDY(vlist);
  (*(vlist->destroy_self))();
  return( retval );
}

int dp_cylinder( VOIDLIST )
{
  return( pg_cylinder() );
}

int dp_sphere( VOIDLIST )
{
  return( pg_sphere() );
}

int dp_torus( double major, double minor )
{ 
  return( pg_torus( major, minor ) );
}

int dp_polymarker( int vtxtype, int ctype, float *data, int npts )
{
  return( pg_polymarker( po_create_cvlist( vtxtype, npts, data ) ) );
}

int dp_polyline( int vtxtype, int ctype, float *data, int npts )
{
  return( pg_polyline( po_create_cvlist( vtxtype, npts, data ) ) );
}

int dp_polygon( int vtxtype, int ctype, float *data, int npts )
{
  return( pg_polygon( po_create_cvlist( vtxtype, npts, data ) ) );
}

int dp_tristrip( int vtxtype, int ctype, float *data, int npts )
{
  return( pg_tristrip( po_create_cvlist( vtxtype, npts, data ) ) );
}

int dp_mesh( int vtxtype, int ctype, float *vdata, int npts,
                   int *vertices, int *facet_lengths, int nfacets )
{
  return( pg_mesh( po_create_cvlist( vtxtype, npts, vdata ),
		   vertices, facet_lengths, nfacets ) );
}

int dp_bezier( int vtxtype, int ctype, float *data )
{
  return( pg_bezier( po_create_cvlist( vtxtype, 16, data ) ) );
}

int dp_text( char *text, P_Point *location, P_Vector *u, P_Vector *v )
{
  return( pg_text( text, location, u, v ) );
}
    
int dp_light( P_Point *location, P_Color *color )
{
  return( pg_light( location, color ) );
}

int dp_ambient( P_Color *color )
{
  return( pg_ambient( color ) );
}

int dp_camera( char *name, P_Point *lookfrom, P_Point *lookat,
	      P_Vector *up, double fovea, double hither, double yon )
{
  return( pg_camera( name, lookfrom, lookat, up, fovea, hither, yon ) );
}

int dp_print_camera( char *name )
{
  return( pg_print_camera( name ) );
}

int dp_camera_background( char *name, P_Color *color )
{
  return( pg_camera_background( name, color ) );
}

int dp_snap( char *gobname, char *lightname, char *cameraname )
{
  return( pg_snap( gobname, lightname, cameraname ) );
}

int dp_set_cmap(double min, double max, 
		       void (*mapfun)( float *, float *, float *,
				      float *, float * ) )
{
  return( pg_set_cmap( min, max, mapfun ) );
}

int dp_std_cmap(double min, double max, int whichmap)
{
  return( pg_std_cmap( min, max, whichmap ) );
}

int dp_debug( VOIDLIST )
{
  ger_toggledebug();
  return( P3D_SUCCESS );
}
