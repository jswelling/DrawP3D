/* Example 4:
 * This example takes the hydrogen atom produced in example 3 and produces
 * a number of frames with the atom in different orientations.
 */

#include <stdio.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Light specification information */
static P_Point lightloc= { 0.0, 20.0, 20.0 };
static P_Color lightcolor= { P3D_RGB, 0.7, 0.7, 0.7, 1.0 };
static P_Color ambcolor= { P3D_RGB, 0.3, 0.3, 0.3, 1.0 };

/* Camera specification information */
static P_Point lookfrom= { 0.0, 0.0, 10.0 };
static P_Point lookat= { 0.0, 0.0, 0.0 };
static P_Vector up= { 0.0, 1.0, 0.0 };
static float fovea= 53.0;
static float hither= -5.0;
static float yon= -15.0;

/* Atomic information */
static float h_radius= 0.3; /* 1/4 of Van der Waals */
static P_Color h_color= { P3D_RGB, 1.0, 1.0, 1.0, 1.0 };
static float o_radius= 0.35; /* 1/4 of Van der Waals */
static P_Color o_color= { P3D_RGB, 1.0, 0.0, 0.0, 1.0 };
static P_Point h1_loc= { 0.854821, -.487790, 0.0 };
static P_Point h2_loc= { -0.854821, -.487790, 0.0 };
static P_Point o_loc= { 0.0, 0.121948, 0.0 };

/* Polyline to join the atoms */
static P_Color pline_color= { P3D_RGB, 0.0, 0.0, 1.0, 1.0 };
static float pline_coords[]= {
  0.854821, -.487790, 0.0,
  0.0, 0.121948, 0.0,
  -0.854821, -.487790, 0.0 
  };

/* Axis about which to rotate */
static P_Vector y_axis= { 0.0, 1.0, 0.0 };

/* Number of rotation steps, and angle between steps */
static int nsteps= 120;
static float dtheta= 3.0;

static void create_h2o()
/* This routine creates the water molecule- a GOB named 'h2o' */
{
  /* Make spheres of appropriate radius and color for elements.
   * Note that the order in which attributes, transformations, and
   * GOBs are inserted into a named GOB is irrelevant.
   */
  ERRCHK( dp_open("hydrogen") );
  ERRCHK( dp_scale(h_radius) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_gobcolor(&h_color) );
  ERRCHK( dp_close() );

  ERRCHK( dp_open("oxygen") );
  ERRCHK( dp_scale(o_radius) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_gobcolor(&o_color) );
  ERRCHK( dp_close() );

  /* Open the outer GOB, and insert in it three nameless subGOBs.  Each
   * contains one atom and the transformation appropriate to put it in
   * the proper place.  
   */
  ERRCHK( dp_open("h2o") );

  ERRCHK( dp_open("") );
  ERRCHK( dp_translate( h1_loc.x, h1_loc.y, h1_loc.z ) );
  ERRCHK( dp_child("hydrogen") );
  ERRCHK( dp_close() );

  ERRCHK( dp_open("") );
  ERRCHK( dp_translate( h2_loc.x, h2_loc.y, h2_loc.z ) );
  ERRCHK( dp_child("hydrogen") );
  ERRCHK( dp_close() );

  ERRCHK( dp_open("") );
  ERRCHK( dp_translate( o_loc.x, o_loc.y, o_loc.z ) );
  ERRCHK( dp_child("oxygen") );
  ERRCHK( dp_close() );

  /* Insert a polyline connecting the atoms.  The color would apply
   * to the atoms as well, but it will be superceded by their own
   * color attribute.
   */
  ERRCHK( dp_gobcolor(&pline_color) );
  ERRCHK( dp_polyline( P3D_CVTX, P3D_RGB, pline_coords, 3 ) );

  /* Close the outer GOB */
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
    ERRCHK( dp_open("mygob") );
    ERRCHK( dp_rotate( &y_axis, theta ) );
    ERRCHK( dp_child("h2o") );
    ERRCHK( dp_close() );

    /* Snap and go on to the next model */
    ERRCHK( dp_snap("mygob","mylights","mycamera") );
    theta += dtheta;
  }
}

main()
{
  /* Initialize renderer */
  ERRCHK( dp_init_ren("myrenderer","p3d","example_4.p3d",
		      "This string will be ignored") );
  
  /* Create my lights and camera */
  ERRCHK( dp_open("mylights") );
  ERRCHK( dp_ambient(&ambcolor) );
  ERRCHK( dp_light(&lightloc, &lightcolor) );
  ERRCHK( dp_close() );
  ERRCHK( dp_camera( "mycamera", &lookfrom, &lookat, &up, fovea,
		    hither, yon ) );

  /* Create the water molecule */
  create_h2o();

  /* Do a series of frames of the molecule */
  make_snaps();

  /* Shut down renderer */
  ERRCHK( dp_shutdown() );
}
