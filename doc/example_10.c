/* Example 10: 
 * This example uses the dp_irreg_zsurf function to produce a height 
 * field or z surface from a 2D array of data in polar coordinates.
 * The dp_axis and dp_boundbox routines are used to provide an axis
 * and bounds for the surface.  Note that the bounds are not really
 * accurate, since the data is regular in polar coordinates and
 * the bounding box is drawn in Cartesian coordinates.
 *
 * Author: Silvia Olabarriaga (silvia@inf.ufrgs.br)
 * March 24th 1993
*/

#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Information giving the corners of the bounding box and ends of the axis */
static P_Point corner1 = { -1.0, -1.0, -1.0 };
static P_Point corner2 = { 1.0, 1.0, 1.0 };
static P_Point axis_pt1 = { -1.0, -1.0, -1.0 };
static P_Point axis_pt2 = { 1.0, -1.0, -1.0 };
static P_Vector axis_up = { 0.0, 0.0, 1.0 };

/* Camera information */
static P_Point lookfrom= { 0.0, -10.0, 3.0 };
static P_Point lookat= { 0.0, 0.0, -0.5 };
static P_Vector up= { 0.0, 0.0, 1.0 };
static float fovea= 20.0;
static float hither= -5.0;
static float yon= -20.0;

/* Colors to be used for the bounding box and axis */
static P_Color red_color= { P3D_RGB, 1.0, 0.1, 0.1, 1.0 };
static P_Color blue_color= { P3D_RGB, 0.1, 0.1, 1.0, 1.0 };

/* step to generate grid points */
float max_rad;
float delta_rad;
float delta_ang;

/* Grid information */
#define NX 31
#define NY 29
#define MIN_RAD   0.01
#define MAX_RAD   1.0
#define MAX_ANGLE (2 * M_PI)

/* This function provides data to use to produce the z surface. */
static void create_zdat( z )
float z[NX][NY];
{
  float rad, ang;
  int i, j;

  for (i=0; i < NX; i++) {
    rad = i * delta_rad + MIN_RAD;
    for (j=0; j < NY; j++) {
      ang = j * delta_ang;
      z[i][j]= rad * sin(ang) * cos(ang); 
    }
  }
}

/* this function returns cartesian (x,y) values from polar
   coordinates on a regular grid.
   i = row (radius) and j = column (theta) 
*/
static void xyfun ( float *x, float *y, int *i, int *j )
{
  float rad= *i * delta_rad + MIN_RAD;
  float theta= *j * delta_ang;
  *x= rad * cos(theta);
  *y= rad * sin(theta);
}

/* Those data values for which *exclude is set to 1 will be excluded
 * from the z surface, leaving holes.  We exclude a band of constant
 * j (angle).
 */
static void testfun( int *exclude, float *testval, int *i, int *j )
{
  if (*j == NY/2) *exclude= 1;
  else *exclude = 0;
}

/* This routine actually creates the height field or z surface. */
static void create_irreg_zsurf()
{
  float z[NX][NY];

  /* This sets the color map which will be used to map the value data
   * to colors on the z surface.  We will be using the surface values
   * themselves, which give the height of the surface at any given
   * point, to color the surface.
   */
  ERRCHK( dp_std_cmap( -1.0, 1.0, 2 ) );

  /* Open a GOB to contain the surface and axis */
  ERRCHK( dp_open("irreg_zsurf_gob") );

  /* Insert a bounding box, with an associated color. */
  ERRCHK( dp_open("") );
  ERRCHK( dp_gobcolor( &blue_color ) );
  ERRCHK( dp_boundbox( &corner1, &corner2 ) );
  ERRCHK( dp_close() );

  /* Insert an axis, with an associated color.  See the reference manual
   * for details on the calling parameters.  
   */
  ERRCHK( dp_open("") );
  ERRCHK( dp_gobcolor( &red_color ) );
  ERRCHK( dp_axis( &axis_pt1, &axis_pt2, &axis_up, -1.0, 1.0, 3, 
                   "Polar Coords", 0.15, 1 ) );
  ERRCHK( dp_close() );

  /* Create the z surface, inserting it into the GOB "irreg_zsurf_gob".  The
   * values in z are used both to provide the heights for the surface
   * and to provide values with which to color it.  testfun provides
   * a function which can be used to exclude points from the surface;
   * it could be left null if all points were to be included.
   * xyfun is a function that provides x,y values for the grid points
   */

					/* init values for xyfunc */
  delta_rad = MAX_RAD / ( NX - 1 );
  delta_ang = MAX_ANGLE / ( NY - 1.0 );
					/* init z values array */
  create_zdat(z);

  ERRCHK( dp_irreg_zsurf(P3D_CVVTX, (float *)z, (float *)z, NX, NY, 
			 xyfun, testfun ) );

  /* Close the GOB "irreg_zsurf_gob". */
  ERRCHK( dp_close() );
}

main()
/* This is the main routine. */
{
  /* Initialize the renderer. */
  ERRCHK( dp_init_ren("myrenderer","p3d","example_10.p3d", 
		      "don't care") );

  /* Create a camera, according to specs given in definitions above. */
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );

  /* Create a GOB called "irreg_zsurf_gob", containing a z surface, a bounding
   * box, and an axis.
   */
  create_irreg_zsurf();

  /* Cause the GOB to be rendered. */
  ERRCHK( dp_snap( "irreg_zsurf_gob", "standard_lights", "mycamera" ) );

  /* Shut down the DrawP3D software. */
  ERRCHK( dp_shutdown() );
}

