/* This example uses the dp_irreg_isosurf function to produce an iso-valued
 * surface from data on an irregular 3D grid (in this case in spherical
 * polar coordinates).
 */

#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Camera information */
static P_Point lookfrom= { 30.0, 0.0, 0.0 };
static P_Point lookat= { 0.0, 0.0, 0.0 };
static P_Vector up= { 0.0, 0.0, 1.0 };
static float fovea= 45.0;
static float hither= -10.0;
static float yon= -60.0;

/* Light source information */
static P_Point light_loc= {30.0, 0.0, 20.0};
static P_Color light_color= { P3D_RGB, 0.8, 0.8, 0.8, 0.3 };
static P_Color ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 0.8 };

/* Dimensions of the grid from which the isosurface is to be extracted */
#define ISO_NX 19
#define ISO_NY 20
#define ISO_NZ 21

static void calc_iso_data( float data[ISO_NX][ISO_NY][ISO_NZ] )
/* This function provides the values of which an iso-valued surface is
 * found.
 */
{
  int i, j, k;
  float r, theta, phi;

  for (i=0; i<ISO_NX; i++) {
    r= 10.0 * (float)i/(float)(ISO_NX-1);
    for (j=0; j<ISO_NY; j++) {
      theta= ((float)j/(float)(ISO_NY-1)) * 3.14159265;
      for (k=0; k<ISO_NZ; k++) {
	phi= ((float)k/(float)(ISO_NZ-1)) * 2.0 * 3.14159265;
	data[i][j][k]= r * r * exp( -r ) * (3*cos(theta)*cos(theta) - 1.0);
      }
    }
  }
}

static void calc_iso_valdata( float data[ISO_NX][ISO_NY][ISO_NZ] )
/* This function supplies data with which to color the surface */
{
  int i, j, k;
  float r, theta, phi;

  for (i=0; i<ISO_NX; i++) {
    r= 10.0 * (float)i/(float)(ISO_NX-1);
    for (j=0; j<ISO_NY; j++) {
      theta= ((float)j/(float)(ISO_NY-1)) * 3.14159265;
      for (k=0; k<ISO_NZ; k++) {
	phi= ((float)k/(float)(ISO_NZ-1)) * 2.0 * 3.14159265;
	data[i][j][k]= sin(phi);
      }
    }
  }
}

static void coordfun( float *x, float *y, float *z, int *i, int *j, int *k )
{
  float r, theta, phi;

  r= 10.0 * (float)(*i)/(float)(ISO_NX-1);
  theta= ((float)(*j)/(float)(ISO_NY-1)) * 3.14159265;
  phi= ((float)(*k)/(float)(ISO_NZ-1)) * 2.0 * 3.14159265;

  *x= r*sin(theta)*cos(phi);
  *y= r*sin(theta)*sin(phi);
  *z= r*cos(theta);
}

static void create_isosurf()
/* This function generates two grids of data, and then produces an
 * isosurface of one colored by the other.
 */
{
  float data[ISO_NX][ISO_NY][ISO_NZ];
  float valdata[ISO_NX][ISO_NY][ISO_NZ];
  float val= 0.1;
  int inner_surface= 0;

  /* Calculate the data to contour, and the values with which to color it */
  calc_iso_data(data);
  calc_iso_valdata(valdata);

  /* This sets the color map which will be used to map the valdata values
   * to colors on the isosurface.
   */
  ERRCHK( dp_std_cmap(-1.0, 1.0, 1) );

  /* Open a GOB into which to put the isosurface */
  ERRCHK( dp_open("mygob") );

  /* This actually generates the isosurface GOB.  See the documentation
   * for an explaination of the parameters.
   */
  ERRCHK( dp_irreg_isosurf( P3D_CVVTX, (float *)data, (float *)valdata,
			   ISO_NX, ISO_NY, ISO_NZ, val,
			   coordfun, inner_surface ) );

  /* Close the GOB which holds the new objects. */
  ERRCHK( dp_close() );
}

main()
{
  /* Initialize the renderer */
  ERRCHK( dp_init_ren("myrenderer","p3d","example_11.p3d",
		      "I don't care about this string") );

  /* Generate a light source and camera */
  ERRCHK( dp_open("mylights") );
  ERRCHK( dp_light(&light_loc, &light_color) );
  ERRCHK( dp_ambient(&ambient_color) );
  ERRCHK( dp_close() );
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );

  /* This function actually generates the isosurface GOB */
  create_isosurf();

  /* Cause the isosurface */
  ERRCHK( dp_snap("mygob","mylights","mycamera") );

  /* Close DrawP3D */
  ERRCHK( dp_shutdown() );
}

