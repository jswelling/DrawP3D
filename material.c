/****************************************************************************
 * transform.c
 * Author Joel Welling
 * Copyright 1991, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides some routines for manipulating materials.
*/

#include <stdio.h>
#include <math.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "indent.h"

/* Predefined materials */
static P_Material p3d_default_material_store= {P3D_DEFAULT_MATERIAL};
P_Material *p3d_default_material= &p3d_default_material_store;
static P_Material p3d_dull_material_store= {P3D_DULL_MATERIAL};
P_Material *p3d_dull_material= &p3d_dull_material_store;
static P_Material p3d_shiny_material_store= {P3D_SHINY_MATERIAL};
P_Material *p3d_shiny_material= &p3d_shiny_material_store;
static P_Material p3d_metallic_material_store= {P3D_METALLIC_MATERIAL};
P_Material *p3d_metallic_material= &p3d_metallic_material_store;
static P_Material p3d_matte_material_store= {P3D_MATTE_MATERIAL};
P_Material *p3d_matte_material= &p3d_matte_material_store;
static P_Material p3d_aluminum_material_store= {P3D_ALUMINUM_MATERIAL};
P_Material *p3d_aluminum_material= &p3d_aluminum_material_store;

void dump_material( P_Material *thismaterial )
/* This routine dumps a material */
{
  ger_debug("material: dump_material");

  switch( thismaterial->type ) {
  case P3D_DEFAULT_MATERIAL:
    ind_write("Default material"); ind_eol(); break;
  case P3D_DULL_MATERIAL:
    ind_write("Dull material"); ind_eol(); break;
  case P3D_SHINY_MATERIAL:
    ind_write("Shiny material"); ind_eol(); break;
  case P3D_METALLIC_MATERIAL:
    ind_write("Metallic material"); ind_eol(); break;
  case P3D_MATTE_MATERIAL:
    ind_write("Matte material"); ind_eol(); break;
  case P3D_ALUMINUM_MATERIAL:
    ind_write("Aluminum material"); ind_eol(); break;
  default:
    ind_write("Unknown material type %d!",thismaterial->type);
  }
}

P_Material *allocate_material( VOIDLIST )
/* This routine allocates a material */
{
  P_Material *thismaterial;

  ger_debug("material: allocate_material");

  if ( !(thismaterial= (P_Material *)malloc(sizeof(P_Material))) )
    ger_fatal("material: allocate_material: unable to allocate %d bytes!",
              sizeof(P_Material));

  return( thismaterial );
}

void destroy_material( P_Material *thismaterial )
/* This routine destroys a material */
{
  ger_debug("material: destroy_material");
  free( (P_Void_ptr)thismaterial );
}

void copy_material( P_Material *target, P_Material *source )
/* This routine copies a material into existing memory */
{
  int i;
  float *f1, *f2;

  ger_debug("material: copy_material");

  target->type= source->type;
}

P_Material *duplicate_material( P_Material *thismaterial )
/* This routine returns a unique copy of a material */
{
  P_Material *dup;

  ger_debug("material: duplicate_material");

  dup= allocate_material();
  copy_material(dup, thismaterial);
  return(dup);
}

