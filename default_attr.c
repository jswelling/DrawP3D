/****************************************************************************
 * default_attr.c
 * Author Joel Welling
 * Copyright 1990, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
/*
This module generates the default attribute list for DrawP3D.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"

P_Attrib_List *po_gen_default_attr( VOIDLIST )
/* Generate and return the default attribute list for DrawP3D */
{
  static P_Color white= { P3D_RGB, 1.0, 1.0, 1.0, 1.0 };
  int false= 0;
  int true= 1;
  float point_diameter= 0.01;
  float line_diameter= 0.01;
  float text_height= 1.0;
  float text_stroke_width_frac= 0.15;
  float text_stroke_thick_frac= 0.1;

  P_Attrib_List *result= (P_Attrib_List *)0;

  ger_debug("default_attr: po_gen_default_attr");

  result= add_attr( result, "material", P3D_MATERIAL, 
		   (P_Void_ptr)p3d_default_material );
  result= add_attr( result, "color", P3D_COLOR, 
		   (P_Void_ptr)&white );
  result= add_attr( result, "backcull", P3D_BOOLEAN, 
		   (P_Void_ptr)&false );
  result= add_attr( result, "point-diameter", P3D_FLOAT, 
		   (P_Void_ptr)&point_diameter );
  result= add_attr( result, "line-diameter", P3D_FLOAT, 
		   (P_Void_ptr)&line_diameter );
  result= add_attr( result, "text-height", P3D_FLOAT, 
		   (P_Void_ptr)&text_height );
  result= add_attr( result, "text-font", P3D_STRING, 
		   (P_Void_ptr)"simplex_roman" );
  result= add_attr( result, "text-stroke-width-fraction", P3D_FLOAT,
		   (P_Void_ptr)&text_stroke_width_frac );
  result= add_attr( result, "text-stroke-thickness-fraction", P3D_FLOAT,
		   (P_Void_ptr)&text_stroke_thick_frac );
  result= add_attr( result, "shadows", P3D_BOOLEAN, (P_Void_ptr)&true );

  return(result);
}
