/* This example uses the dp_zsurface function to produce a height field
 * or z surface from a 2D array of data.  The dp_axis and dp_boundbox
 * routines are used to provide an axis and bounds for the surface.
 */
#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Information giving the corners of the bounding box and ends of the axis */
static P_Point corner1 = { -2.0, -2.0, -1.5 };
static P_Point corner2 = { 2.0, 2.0, 1.5 };
static P_Point axis_pt1 = { -2.0, -2.1, -2.0 };
static P_Point axis_pt2 = { 2.0, -2.1, -2.0 };
static P_Vector axis_up = { 0.0, 0.0, 1.0 };

/* Camera information */
static P_Point lookfrom= { 0.0, -10.0, 2.0 };
static P_Point lookat= { 0.0, 0.0, 0.0 };
static P_Vector up= { 0.0, 0.0, 1.0 };
static float fovea= 45.0;
static float hither= -5.0;
static float yon= -20.0;

/* Colors to be used for the bounding box and axis */
static P_Color red_color= { P3D_RGB, 1.0, 0.0, 0.0, 1.0 };
static P_Color blue_color= { P3D_RGB, 0.0, 0.0, 1.0, 1.0 };

/* This function provides data to use to produce the z surface. */
static float *create_zsurfdat( z )
float z[29][31];
{
  float r, x, y, tz;
  int i, j;

  for (i=0; i<29; i++) {
    x= (float)(i-14);
    for (j=0; j<31; j++) {
      y= (float)(j-15);
      r= sqrt( x*x + y*y );
      tz= cos(r);
      z[i][j]= 2.0*exp(-r/5.0)*tz;
    }
  }
}

/* Those data values for which *exclude is set to 1 will be excluded
 * from the z surface, leaving holes.
 */
static void testfun( int *exclude, float *testval, int *x, int *y )
{
  if (*testval < 1.9 )
    *exclude = 0;
  else 
    *exclude = 1;
}

/* This routine actually creates the height field or z surface. */
static void create_surf()
{
  float z[29][31];

  create_zsurfdat(z);

  /* This sets the color map which will be used to map the value data
   * to colors on the z surface.  We will be using the surface values
   * themselves, which give the height of the surface at any given
   * point, to color the surface.
   */
  ERRCHK( dp_std_cmap( -2.0, 2.0, 2 ) );

  /* Open a GOB to contain the surface, bounding box, and axis */
  ERRCHK( dp_open("zsurf_gob") );

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
  ERRCHK( dp_axis( &axis_pt1, &axis_pt2, &axis_up, -2.0, 2.0, 3, 
                   "Sample Z surface", 0.25, 2 ) );
  ERRCHK( dp_close() );

  /* Create the z surface, inserting it into the GOB "zsurf_gob".  The
   * values in z are used both to provide the heights for the surface
   * and to provide values with which to color it.  testfun provides
   * a function which can be used to exclude points from the surface;
   * it could be left null if all points were to be included.
   */
  ERRCHK( dp_zsurface(P3D_CVVTX, (float *)z, (float *)z, 29, 31, 
		      &corner1, &corner2, testfun ) );

  /* Close the GOB "zsurf_gob". */
  ERRCHK( dp_close() );
}

main()
/* This is the main routine. */
{
  /* Initialize the renderer. */
  ERRCHK( dp_init_ren("myrenderer","p3d","example_6.p3d",
		      "I don't care about this string") );

  /* Create a camera, according to specs given in definitions above. */
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );

  /* Create a GOB called "zsurf_gob", containing a z surface, a bounding
   * box, and an axis.
   */
  create_surf();

  /* Cause the GOB to be rendered. */
  ERRCHK( dp_snap( "zsurf_gob", "standard_lights", "mycamera" ) );

  /* Shut down the DrawP3D software. */
  ERRCHK( dp_shutdown() );
}

