/****************************************************************************
 * tube_molecules.c
 * Author Joel Welling
 * Copyright 1999, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
/* This is a test program for the tube molecule generator. */

#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>
#include "pgen_objects.h" /* because we must access vertex list methods */
#include "ge_error.h"

static int n_mol_coords= 123;

static float mol_coords[]= {
    12.856, 16.686, 32.935, 0.96,  0.87,  0.70, 1.0,
    10.382, 15.331, 30.396, 0.53,  0.81,  0.92, 1.0,
     8.056, 18.355, 30.328, 0.53,  0.81,  0.92, 1.0,
    10.892, 20.697, 29.229, 0.53,  0.81,  0.92, 1.0,
    12.322, 18.199, 26.691, 0.53,  0.81,  0.92, 1.0,
     8.947, 17.658, 25.007, 0.53,  0.81,  0.92, 1.0,
     8.614, 21.495, 24.872, 0.53,  0.81,  0.92, 1.0,
    12.034, 21.830, 23.208, 0.53,  0.81,  0.92, 1.0,
    11.154, 19.281, 20.477, 0.53,  0.81,  0.92, 1.0,
     7.919, 21.205, 19.825, 0.53,  0.81,  0.92, 1.0,
     9.946, 24.434, 19.651, 0.53,  0.81,  0.92, 1.0,
    12.106, 22.986, 16.832, 0.53,  0.81,  0.92, 1.0,
     9.391, 20.830, 15.167, 0.96,  0.87,  0.70, 1.0,
     6.135, 22.713, 15.743, 0.96,  0.87,  0.70, 1.0,
     4.100, 20.173, 13.771, 0.96,  0.87,  0.70, 1.0,
     4.991, 17.308, 16.155, 0.96,  0.87,  0.70, 1.0,
     3.413, 15.744, 19.214, 0.96,  0.87,  0.70, 1.0,
     6.685, 14.342, 20.591, 0.53,  0.81,  0.92, 1.0,
     5.224, 11.816, 23.027, 0.53,  0.81,  0.92, 1.0,
     3.417, 10.183, 20.133, 0.53,  0.81,  0.92, 1.0,
     5.947, 10.721, 17.294, 0.53,  0.81,  0.92, 1.0,
     9.359, 10.225, 19.064, 0.53,  0.81,  0.92, 1.0,
     8.590, 7.704, 21.713, 0.96,  0.87,   0.70, 1.0,
     9.416, 4.139, 20.837, 0.96,  0.87,   0.70, 1.0,
    11.269, 4.773, 17.294, 0.96,  0.87,   0.70, 1.0,
    14.269, 2.650, 19.064, 0.96,  0.87,   0.70, 1.0,
    16.555, 1.074, 21.713, 0.96,  0.87,   0.70, 1.0,
    17.250, 4.166, 20.837, 0.96,  0.87,   0.70, 1.0,
    14.308, 6.590, 17.564, 0.96,  0.87,   0.70, 1.0,
    12.376, 5.553, 16.563, 0.96,  0.87,   0.70, 1.0,
    13.113, 3.558, 19.127, 0.96,  0.87,   0.70, 1.0,
    16.460, 1.832, 21.231, 0.96,  0.87,   0.70, 1.0,
    18.582, 1.708, 21.179, 0.96,  0.87,   0.70, 1.0,
    20.907, -1.052, 22.845, 0.96,  0.87,  0.70, 1.0,
    22.366, -2.408, 19.645, 0.96,  0.87,  0.70, 1.0,
    23.187, -.508, 16.480, 0.96,   0.87,   0.70, 1.0,
    20.967, 2.523, 15.838, 0.96,   0.87,   0.70, 1.0,
    18.732, 1.877, 12.829, 0.96,   0.87,   0.70, 1.0,
    18.979, 5.382, 11.313, 0.96,   0.87,   0.70, 1.0,
    20.132, 8.951, 11.922, 0.53,   0.81,   0.92, 1.0,
    17.134, 9.804, 14.083, 0.53,   0.81,   0.92, 1.0,
    17.978, 6.700, 16.218, 0.53,   0.81,   0.92, 1.0,
    21.605, 7.980, 16.422, 0.53,   0.81,   0.92, 1.0,
    20.193, 11.280, 17.741, 0.53,  0.81,  0.92, 1.0,
    18.545, 9.301, 20.505, 0.53,   0.81,   0.92, 1.0,
    21.665, 7.221, 21.089, 0.53,   0.81,   0.92, 1.0,
    23.724, 10.403, 21.569, 0.53,  0.81,  0.92, 1.0,
    21.032, 11.750, 23.903, 0.53,  0.81,  0.92, 1.0,
    21.013, 8.570, 26.015, 0.53,   0.81,   0.92, 1.0,
    24.837, 8.775, 26.280, 0.53,   0.81,   0.92, 1.0,
    24.531, 12.445, 27.292, 0.53,  0.81,  0.92, 1.0,
    22.349, 11.498, 30.287, 0.53,  0.81,  0.92, 1.0,
    24.816, 8.779, 31.409,  0.53,   0.81,   0.92, 1.0,
    27.565, 11.336, 31.224, 0.53,  0.81,  0.92, 1.0,
    25.635, 13.889, 33.247, 0.53,  0.81,  0.92, 1.0,
    25.381, 11.307, 35.966, 0.53,  0.81,  0.92, 1.0,
    29.181, 11.310, 36.289, 0.53,  0.81,  0.92, 1.0,
    29.446, 15.110, 36.775, 0.96,  0.87,  0.70, 1.0,
    30.569, 16.065, 40.302, 0.53,  0.81,  0.92, 1.0,
    28.020, 18.880, 40.244, 0.53,  0.81,  0.92, 1.0,
    25.153, 16.515, 39.509, 0.53,  0.81,  0.92, 1.0,
    26.330, 13.891, 41.994, 0.53,  0.81,  0.92, 1.0,
    26.631, 16.279, 44.926, 0.53,  0.81,  0.92, 1.0,
    23.170, 17.694, 44.062, 0.96,  0.87,  0.70, 1.0,
    21.931, 14.024, 43.945, 0.96,  0.87,  0.70, 1.0,
    20.338, 14.789, 40.617, 0.96,  0.87,  0.70, 1.0,
    18.360, 12.636, 38.222, 0.96,  0.87,  0.70, 1.0,
    18.571, 13.808, 34.552, 0.96,  0.87,  0.70, 1.0,
    15.032, 12.512, 34.098, 0.96,  0.87,  0.70, 1.0,
    13.752, 14.871, 36.853, 0.96,  0.87,  0.70, 1.0,
    16.180, 17.796, 36.351, 0.96,  0.87,  0.70, 1.0,
    14.381, 21.121, 36.663, 0.96,  0.87,  0.70, 1.0,
    16.482, 23.379, 34.363, 0.96,  0.87,  0.70, 1.0,
    14.918, 26.408, 32.666, 0.56,  0.37,  0.60, 1.0,
    14.752, 27.049, 28.931, 0.56,  0.37,  0.60, 1.0,
    12.970, 29.402, 26.530, 0.56,  0.37,  0.60, 1.0,
    11.639, 29.073, 22.979, 0.56,  0.37,  0.60, 1.0,
    11.289, 32.037, 20.623, 0.56,  0.37,  0.60, 1.0,
    11.168, 31.921, 16.836, 0.96,  0.87,  0.70, 1.0,
    11.775, 28.149, 16.946, 0.96,  0.87,  0.70, 1.0,
    15.102, 28.637, 18.718, 0.56,  0.37,  0.60, 1.0,
    15.897, 27.138, 22.090, 0.56,  0.37,  0.60, 1.0,
    17.884, 28.942, 24.717, 0.56,  0.37,  0.60, 1.0,
    18.967, 27.266, 27.924, 0.56,  0.37,  0.60, 1.0,
    18.805, 29.718, 30.824, 0.56,  0.37,  0.60, 1.0,
    21.922, 31.414, 32.083, 0.96,  0.87,  0.70, 1.0,
    20.518, 31.015, 35.563, 0.96,  0.87,  0.70, 1.0,
    20.964, 27.211, 35.285, 0.96,  0.87,  0.70, 1.0,
    23.567, 25.696, 37.612, 0.96,  0.87,  0.70, 1.0,
    26.375, 23.593, 36.181, 0.53,  0.81,  0.92, 1.0,
    24.418, 20.293, 36.310, 0.53,  0.81,  0.92, 1.0,
    21.117, 21.760, 35.115, 0.53,  0.81,  0.92, 1.0,
    22.978, 23.471, 32.334, 0.53,  0.81,  0.92, 1.0,
    24.578, 20.185, 31.245, 0.53,  0.81,  0.92, 1.0,
    21.131, 18.425, 31.075, 0.53,  0.81,  0.92, 1.0,
    19.558, 21.328, 29.224, 0.53,  0.81,  0.92, 1.0,
    22.295, 21.156, 26.574, 0.53,  0.81,  0.92, 1.0,
    21.805, 17.353, 26.183, 0.53,  0.81,  0.92, 1.0,
    18.081, 18.050, 25.568, 0.53,  0.81,  0.92, 1.0,
    18.744, 21.036, 23.280, 0.53,  0.81,  0.92, 1.0,
    21.249, 19.143, 21.097, 0.53,  0.81,  0.92, 1.0,
    18.820, 16.231, 20.695, 0.53,  0.81,  0.92, 1.0,
    15.889, 18.457, 19.762, 0.53,  0.81,  0.92, 1.0,
    18.000, 20.133, 17.081, 0.53,  0.81,  0.92, 1.0,
    19.252, 16.761, 15.898, 0.53,  0.81,  0.92, 1.0,
    15.661, 15.473, 15.640, 0.53,  0.81,  0.92, 1.0,
    14.783, 18.475, 13.492, 0.53,  0.81,  0.92, 1.0,
    17.657, 17.916, 11.062, 0.96,  0.87,  0.70, 1.0,
    17.228, 14.234, 10.180, 0.96,  0.87,  0.70, 1.0,
    14.737, 12.203, 8.188,  0.96,   0.87,   0.70, 1.0,
    11.911, 10.315, 9.942,  0.96,   0.87,   0.70, 1.0,
    11.506, 6.699, 8.752,  0.96,   0.87,   0.70, 1.0,
     8.093, 5.552, 9.903,  0.96,   0.87,   0.70, 1.0,
     9.077, 1.962, 9.281,  0.96,   0.87,   0.70, 1.0,
    11.371, 2.296, 12.310, 0.96,   0.87,   0.70, 1.0,
     8.591, 3.282, 14.684, 0.96,   0.87,   0.70, 1.0,
     7.770,  .612, 17.301, 0.96,   0.87,   0.70, 1.0,
    10.671, -1.615, 16.216, 0.96,  0.87,  0.70, 1.0,
    10.868, -5.120, 17.697, 0.96,  0.87,  0.70, 1.0,
    13.872, -4.918, 20.073, 0.96,  0.87,  0.70, 1.0,
    14.963, -8.204, 18.735, 0.96,  0.87,  0.70, 1.0,
    16.473, -6.186, 15.874, 0.96,  0.87,  0.70, 1.0,
    18.637, -4.346, 18.436, 0.96,  0.87,  0.70, 1.0
};

