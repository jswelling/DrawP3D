/* This example uses the dp_isosurface function to produce an iso-valued
 * surface from 3D gridded data.  A bounding box shows the bounds of the
 * computational region.
 */

#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Camera information */
static P_Point lookfrom= { 1.0, 2.0, 17.0 };
static P_Point lookat= { 1.0, 2.0, -3.0 };
static P_Vector up= { 0.0, 1.0, 0.0 };
static float fovea= 45.0;
static float hither= -5.0;
static float yon= -30.0;

/* Light source information */
static P_Point light_loc= {0.0, 1.0, 10.0};
static P_Color light_color= { P3D_RGB, 0.8, 0.8, 0.8, 0.3 };
static P_Color ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 0.8 };

/* Dimensions of the grid from which the isosurface is to be extracted */
#define ISO_NX 14
#define ISO_NY 15
#define ISO_NZ 16

/* Axis about which to rotate */
static P_Vector y_axis= { 0.0, 1.0, 0.0 };

/* Number of rotation steps, and angle between steps */
static int nsteps= 200;
static float dtheta= 30.0;

static void calc_iso_data( float data[ISO_NX][ISO_NY][ISO_NZ] )
/* This function provides the values of which an iso-valued surface is
 * found.
 */
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
/* This function supplies data with which to color the surface */
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

static void create_isosurf()
/* This function generates two grids of data, and then produces an
 * isosurface of one colored by the other.
 */
{
  P_Point corner1, corner2;
  float data[ISO_NX][ISO_NY][ISO_NZ];
  float valdata[ISO_NX][ISO_NY][ISO_NZ];
  float val= 0.5;
  int inner_surface= 1;

  /* Set the corners of the region in which to draw the isosurface 
   * and bounding box.
   */
  corner1.x= -2.6;
  corner1.y= -2.5;
  corner1.z= 3.75;
  corner2.x= 4.6;
  corner2.y= 6.5;
  corner2.z= -9.75;

  /* Calculate the data to contour, and the values with which to color it */
  calc_iso_data(data);
  calc_iso_valdata(valdata);

  /* This sets the color map which will be used to map the valdata values
   * to colors on the isosurface.
   */
  ERRCHK( dp_std_cmap(0.0, 1.0, 1) );

  /* Open a GOB into which to put the isosurface and bounding box */
  ERRCHK( dp_open("mygob") );

  /* This actually generates the isosurface GOB.  See the documentation
   * for an explaination of the parameters.
   */
  ERRCHK( dp_isosurface( P3D_CVNVTX, (float *)data, (float *)valdata, 
			ISO_NX, ISO_NY, ISO_NZ, val, 
			&corner1, &corner2, inner_surface) );

  /* This adds a bounding box, to show the boundaries of the grid of
   * data from which the isosurface is extracted.
   */
  ERRCHK( pg_boundbox( &corner1, &corner2 ) );

  /* Close the GOB which holds the new objects. */
  ERRCHK( dp_close() );
}

static void make_snaps()
/* This routine takes the water molecule and makes a series of frames
 * as the atom is rotated about its axis of symmetry.  The creation
 * of each new "mygob" GOB causes the garbage collection of the last,
 * but the GOB "h2o" survives because it has a name.
 */
{
  float theta= 0.0;
  int step;

  for (step=0; step<nsteps; step++) {

    /* Create a rotated model */
    ERRCHK( dp_open("myparent") );
    ERRCHK( dp_rotate( &y_axis, theta ) );
    ERRCHK( dp_child("mygob") );
    ERRCHK( dp_close() );

    /* Snap and go on to the next model */
    ERRCHK( dp_snap("myparent","mylights","mycamera") );
    theta += dtheta;
  }
}

main()
{
  /* Initialize the renderer */
  ERRCHK( dp_init_ren("myrenderer","painter","xws","-") );

  /* Generate a light source and camera */
  ERRCHK( dp_open("mylights") );
  ERRCHK( dp_light(&light_loc, &light_color) );
  ERRCHK( dp_ambient(&ambient_color) );
  ERRCHK( dp_close() );
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );

  /* This function actually generates the isosurface GOB, and a bounding
   * box to show the limits of the grid from which the surface is extracted.
   */
  create_isosurf();

  make_snaps();
#ifdef never
  /* Cause the isosurface and bounding box to be rendered */
  ERRCHK( dp_snap("mygob","mylights","mycamera") );
#endif

  /* Close DrawP3D */
  ERRCHK( dp_shutdown() );
}

