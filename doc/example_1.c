/* Example 1:
 * This example represents the simplest possible DrawP3D program.  A
 * GOB is created, a sphere primitive GOB is placed in it, and the
 * model is viewed with the default lights and camera.
 */

#include <stdio.h>
#include <p3dgen.h>
#include <drawp3d.h>

/* Macro to ease checking of return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR!\n")

main()
{
  ERRCHK( dp_init_ren("myrenderer","p3d","example_1.p3d",
		      "This string will be ignored") );
  
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_close() );

  ERRCHK( dp_snap("mygob","standard_lights","standard_camera") );

  ERRCHK( dp_shutdown() );
}
