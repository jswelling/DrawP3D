/****************************************************************************
 * transform.c
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
This module provides some routines for manipulating colors.
*/

#include <stdio.h>
#include <math.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "indent.h"

void dump_color( P_Color *thiscolor )
/* This routine dumps a color */
{
  ger_debug("color: dump_color");

  switch( thiscolor->ctype ) {
  case P3D_RGB:
    ind_write("RGBA= ( %f %f %f %f )", 
	      thiscolor->r, thiscolor->g, thiscolor->b, thiscolor->a);
    ind_eol();
    break;
  default:
    ind_write("Unknown color type %d!",thiscolor->ctype);
  }
}

P_Color *allocate_color( VOIDLIST )
/* This routine allocates a color */
{
  P_Color *thiscolor;

  ger_debug("color: allocate_color");

  if ( !(thiscolor= (P_Color *)malloc(sizeof(P_Color))) )
    ger_fatal("color: allocate_color: unable to allocate %d bytes!",
              sizeof(P_Color));

  return( thiscolor );
}

void destroy_color( P_Color *thiscolor )
/* This routine destroys a color */
{
  ger_debug("color: destroy_color");
  free( (P_Void_ptr)thiscolor );
}

void copy_color( P_Color *target, P_Color *source )
/* This routine copies a color into existing memory */
{
  int i;
  float *f1, *f2;

  ger_debug("color: copy_color");

  target->ctype= source->ctype;
  target->r= source->r;
  target->g= source->g;
  target->b= source->b;
  target->a= source->a;
}

P_Color *duplicate_color( P_Color *thiscolor )
/* This routine returns a unique copy of a color */
{
  P_Color *dup;

  ger_debug("color: duplicate_color");

  dup= allocate_color();
  copy_color(dup, thiscolor);
  return(dup);
}

void rgbify_color( P_Color *thiscolor )
/* This routine converts a color to RGB format, in place. */
{
  ger_debug("color: rgbify_color");

  if (thiscolor->ctype != P3D_RGB)
    ger_error("color: rgbify_color: unknown color type %d",thiscolor->ctype);
}
