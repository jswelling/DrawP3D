/****************************************************************************
 * autopaint_tester.c
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "p3dgen.h"
#include "drawp3d.h"
#include "pgen_objects.h" /* for access to transform routines */

static void errchk( int ierr )
{
  if (ierr != P3D_SUCCESS) {
    fprintf(stderr,"Error!\n");
    abort();
  }
}

static void build_model()
{
  static P_Point llf= {-1.0, -1.0, -1.0};
  static P_Point trb= {1.0, 1.0, 1.0};
  errchk( dp_open("mygob") );
  errchk( dp_sphere() );
  errchk( dp_boundbox( &llf, &trb ) );
  errchk( dp_close() );
}

static void init_graphics()
{
  static P_Point from= { 0.0, 0.0, 10.0 };
  static P_Point to= { 0.0, 0.0, 0.0 };
  static P_Vector up= { 0.0, 1.0, 0.0 };
  static float fovea= 30.0;
  static float hither= -5.0;
  static float yon= -15.0;

  errchk( dp_init_ren("myren","xpainter","automanage","300x300") );
  errchk( dp_init_ren("myren2","xpainter","automanage","300x200") );
  errchk( dp_camera("mycam",&from, &to, &up, fovea, hither, yon) );
}

static void shutdown_graphics()
{
  errchk( dp_shutdown() );
}

main(int argc, char* argv[])
{
  int run_flag;
  char instring[128];
  char *inchar;
  static P_Vector x_vec= { 1.0, 0.0, 0.0 };
  static P_Vector y_vec= { 0.0, 1.0, 0.0 };
  P_Transform* working_trans;
  P_Transform* l_rot;
  P_Transform* r_rot;
  P_Transform* u_rot;
  P_Transform* d_rot;

  init_graphics();
  build_model();

  working_trans= duplicate_trans( Identity_trans );
  l_rot= rotate_trans( &y_vec, 10.0 );
  r_rot= rotate_trans( &y_vec, -10.0 );
  u_rot= rotate_trans( &x_vec, -10.0 );
  d_rot= rotate_trans( &x_vec, 10.0 );

  errchk(dp_open("rotgob"));
  errchk(dp_child("mygob"));
  errchk(dp_transform(working_trans));
  errchk(dp_close());
  errchk( dp_snap("rotgob", "standard_lights", "mycam") );

  run_flag= 1;
  puts("Inputs: u=up, d=down, l=left, r=right, q=quit");
  while (run_flag) {
    fprintf(stdout,"%s> ",argv[0]);
    fgets(instring,128,stdin);
    inchar= instring;
    while (*inchar && run_flag) {
    char thischar= *inchar++;
      switch (thischar) {
      case '\n':
	{
	}
      break;
      case 'q':
	{
	  run_flag= 0;
	  continue;
	}
      break;
      case 'u':
	{
	  (void)premult_trans( u_rot, working_trans );
	}
      break;
      case 'd':
	{
	  (void)premult_trans( d_rot, working_trans );
	}
      break;
      case 'l':
	{
	  (void)premult_trans( l_rot, working_trans );
	}
      break;
      case 'r':
	{
	  (void)premult_trans( r_rot, working_trans );
	}
      break;
      default:
	{
	  fprintf(stdout,"unknown '%c'\n",thischar);
	}
      }
    }
    errchk(dp_open("rotgob"));
    errchk(dp_child("mygob"));
    errchk(dp_transform(working_trans));
    errchk(dp_close());
    errchk( dp_snap("rotgob", "standard_lights", "mycam") );
  }
  
  shutdown_graphics();
}
