/* Example 9:
 * This example places a cylinder of a given size between two given
 * points in space.
 */

#include <stdio.h>
#include <math.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

/* Degrees per radian */
#define RADTODEG 180.0/3.14159265

/* Vectors are considered to align if the ratio of their cross product 
 * squared to their dot product squared is less than the following value.
 */
#define epsilon 0.00001

/* This routine produces a rotation which will reverse the given
 * vector (as opposed to a simple inversion, which is not a rotation.
 */
static void make_flip( vx, vy, vz )
float vx, vy, vz;
{
  float px= 0.0, py= 0.0, pz= 1.0, dot, cx, cy, cz, normsqr;
  P_Vector axis;

  /* Find a vector not parallel to the given vector */
  dot= px*vx + py*vy + pz*vz;
  cx= py*vz - pz*vy;
  cy= pz*vx - px*vz;
  cz= px*vy - py*vx;
  if ( (cx*cx + cy*cy + cz*cz) < epsilon*dot*dot ) { /* this p won't work */
    px= 1.0; py= 0.0; pz= 0.0;
    dot= px*vx + py*vy + pz*vz;
    };

  /* Extract the normal component of that vector */
  normsqr= vx*vx + vy*vy + vz*vz;
  px= px - vx*dot/normsqr;
  py= py - vy*dot/normsqr;
  pz= pz - vz*dot/normsqr;

  /* Generate a 180 degree rotation about that vector */
  axis.x= px;
  axis.y= py;
  axis.z= pz;
  ERRCHK( dp_rotate( &axis, 180.0 ) );
}

/* This routine returns a rotation which will rotate its first
 * parameter vector to align with its second parameter vector.
 */
static void make_aligning_rotation( v1x, v1y, v1z, v2x, v2y, v2z )
float v1x, v1y, v1z, v2x, v2y, v2z;
{
  float ax, ay, az, dot, theta;
  P_Vector axis;

  /* Take the cross and dot products of the two vectors */
  ax= v1y*v2z - v1z*v2y;
  ay= v1z*v2x - v1x*v2z;
  az= v1x*v2y - v1y*v2x;
  dot= v1x*v2x + v1y*v2y + v1z*v2z;

  if ( (ax*ax + ay*ay + az*az) < epsilon*dot*dot ) { /* they already align */
    if (dot >= 0.0 ) { /* parallel */
      ERRCHK( dp_translate( 0.0, 0.0, 0.0 ) ); /* no rotation needed */
    }
    else /* anti-parallel */
      make_flip( v1x, v1y, v1z );
    }
  else {
    /* Find the angle of rotation around the cross product axis */
    theta= acos( ( v1x*v2x+v1y*v2y+v1z*v2z ) / 
		( sqrt( v1x*v1x+v1y*v1y+v1z*v1z )
		 * sqrt( v2x*v2x+v2y*v2y+v2z*v2z ) ) );
    /* Generate a theta degree rotation about that vector */
    axis.x= ax;
    axis.y= ay;
    axis.z= az;
    ERRCHK( dp_rotate( &axis, RADTODEG*theta ) );
  }

  /* NOTREACHED */
}

/* This routine creates a cylinder between the two end points, and of
 * the given diameter.  The cylinder is added to the currently open GOB.
 */
static void make_connecting_cylinder( p1, p2, rad )
P_Point *p1;
P_Point *p2;
float rad;
{
  float length;
  float dx, dy, dz;

  /* All of this goes inside a nameless GOB */
  ERRCHK( dp_open("") );

  /* Find the needed orientation, and the length */
  dx= p2->x - p1->x;
  dy= p2->y - p1->y;
  dz= p2->z - p1->z;
  length= sqrt( dx*dx + dy*dy + dz*dz );

  /* Add a scaling operation to make it the right length and diameter */
  ERRCHK( dp_ascale( rad, rad, length ) );

  /* Add a rotation to line the cylinder up with the vector between the
   * two points.
   */
  make_aligning_rotation( 0.0, 0.0, 1.0, dx, dy, dz );

  /* Add a translation to move the end to point 1 */
  ERRCHK( dp_translate( p1->x, p1->y, p1->z ) );

  /* Add the cylinder */
  ERRCHK( dp_cylinder() );

  /* Close the outer GOB */
  ERRCHK( dp_close() );
}

/* This is the main routine. */
main()
{
  /* Camera specificatin information */
  static P_Vector up = { 0.0, 1.0, 0.0 };
  static P_Point from = { 0.0, 0.0, 20.0 };
  static P_Point at = { 0.0, 0.0, 0.0 };
  static P_Point pt1 = { 2.0, 0.0, 0.0 };
  static P_Point pt2 = { 4.0, 5.0, 1.0 };
  static P_Color blue = { P3D_RGB, 0.0, 0.0, 1.0, 1.0 };

  /* Initialize the renderer. */
  ERRCHK( dp_init_ren("myrenderer","p3d","example_9.p3d",
		      "I don't care about this string") );

  /* Create a camera, using the information above. */
  ERRCHK( dp_camera( "this_camera", &from, &at, &up, 45.0, -5.0, -30.0 ) );

  /* Open the overall model */
  ERRCHK( dp_open("mygob") );

  /* Add spheres at points 1 and 2 */
  ERRCHK( dp_open("") );
  ERRCHK( dp_translate(pt1.x, pt1.y, pt1.z) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("") );
  ERRCHK( dp_translate(pt2.x, pt2.y, pt2.z) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_close() );

  /* Add a blue cylinder between the two points */
  ERRCHK( dp_open("") );
  ERRCHK( dp_gobcolor(&blue) );
  make_connecting_cylinder( &pt1, &pt2, 0.5 );
  ERRCHK( dp_close() );

  /* Close the overall model */
  ERRCHK( dp_close() );

  /* Cause the GOB "mygob" to be rendered. */
  ERRCHK( dp_snap("mygob","standard_lights","this_camera") );

  /* Shut down the DrawP3D software. */
  ERRCHK( dp_shutdown() );
}

