/****************************************************************************
 * c_tester.c
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

#include <stdio.h>
#include <math.h>
#include "p3dgen.h"
#include "drawp3d.h"
#include "ge_error.h"
#include "random_flts.h"

#define ERRCHK( string ) if (!(string)) ger_error("ERROR!")

static int test_flag= 1;
static int test_int= 2;
static float test_float= 3.0;
static P_Color test_color= { P3D_RGB, 0.2, 0.3, 0.8, 0.9 };
static P_Color red_color= { P3D_RGB, 1.0, 0.0, 0.0, 1.0 };
static P_Color green_color= { P3D_RGB, 0.0, 1.0, 0.0, 1.0 };
static P_Color blue_color= { P3D_RGB, 0.0, 0.0, 1.0, 1.0 };
static P_Color black_color= { P3D_RGB, 0.0, 0.0, 0.0, 1.0 };
static P_Point test_point= { 0.3, 0.4, 0.5 };
static P_Vector test_vector= { 1.414, 0.0, 1.414 };
static char test_string[]= "This is my test string";

static P_Point corner1 = { -2.0, -2.0, -2.0 };
static P_Point corner2 = { 2.0, 2.0, 2.0 };
static P_Point axis_pt1 = { -2.0, -2.1, 2.5 };
static P_Point axis_pt2 = { 2.0, -2.1, 2.5 };
static P_Vector axis_up = { 0.0, 1.0, 0.0 };
static float *z, *zdata, *valdata;

P_Point cor1 = { -4.0, -4.0, 0.0 };
P_Point cor2 = { 4.0, 4.0, 0.0 };
P_Point cor1_14 = { -14.0, -14.0, -4.0 };
P_Point cor2_14 = { 14.0, 14.0, 10.0 };

static P_Transform test_trans= {{
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 6.0,
  0.0, 0.0, 6.0, 0.0,
  0.0, 0.0, 0.0, 1.0
  }};

static P_Point lookfrom= { 1.0, 2.0, 17.0 };
static P_Point lookat= { 1.0, 2.0, -3.0 };
static P_Vector up= { 0.0, 1.0, 0.0 };
static float fovea= 45.0;
static float hither= -5.0;
static float yon= -30.0;

static P_Point text_loc= { 0.0, 0.0, 3.0 };
static P_Vector text_u= { 1.0, 0.0, 0.0 };
static P_Vector text_v= { 0.0, 0.0, 1.0 };

static P_Point light_loc= {0.0, 1.0, 10.0};
static P_Color light_color= { P3D_RGB, 0.8, 0.8, 0.8, 0.3 };

static P_Color ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 0.8 };

#define ISO_NX 14
#define ISO_NY 15
#define ISO_NZ 16

#define MESH_LENGTH 9
#define MESH_FACETS 3
static int meshind[MESH_LENGTH]={
  0, 2, 1,
  1, 2, 3,
  0, 3, 2
  };
static int meshlengths[MESH_FACETS]= { 3, 3, 3 };
static int meshfacets= MESH_FACETS;

#define TSTRIP_CVVTX_LENGTH 100
static float tstrip_cvvtx_coords[4*TSTRIP_CVVTX_LENGTH];

static float b_cvtx_coords[3*16]= {
  0.0, 0.0, 0.0, 
  1.0, 0.0, 1.0, 
  2.0, 0.0, 1.0, 
  3.0, 0.0, 0.0,
  0.0, 1.0, 1.0, 
  1.0, 1.0, 2.0, 
  2.0, 1.0, 2.0, 
  3.0, 1.0, 1.0,
  0.0, 2.0, 1.0, 
  1.0, 2.0, 2.0, 
  2.0, 2.0, 2.0, 
  3.0, 2.0, 1.0,
  0.0, 3.0, 0.0, 
  1.0, 3.0, 1.0, 
  2.0, 3.0, 1.0, 
  3.0, 3.0, 0.0
  };

static float b_ccvtx_coords[7*16]= {
  0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.8,
  1.0, 0.0, 1.0, 0.6, 0.0, 0.3, 0.8,
  2.0, 0.0, 1.0, 0.3, 0.0, 0.6, 0.8,
  3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.8,
  0.0, 1.0, 1.0, 0.6, 0.0, 0.3, 0.8,
  1.0, 1.0, 2.0, 0.4, 0.1, 0.4, 0.8,
  2.0, 1.0, 2.0, 0.2, 0.2, 0.5, 0.8,
  3.0, 1.0, 1.0, 0.0, 0.3, 0.6, 0.8,
  0.0, 2.0, 1.0, 0.3, 0.0, 0.6, 0.8,
  1.0, 2.0, 2.0, 0.2, 0.2, 0.5, 0.8,
  2.0, 2.0, 2.0, 0.1, 0.4, 0.4, 0.8,
  3.0, 2.0, 1.0, 0.0, 0.6, 0.3, 0.8,
  0.0, 3.0, 0.0, 0.0, 0.0, 1.0, 0.8,
  1.0, 3.0, 1.0, 0.0, 0.3, 0.6, 0.8,
  2.0, 3.0, 1.0, 0.0, 0.6, 0.3, 0.8,
  3.0, 3.0, 0.0, 0.0, 1.0, 0.0, 0.8
  };

static float b_ccnvtx_coords[10*16]= {
  0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.8, 0.0, 0.0, 1.0,
  1.0, 0.0, 1.0, 0.6, 0.0, 0.3, 0.8, 0.0, 0.0, 1.0,
  2.0, 0.0, 1.0, 0.3, 0.0, 0.6, 0.8, 0.0, 0.0, 1.0,
  3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.8, 0.0, 0.0, 1.0,
  0.0, 1.0, 1.0, 0.6, 0.0, 0.3, 0.8, 0.0, 0.0, 1.0,
  1.0, 1.0, 2.0, 0.4, 0.1, 0.4, 0.8, 0.0, 0.0, 1.0,
  2.0, 1.0, 2.0, 0.2, 0.2, 0.5, 0.8, 0.0, 0.0, 1.0,
  3.0, 1.0, 1.0, 0.0, 0.3, 0.6, 0.8, 0.0, 0.0, 1.0,
  0.0, 2.0, 1.0, 0.3, 0.0, 0.6, 0.8, 0.0, 0.0, 1.0,
  1.0, 2.0, 2.0, 0.2, 0.2, 0.5, 0.8, 0.0, 0.0, 1.0,
  2.0, 2.0, 2.0, 0.1, 0.4, 0.4, 0.8, 0.0, 0.0, 1.0,
  3.0, 2.0, 1.0, 0.0, 0.6, 0.3, 0.8, 0.0, 0.0, 1.0,
  0.0, 3.0, 0.0, 0.0, 0.0, 1.0, 0.8, 0.0, 0.0, 1.0,
  1.0, 3.0, 1.0, 0.0, 0.3, 0.6, 0.8, 0.0, 0.0, 1.0,
  2.0, 3.0, 1.0, 0.0, 0.6, 0.3, 0.8, 0.0, 0.0, 1.0,
  3.0, 3.0, 0.0, 0.0, 1.0, 0.0, 0.8, 0.0, 0.0, 1.0
  };

static float b_cnvtx_coords[6*16]= {
  0.0, 0.0, 0.0, 0.0, 0.0, 1.0,
  1.0, 0.0, 1.0, 0.0, 0.0, 1.0,
  2.0, 0.0, 1.0, 0.0, 0.0, 1.0,
  3.0, 0.0, 0.0, 0.0, 0.0, 1.0,
  0.0, 1.0, 1.0, 0.0, 0.0, 1.0,
  1.0, 1.0, 2.0, 0.0, 0.0, 1.0,
  2.0, 1.0, 2.0, 0.0, 0.0, 1.0,
  3.0, 1.0, 1.0, 0.0, 0.0, 1.0,
  0.0, 2.0, 1.0, 0.0, 0.0, 1.0,
  1.0, 2.0, 2.0, 0.0, 0.0, 1.0,
  2.0, 2.0, 2.0, 0.0, 0.0, 1.0,
  3.0, 2.0, 1.0, 0.0, 0.0, 1.0,
  0.0, 3.0, 0.0, 0.0, 0.0, 1.0,
  1.0, 3.0, 1.0, 0.0, 0.0, 1.0,
  2.0, 3.0, 1.0, 0.0, 0.0, 1.0,
  3.0, 3.0, 0.0, 0.0, 0.0, 1.0
  };

static float b_cvvtx_coords[4*16]= {
  0.0, 0.0, 0.0, 5.0,
  1.0, 0.0, 1.0, 5.6,
  2.0, 0.0, 1.0, 6.3,
  3.0, 0.0, 0.0, 7.0,
  0.0, 1.0, 1.0, 5.6,
  1.0, 1.0, 2.0, 6.3,
  2.0, 1.0, 2.0, 7.0,
  3.0, 1.0, 1.0, 7.6,
  0.0, 2.0, 1.0, 6.3,
  1.0, 2.0, 2.0, 7.0,
  2.0, 2.0, 2.0, 7.6,
  3.0, 2.0, 1.0, 8.3,
  0.0, 3.0, 0.0, 7.0,
  1.0, 3.0, 1.0, 7.6,
  2.0, 3.0, 1.0, 8.3,
  3.0, 3.0, 0.0, 9.0
  };

static float b_cvnvtx_coords[7*16]= {
  0.0, 0.0, 0.0, 5.0, 0.0, 0.0, 1.0,
  1.0, 0.0, 1.0, 5.6, 0.0, 0.0, 1.0,
  2.0, 0.0, 1.0, 6.3, 0.0, 0.0, 1.0,
  3.0, 0.0, 0.0, 7.0, 0.0, 0.0, 1.0,
  0.0, 1.0, 1.0, 5.6, 0.0, 0.0, 1.0,
  1.0, 1.0, 2.0, 6.3, 0.0, 0.0, 1.0,
  2.0, 1.0, 2.0, 7.0, 0.0, 0.0, 1.0,
  3.0, 1.0, 1.0, 7.6, 0.0, 0.0, 1.0,
  0.0, 2.0, 1.0, 6.3, 0.0, 0.0, 1.0,
  1.0, 2.0, 2.0, 7.0, 0.0, 0.0, 1.0,
  2.0, 2.0, 2.0, 7.6, 0.0, 0.0, 1.0,
  3.0, 2.0, 1.0, 8.3, 0.0, 0.0, 1.0,
  0.0, 3.0, 0.0, 7.0, 0.0, 0.0, 1.0,
  1.0, 3.0, 1.0, 7.6, 0.0, 0.0, 1.0,
  2.0, 3.0, 1.0, 8.3, 0.0, 0.0, 1.0,
  3.0, 3.0, 0.0, 9.0, 0.0, 0.0, 1.0
  };

#define CVTX_LENGTH 4
static float cvtx_coords[3*CVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 
  2.0, 0.0, 0.0, 
  2.0, 2.0, 1.0, 
  0.0, 2.0, 1.0
  };

#define M_CVTX_LENGTH 4
static float m_cvtx_coords[3*M_CVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 
  2.0, 0.0, 0.0, 
  0.0, 2.0, 0.0, 
  0.0, 0.0, 2.0
  };

#define CNVTX_LENGTH 4
static float cnvtx_coords[6*CNVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  2.0, 0.0, 0.0, 1.0, 0.0, 0.0,
  2.0, 2.0, 1.0, 0.0, 1.0, 0.0,
  0.0, 2.0, 1.0, 0.0, 0.0, 1.0
  };

#define M_CNVTX_LENGTH 4
static float m_cnvtx_coords[6*M_CNVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  2.0, 0.0, 0.0, 1.0, 0.0, 0.0,
  0.0, 2.0, 0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 2.0, 0.0, 0.0, 1.0
  };

#define CCVTX_LENGTH 4
static float ccvtx_coords[7*CCVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.2,
  2.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.3, 
  2.0, 2.0, 1.0, 0.0, 1.0, 0.0, 0.4,
  0.0, 2.0, 1.0, 0.0, 0.0, 1.0, 0.5
  };

#define M_CCVTX_LENGTH 4
static float m_ccvtx_coords[7*M_CCVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.2,
  2.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.3, 
  0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 0.4,
  0.0, 0.0, 2.0, 0.0, 0.0, 1.0, 0.5
  };

#define CCNVTX_LENGTH 4
static float ccnvtx_coords[10*CCNVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.2, 0.0, 0.0, 0.0,
  2.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.3, 1.0, 0.0, 0.0,
  2.0, 2.0, 1.0, 0.0, 1.0, 0.0, 0.4, 0.0, 1.0, 0.0,
  0.0, 2.0, 1.0, 0.0, 0.0, 1.0, 0.5, 0.0, 0.0, 1.0
  };

#define M_CCNVTX_LENGTH 4
static float m_ccnvtx_coords[10*M_CCNVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.2, 0.0, 0.0, 0.0,
  2.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.3, 1.0, 0.0, 0.0,
  0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 0.4, 0.0, 1.0, 0.0,
  0.0, 0.0, 2.0, 0.0, 0.0, 1.0, 0.5, 0.0, 0.0, 1.0
  };

#define CVNVTX_LENGTH 4
static float cvnvtx_coords[7*CVNVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 5.3, 0.0, 0.0, 0.0,
  2.0, 0.0, 0.0, 6.4, 1.0, 0.0, 0.0,
  2.0, 2.0, 1.0, 7.5, 0.0, 1.0, 0.0,
  0.0, 2.0, 1.0, 8.6, 0.0, 0.0, 1.0
  };

#define M_CVNVTX_LENGTH 4
static float m_cvnvtx_coords[7*M_CVNVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 5.3, 0.0, 0.0, 0.0,
  2.0, 0.0, 0.0, 6.4, 1.0, 0.0, 0.0,
  0.0, 2.0, 0.0, 7.5, 0.0, 1.0, 0.0,
  0.0, 0.0, 2.0, 8.6, 0.0, 0.0, 1.0
  };

#define CVVTX_LENGTH 4
static float cvvtx_coords[4*CVVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 5.3,
  2.0, 0.0, 0.0, 6.4,
  2.0, 2.0, 1.0, 7.5,
  0.0, 2.0, 1.0, 8.6
  };

#define M_CVVTX_LENGTH 4
static float m_cvvtx_coords[4*M_CVVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 5.3,
  2.0, 0.0, 0.0, 6.4,
  0.0, 2.0, 0.0, 7.5,
  0.0, 0.0, 2.0, 8.6
  };

#define ZPTS 15
float data_zsurf[4*ZPTS]= {-10.0,    0.0,    0.0,   0.0,
                            10.0,    0.0,    0.0,   0.0,
		             0.0,    10.0,   0.0,   0.0,
		             0.0,    0.0,   10.0,  10.0,
		            -1.2,    -2.3, -14.0, -14.0,
		           -30.4,    92.1, -10.0, -10.0,
		            35.45,    34.3, -19.9, -19.9,
		           -42.1,    65.3,  -9.3,  -9.3,
		           -20.6,    30.5, -14.3, -14.3,
			    34.2,   -40.1, -15.3, -15.3,
			    -3.0,     59.0,   3.0,   3.0,
			    -5.0,     59.0,   3.0,   3.0,
			    -1.0,     57.0,   3.0,   3.0,
			    -8.0,     61.0,   3.0,   3.0,
			    -6.0,     70.0,   3.0,   3.0}; 

#define RAND_ISOSURF_PTS 1000

static float get_random_flt()
/* This function returns a float between 0.0 and 1.0.  We do it this
 * way to avoid differences in the random number generators on different
 * machines.  It returns a value between 0.0 and 1.0.
 */
{
  static int i= 0;
  float val;

  val= random_flts[i++];
  if (i >= NUM_RANDOM_FLTS) i= 0;
  return(val);
}

