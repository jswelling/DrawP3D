/* This example uses the dp_rand_isosurf function to produce an isosurface
 * from data points at random 3D locations.  The dp_boundbox routine is 
 * used to provide bounds for the surface.
 */

#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Number of data points to define the isosurface */
#define RAND_ISOSURF_PTS 1000

/* This function returns a float between 0.0 and 1.0. */
static float get_random_flt()
{
  float result;

  result= (float)(0.5*(double)random()/(double)(2<<30-1));

  return(result);
}

/* This function generates a single randomly located data point.  The
 * val field provides the data values from which the isosurface is
 * extracted, and the val2 field provides values mapped to colors
 * on the surface.
 */
static void gen_ran_iso_point( float *x, float *y, float *z, 
			      float *val, float *val2 )
{
  *x= get_random_flt() - 0.5;
  *y= get_random_flt() - 0.5;
  *z= get_random_flt() - 0.5;
  *val= sqrt( ( *x * *x ) + ( *y * *y ) + ( *z * *z ) );
  *val2= *z;
}

/* This routine actually creates the isosurface. */
static void rand_iso_demo()
{
  int npts= RAND_ISOSURF_PTS;
  float data[5*RAND_ISOSURF_PTS], *dataptr= data;
  int i;
  float ival= 0.48; /* This is the constant value of the isosurface */

  /* Corners for the bounding box, and a color */
  P_Point p1= { -0.5, -0.5, -0.5 };
  P_Point p2= { 0.5, 0.5, 0.5 };
  P_Color blue = { 0, 0.0, 0.0, 1.0, 1.0 };

  /* Generate the data */
  for (i=0; i<npts; i++) 
    gen_ran_iso_point( &data[5*i], &data[5*i+1], &data[5*i+2],
		      &data[5*i+3], &data[5*i+4] );

  /* This sets the color map which will be used to map the value data 
   * to colors on the z surface.
   */
  ERRCHK( dp_std_cmap(-0.5, 0.5, 2) );

  /* Open a GOB to contain the z surface and bounding box. */
  ERRCHK( dp_open("mygob") );

  /* Generate and add the isosurface.  See the reference manual for details
   * on the calling parameters.
   */
  ERRCHK( dp_rand_isosurf( P3D_CVVVTX, P3D_RGB, data, npts, ival, 1 ) );

  /* Add a bounding box, with an appropriate color. */
  ERRCHK( dp_open("") );
  ERRCHK( dp_boundbox( &p1, &p2 ) );
  ERRCHK( dp_gobcolor( &blue ) );
  ERRCHK( dp_close() );

  /* Close the GOB "mygob". */
  ERRCHK( dp_close() );

}

/* This is the main routine. */
main()
{
  /* Camera specificatin information */
  P_Vector up = { 0.0, 1.0, 0.0 };
  P_Point from = { 0.0, 0.0, 8.0 };
  P_Point at = { 0.0, 0.0, 0.0 };

  /* Initialize the renderer. */
  ERRCHK( dp_init_ren("myrenderer","p3d","example_8.p3d",
		      "I don't care about this string") );

  /* Create a camera, using the information above. */
  ERRCHK( dp_camera( "this_camera", &from, &at, &up, 10.0, -1.0, -10.0 ) );

  /* Generate a GOB called "mygob", with the isosurface and bounding box. */
  rand_iso_demo();

  /* Cause the GOB "mygob" to be rendered. */
  ERRCHK( dp_snap("mygob","standard_lights","this_camera") );

  /* Shut down the DrawP3D software. */
  ERRCHK( dp_shutdown() );
}

