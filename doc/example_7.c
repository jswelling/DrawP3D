/* This example uses the dp_rand_zsurf function to produce a height field
 * or z surface from data points at random 2D locations.  The dp_boundbox
 * routine is used to provide bounds for the surface.
 */

#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* These are the data points (x, y, z coordinates and values) from
 * which the surface will be constructed.
 */
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

/* Those data values for which *flag is set to P3D_TRUE will be excluded
 * from the z surface, leaving holes.
 */
static void zsurf_test( int *flag, float *x, float *y, float *z, int *i )
/* This function is used to test points for exclusion from the z surface */
{
  if (*i==10) *flag= P3D_TRUE;
}

/* This routine actually creates the z surface. */  
static void rand_zsurf_demo() 
{
  int npts=ZPTS;

  /* Corners for the bounding box, and a color */
  P_Point p1= { -42.1, -40.1, -19.9 };
  P_Point p2= { 35.45, 92.1, 10.0 };
  P_Color blue = { 0, 0.0, 0.0, 1.0, 1.0 };

  /* This sets the color map which will be used to map the value data
   * to colors on the z surface.
   */
  ERRCHK( dp_std_cmap( -15.3, 10.0, 1 ) );

  /* Open a GOB to contain the z surface and bounding box. */
  ERRCHK( dp_open("thisgob") );

  /* Add a the z surface.  See the reference manual for details on the
   * calling parameters.
   */
  ERRCHK( dp_rand_zsurf(  P3D_CVVTX, P3D_RGB, data_zsurf, npts, zsurf_test ) );

  /* Add a bounding box, with an appropriate color. */
  ERRCHK( dp_open("") );
  ERRCHK( dp_boundbox( &p1, &p2 ) );
  ERRCHK( dp_gobcolor( &blue ) );
  ERRCHK( dp_close() );

  /* Close the GOB "thisgob". */
  ERRCHK( dp_close() );

}

/* This is the main routine. */
main()
{
  /* Camera viewing information */
  P_Vector up = { 0.0, 0.0, 1.0 };
  P_Point from = { 200.0, 26.0, 19.9 };
  P_Point at = { 3.0, 26.0, 0.0 };

  /* Initialize the renderer */
  ERRCHK( dp_init_ren("myrenderer","p3d","example_7.p3d",
		      "I don't care about this string") );

  /* Define a camera, using the viewing information above. */
  ERRCHK( dp_camera("mycamera", &from, &at, &up, 53.0, -50.0, -300.0 ) );

  /* Generate the GOB "thisgob", containing a random z surface and
   * a bounding box.
   */
  rand_zsurf_demo();

  /* Cause the GOB to be rendered. */
  ERRCHK( dp_snap("thisgob","standard_lights","mycamera") );
  
  /* Shut down the DrawP3D software. */
  ERRCHK( dp_shutdown() );
}