static void simple_prim_test( VOIDLIST )
/* This routine tests spheres, cylinders, tori, and text */
{
  ERRCHK( dp_open("rotbez") );
  ERRCHK( dp_rotate(&test_vector,270.0) );
  ERRCHK( dp_translate(-2.0,0.0,5.0) );
  ERRCHK( dp_gobcolor(&red_color) );
  ERRCHK( dp_gobmaterial(p3d_shiny_material) );
  ERRCHK( dp_bezier(P3D_CVTX, P3D_RGB, b_cvtx_coords) );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("shovecyl") );
  ERRCHK( dp_translate(0.0, 0.0, 2.0) );
  ERRCHK( dp_color_attr("color",&green_color) );
  ERRCHK( dp_cylinder() );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_gobcolor(&test_color) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_torus(3.0, 1.0) );
  ERRCHK( dp_text("Sample Text",&text_loc, &text_u, &text_v) );
  ERRCHK( dp_textheight(0.5) );
  ERRCHK( dp_child("shovecyl") );
  ERRCHK( dp_child("rotbez") );
  /* Add some random attributes */
  ERRCHK( dp_bool_attr("meaningless-bool",test_flag) );
  ERRCHK( dp_float_attr("meaningless-float",test_float) );    
  ERRCHK( dp_int_attr("meaningless-int",test_int) );
  ERRCHK( dp_string_attr("meaningless-string",test_string) );
  ERRCHK( dp_point_attr("meaningless-point",&test_point) );
  ERRCHK( dp_vector_attr("meaningless-vector",&test_vector) );
  ERRCHK( dp_trans_attr("meaningless-trans", &test_trans) );
  ERRCHK( dp_material_attr("meaningless_material", p3d_matte_material) );
  ERRCHK( dp_close() );
  ERRCHK( dp_camera_background("mycamera",&blue_color) );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
  ERRCHK( dp_camera_background("mycamera",&black_color) );
  ERRCHK( dp_print_gob("mygob") );

}