static int mol_types[]= {
  3,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  3,
  3,
  3,
  3,
  3,
  1,
  1,
  1,
  1,
  1,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  3,
  1,
  1,
  1,
  1,
  1,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  2,
  2,
  2,
  2,
  2,
  3,
  3,
  2,
  2,
  2,
  2,
  2,
  3,
  3,
  3,
  3,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3,
  3
};

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Camera information */
static P_Point lookfrom= { -20.0, 10.0, 10.0 };
static P_Point lookat= { 10.0, 10.0, 10.0 };
static P_Vector up= { 0.0, 0.0, 1.0 };
static float fovea= 40.0;
static float hither= -5.0;
static float yon= -50.0;

/* This routine actually creates the height field or z surface. */
static void create_tube_mol()
{
  int pg_spline_tube(P_Vlist*,int*,int,int);
  P_Vlist* vlist;

  /* This sets the color map which will be used to map the value data
   * to colors on the z surface.  We will be using the surface values
   * themselves, which give the height of the surface at any given
   * point, to color the surface.
   */
  ERRCHK( dp_std_cmap( -1.0, 1.0, 2 ) );

  /* Open a GOB to contain the surface and axis */
  ERRCHK( dp_open("tube_mol_gob") );

  vlist = po_create_cvlist( P3D_CCVTX, n_mol_coords, mol_coords );


  ERRCHK( dp_spline_tube( P3D_CCVTX, P3D_RGB, mol_coords, n_mol_coords, 
			  mol_types, 6, 20 ) );

  /* Close the GOB "tube_mol_gob". */
  ERRCHK( dp_close() );
}

main()
/* This is the main routine. */
{
  /* Initialize the renderer. */
#ifdef never
  ERRCHK( dp_init_ren("myrenderer","vrml","test.wrl", "") );
  ERRCHK( dp_init_ren("myrenderer","p3d","test.p3d", "") );
#endif
  ERRCHK( dp_init_ren("myrenderer","iv","test.iv", "") );

  /* Create a camera, according to specs given in definitions above. */
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );

  /* Create a tube rep of the molecule */
  create_tube_mol();

  /* Cause the GOB to be rendered. */
  ERRCHK( dp_snap( "tube_mol_gob", "standard_lights", "mycamera" ) );

  /* Shut down the DrawP3D software. */
  ERRCHK( dp_shutdown() );
}
