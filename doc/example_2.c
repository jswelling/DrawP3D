/* Example 2:
 * This example differs from example_1 in that we add explicit lights
 * and camera.
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

main()
{
  ERRCHK( dp_init_ren("myrenderer","p3d","example_2.p3d",
		      "This string will be ignored") );
  
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_close() );

  ERRCHK( dp_open("mylights") );
  ERRCHK( dp_ambient(&ambcolor) );
  ERRCHK( dp_light(&lightloc, &lightcolor) );
  ERRCHK( dp_close() );

  ERRCHK( dp_camera( "mycamera", &lookfrom, &lookat, &up, fovea,
		    hither, yon ) );

  ERRCHK( dp_snap("mygob","mylights","mycamera") );

  ERRCHK( dp_shutdown() );
}