static void vlist_prim_test(int vtxtype, int ctype, float *data, int npts )
{
  ERRCHK( dp_open("shovepm") );
  ERRCHK( dp_translate(0.0, 0.0, 2.0) );
  ERRCHK( dp_polymarker(vtxtype, ctype, data, npts) );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_polygon(vtxtype, ctype, data, npts) );
  ERRCHK( dp_open("") );
  ERRCHK( dp_translate(0.0, 0.0, 1.0) );
  ERRCHK( dp_polyline(vtxtype, ctype, data, npts) );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("") );
  ERRCHK( dp_translate(0.0, 0.0, -1.0) );
  ERRCHK( dp_tristrip(vtxtype, ctype, data, npts) );
  ERRCHK( dp_close() );
  ERRCHK( dp_child("shovepm") );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
}

static void mesh_test(int vtxtype, int ctype, float *data, int npts )
{
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_backcull(1) );
  ERRCHK( dp_mesh( vtxtype, ctype, data, npts, meshind, meshlengths, 
		  meshfacets ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
}

static void bezier_test(int vtxtype, int ctype, float *data)
{
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_bezier( vtxtype, ctype, data ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
}

static void mymap( float *val, float *r, float *g, float *b, float *a )
{
  if (*val>0.5) {
    *r= 2.0 * (1.0 - *val);
    *g= 2.0 * (1.0 - *val);
    *b= 2.0 * (1.0 - *val);
    *a= 0.8;
  }
  else {
    *r= 2.0 * *val;
    *g= 2.0 * *val;
    *b= 2.0 * *val;
    *a= 0.8;
  }
}

static void cmap_test( VOIDLIST )
{
  float xmin= -4.0, xmax= 4.0, xstep, x, ymin= 0.0, ymax= 0.5;
  int i, length= TSTRIP_CVVTX_LENGTH/2;
  float *fpt;
  char label[80];
  static P_Point pt={ -2.0, 1.0, 0.0 };
  static P_Vector u={ 1.0, 0.0, 0.0 };
  static P_Vector v={ 0.0, 1.0, 0.0 };

  xstep= (xmax-xmin)/(length-1);
  fpt= tstrip_cvvtx_coords;
  x= xmin;
  for (i=0; i<length; i++) {
    *fpt++= x;
    *fpt++= ymax;
    *fpt++= 0.0;
    *fpt++= 7.0*((float)i)/((float)(length-1));
    *fpt++= x;
    *fpt++= ymin;
    *fpt++= 0.0;
    *fpt++= 7.0*((float)i)/((float)(length-1));
    x += xstep;
  }
  (void)sprintf(label,"Test Map");
  ERRCHK( dp_set_cmap(0.0, 7.0, mymap) );
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_tristrip( P3D_CVVTX, P3D_RGB, tstrip_cvvtx_coords, 2*length ) );
  ERRCHK( dp_text( label, &pt, &u, &v ) );
  ERRCHK( dp_textheight( 0.5 ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
}

static void std_cmap_test( int whichmap )
{
  static int initialized= 0;
  float xmin= -4.0, xmax= 4.0, xstep, x, ymin= 0.0, ymax= 0.5;
  int i, length= TSTRIP_CVVTX_LENGTH/2;
  float *fpt;
  char label[80];
  static P_Point pt={ -2.0, 1.0, 0.0 };
  static P_Vector u={ 1.0, 0.0, 0.0 };
  static P_Vector v={ 0.0, 1.0, 0.0 };

  if (!initialized) {
    xstep= (xmax-xmin)/(length-1);
    fpt= tstrip_cvvtx_coords;
    x= xmin;
    for (i=0; i<length; i++) {
      *fpt++= x;
      *fpt++= ymax;
      *fpt++= 0.0;
      *fpt++= 7.0*((float)i)/((float)(length-1));
      *fpt++= x;
      *fpt++= ymin;
      *fpt++= 0.0;
      *fpt++= 7.0*((float)i)/((float)(length-1));
      x += xstep;
    }
    initialized= 1;
  }
  sprintf(label,"Standard Map %d",whichmap);
  ERRCHK( dp_std_cmap(0.0, 7.0, whichmap) );
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_tristrip( P3D_CVVTX, P3D_RGB, tstrip_cvvtx_coords, 2*length ) );
  ERRCHK( dp_text( label, &pt, &u, &v ) );
  ERRCHK( dp_textheight( 0.5 ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
}

static void empty_gob_test( VOIDLIST )
{
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
  ERRCHK( dp_print_gob("mygob") );
}

static void scaling_test( VOIDLIST )
{
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_open("") );
  ERRCHK( dp_ascale(1.0,2.0,3.0) );
  ERRCHK( dp_translate(0.0,0.0,3.0) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_gobcolor( &red_color ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("") );
  ERRCHK( dp_scale(3.0) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_gobcolor( &blue_color ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("") );
  ERRCHK( dp_transform( &test_trans ) );
  ERRCHK( dp_cylinder() );
  ERRCHK( dp_close() );
  ERRCHK( dp_torus(3.5,0.5) );
  ERRCHK( dp_print_gob("mygob") );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
}

static void calc_iso_data( float data[ISO_NX][ISO_NY][ISO_NZ] )
{
  int i, j, k;
  float x, y, z, phi, r, eps= 0.001;

  for (i=0; i<ISO_NX; i++) {
    x= (float)(i-(ISO_NX-1)/2)/(float)((ISO_NX-1)/2);
    for (j=0; j<ISO_NY; j++) {
      y= (float)(j-(ISO_NY-1)/2)/(float)((ISO_NY-1)/2);
      r= sqrt( x*x + y*y );
      if (r<eps) phi= 0.0;
      else {
	phi= acos( x/sqrt(x*x+y*y) );
	if (y<0.0) phi= -phi;
      }
      for (k=0; k<ISO_NZ; k++) {
	z= (float)(k-(ISO_NZ-1)/2)/(float)((ISO_NZ-1)/2);
	data[i][j][k]= (x*x + y*y + z*z)/
	  (1.0+sin(3.0*phi+z)*sin(3.0*phi+z));
      }
    }
  }
}

static void calc_iso_valdata( float data[ISO_NX][ISO_NY][ISO_NZ] )
{
  int i, j, k;
  float x, y, z;

  for (i=0; i<ISO_NX; i++) {
    x= (float)(i-(ISO_NX-1)/2)/(float)((ISO_NX-1)/2);
    for (j=0; j<ISO_NY; j++) {
      y= (float)(j-(ISO_NY-1)/2)/(float)((ISO_NY-1)/2);
      for (k=0; k<ISO_NZ; k++) {
	z= (float)(k-(ISO_NZ-1)/2)/(float)((ISO_NZ-1)/2);
	data[i][j][k]= 0.5*(x*x + y*y);
      }
    }
  }
}

static void isosurf_test( VOIDLIST )
{
  P_Point corner1, corner2;
  float data[ISO_NX][ISO_NY][ISO_NZ];
  float valdata[ISO_NX][ISO_NY][ISO_NZ];
  float val= 0.5;
  int inner_surface= 1;

  corner1.x= -2.6;
  corner1.y= -2.5;
  corner1.z= 3.75;
  corner2.x= 4.6;
  corner2.y= 6.5;
  corner2.z= -9.75;

  calc_iso_data(data);
  calc_iso_valdata(valdata);

  ERRCHK( dp_std_cmap(0.0, 1.0, 1) );

  ERRCHK( dp_open("mygob") );

  ERRCHK( dp_isosurface( P3D_CVNVTX, (float *)data, (float *)valdata, 
			ISO_NX, ISO_NY, ISO_NZ, val, 
			&corner1, &corner2, inner_surface) );

  ERRCHK( pg_boundbox( &corner1, &corner2 ) );

  ERRCHK( dp_close() );

  ERRCHK( dp_snap("mygob","mylights","mycamera") );
}



static void create_zsurfdat( )
{
  float r, x, y, tz;
  int i, j;

  z = ( float * ) malloc( 841 * sizeof( float ) );
  
  for ( i= -14; i<15; i++ ) 
    for ( j= -14; j<15; j++ ) {
      r = (float) sqrt( (double) i*i + j*j );
      tz = (float) cos( (double) r );
      *( z + 29*(j+14)+i+14 ) = (float) exp(-r/5.0)*tz*10.0;      
    }
}


static void testfun( int *exclude, float *testval, int *x, int *y )
{
  if (*testval <4.0 )
    *exclude = 0;
  else 
    *exclude = 1;
}


static void create_zdata()
{
  int i, j;
  
  zdata = ( float * ) malloc( 48*( sizeof( float ) ) );
  valdata = ( float * ) malloc( 48*( sizeof( float ) ) );

  for ( i=0; i<6; i++ )
    for ( j=0; j<8; j++ ) {
      *( zdata+8*i+j ) = 5/(34.001-(j+1+(i+1)*8));
      *( valdata+8*i+j ) = *( zdata+8*i+j );
    }
}
      

static void composite_test( VOIDLIST )
{
  ERRCHK( dp_open("composite_gob") );
  ERRCHK( dp_open("") );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_scale(2.0) );
  ERRCHK( dp_gobcolor(&green_color));
  ERRCHK( dp_close() );
  ERRCHK( dp_open("") );
  ERRCHK( dp_boundbox( &corner1, &corner2 ) );
  ERRCHK( dp_gobcolor( &blue_color ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("") );
  ERRCHK( dp_axis( &axis_pt1, &axis_pt2, &axis_up, 0.0, 0.5, 3, 
                   "Bounding Box", 0.25, 2 ) );
  ERRCHK( dp_gobcolor( &red_color ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap( "composite_gob", "standard_lights", "standard_camera" ) );  

  ERRCHK( dp_open("zgob1") );
  ERRCHK( dp_std_cmap( 0.0, 1.0, 4 ) );
  create_zdata();
  ERRCHK( dp_zsurface(P3D_CVNVTX, zdata, valdata, 
		      6, 8, &cor1, &cor2, testfun) );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap( "zgob1", "standard_lights", "standard_camera" ) );  

  ERRCHK( dp_open("zgob2") );
  ERRCHK( dp_std_cmap( -4.0, 5.0, 2 ) );
  create_zsurfdat();
  ERRCHK( dp_zsurface(P3D_CVVTX, z, z, 29, 29, &cor1_14, &cor2_14, 0 ) );
  ERRCHK( dp_boundbox( &cor1_14, &cor2_14 ) );
  ERRCHK( dp_rotate( &text_u, -80.0 ) );
  ERRCHK( dp_scale( 0.6 ) );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap( "zgob2", "standard_lights", "standard_camera" ) );
  free(z);
  free( valdata );
  free( zdata );
}


static void zsurf_test( int *flag, float *x, float *y, float *z, int *i )
/* This function is used to test points for exclusion from the z surface */
{
  if (*i==10) *flag= P3D_TRUE;
}
  
static void rand_zsurf_test() 
{
  static P_Vector up = { 0.0, 0.0, 1.0 };
  static P_Point from = { 200.0, 26.0, 19.9 };
  static P_Point at = { 3.0, 26.0, 0.0 };
  static P_Point p1= { -42.1, -40.1, -19.9 };
  static P_Point p2= { 35.45, 92.1, 10.0 };
  static P_Color blue = { 0, 0.0, 0.0, 1.0, 1.0 };
  static P_Color red = { 0, 1.0, 0.0, 0.0, 1.0 };
  int npts=ZPTS;

  ERRCHK( dp_camera( "this_camera", &from, &at, &up, 53.0, -50.0, -300.0 ) );
  ERRCHK( dp_std_cmap( -15.3, 10.0, 1 ) );
  ERRCHK( dp_open("thisgob") );

  ERRCHK( dp_open("") );
  ERRCHK( dp_rand_zsurf(  P3D_CVVTX, P3D_RGB, data_zsurf, npts, zsurf_test ) );
  ERRCHK( dp_gobcolor( &red ) );
  ERRCHK( dp_close() );

  ERRCHK( dp_open("") );
  ERRCHK( dp_boundbox( &p1, &p2 ) );
  ERRCHK( dp_gobcolor( &blue ) );
  ERRCHK( dp_close() );

  ERRCHK( dp_close() );

  ERRCHK( dp_snap("thisgob","standard_lights","this_camera") );
  
}

static void gen_ran_iso_point( float *x, float *y, float *z, 
			      float *val, float *val2 )
{
  *x= get_random_flt() - 0.5;
  *y= get_random_flt() - 0.5;
  *z= get_random_flt() - 0.5;
  *val= sqrt( ( *x * *x ) + ( *y * *y ) + ( *z * *z ) );
  *val2= *z;
}

static void rand_iso_test()
{
  int npts= RAND_ISOSURF_PTS;
  float data[5*RAND_ISOSURF_PTS], *dataptr= data;
  int i;
  float ival= 0.51;
  static P_Vector up = { 0.0, 1.0, 0.0 };
  static P_Point from = { 0.0, 0.0, 6.0 };
  static P_Point at = { 0.0, 0.0, 0.0 };

  for (i=0; i<npts; i++) 
    gen_ran_iso_point( &data[5*i], &data[5*i+1], &data[5*i+2],
		      &data[5*i+3], &data[5*i+4] );
    
  ERRCHK( dp_camera( "this_camera", &from, &at, &up, 10.0, -1.0, -10.0 ) );

  ERRCHK( dp_std_cmap(-0.5, 0.5, 2) );
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_rand_isosurf( P3D_CVVVTX, P3D_RGB, data, npts, ival, 1 ) );
  ERRCHK( dp_close() );

  ERRCHK( dp_snap("mygob","standard_lights","this_camera") );
}

main()
{
#ifdef never
  ERRCHK( dp_init_ren("myrenderer","painter","cgmb","test3.cgm") );
#endif
  ERRCHK( dp_init_ren("myrenderer","painter","xws","-") );
  ERRCHK( dp_init_ren("myrenderer2","painter","xws","-") );

#ifdef never
  ERRCHK( dp_debug() );
#endif

  ERRCHK( dp_close_ren("myrenderer") );
  ERRCHK( dp_open_ren("myrenderer") );
  ERRCHK( dp_print_ren("myrenderer") );
  ERRCHK( dp_open("mylights") );
  ERRCHK( dp_light(&light_loc, &light_color) );
  ERRCHK( dp_ambient(&ambient_color) );
  ERRCHK( dp_close() );
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );
  ERRCHK( dp_print_camera("mycamera") );
  ERRCHK( dp_std_cmap( 5.0, 9.0, 0 ) );

  simple_prim_test();

  vlist_prim_test( P3D_CVTX, P3D_RGB, cvtx_coords, CVTX_LENGTH );
  vlist_prim_test( P3D_CCVTX, P3D_RGB, ccvtx_coords, CCVTX_LENGTH );
  vlist_prim_test( P3D_CCNVTX, P3D_RGB, ccnvtx_coords, CCNVTX_LENGTH);
  vlist_prim_test( P3D_CNVTX, P3D_RGB, cnvtx_coords, CNVTX_LENGTH );
  vlist_prim_test( P3D_CVVTX, P3D_RGB, cvvtx_coords, CVVTX_LENGTH );
  vlist_prim_test( P3D_CVNVTX, P3D_RGB, cvnvtx_coords, CVNVTX_LENGTH );

  mesh_test( P3D_CVTX, P3D_RGB, m_cvtx_coords, M_CVTX_LENGTH );
  mesh_test( P3D_CCVTX, P3D_RGB, m_ccvtx_coords, M_CCVTX_LENGTH );
  mesh_test( P3D_CCNVTX, P3D_RGB, m_ccnvtx_coords, M_CCNVTX_LENGTH );
  mesh_test( P3D_CNVTX, P3D_RGB, m_cnvtx_coords, M_CNVTX_LENGTH );
  mesh_test( P3D_CVVTX, P3D_RGB, m_cvvtx_coords, M_CVVTX_LENGTH );
  mesh_test( P3D_CVNVTX, P3D_RGB, m_cvnvtx_coords, M_CVNVTX_LENGTH );

  bezier_test( P3D_CVTX, P3D_RGB, b_cvtx_coords );
  bezier_test( P3D_CCVTX, P3D_RGB, b_ccvtx_coords );
  bezier_test( P3D_CCNVTX, P3D_RGB, b_ccnvtx_coords );
  bezier_test( P3D_CNVTX, P3D_RGB, b_cnvtx_coords );
  bezier_test( P3D_CVVTX, P3D_RGB, b_cvvtx_coords );
  bezier_test( P3D_CVNVTX, P3D_RGB, b_cvnvtx_coords );

  cmap_test();

  std_cmap_test(0);
  std_cmap_test(1);
  std_cmap_test(2);
  std_cmap_test(3);
  std_cmap_test(4);

  empty_gob_test();

  scaling_test();

  isosurf_test();

  composite_test();

  rand_zsurf_test();

  rand_iso_test();

  /* Test camera replacement */
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea/2,hither,yon) );

  ERRCHK( dp_free("mygob") );
  ERRCHK( dp_free("mylights") );

  ERRCHK( dp_close_ren("myrenderer") );
  ERRCHK( dp_shutdown() );
}

