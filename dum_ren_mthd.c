/****************************************************************************
 * p3dgen.c
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
 * This module provides dummy renderer creation entry points, to simplify
 * the automation of library creation.
 */

#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"


#ifndef INCL_PAINTER
P_Renderer *po_create_painter_renderer( char *device, char *datastr )
{
  ger_debug("dum_ren_mthd: po_create_painter_renderer:");
    ger_error("dum_ren_mthd: po_create_painter_renderer: \n\
Sorry, renderer type 'painter' not installed;  using 'p3d' instead.");
  return( po_create_p3d_renderer( device, datastr ) );
}
#endif

#ifndef INCL_GL
P_Renderer *po_create_gl_renderer( char *device, char *datastr )
{
  ger_debug("dum_ren_mthd: po_create_gl_renderer:");
    ger_error("dum_ren_mthd: po_create_gl_renderer: \n\
Sorry, renderer type 'gl' not installed;  using 'p3d' instead.");
  return( po_create_p3d_renderer( device, datastr ) );
}
#endif

#ifndef INCL_XPAINTER
P_Renderer *po_create_xpainter_renderer( char *device, char *datastr )
{
  ger_debug("dum_ren_mthd: po_create_xpainter_renderer:");
  ger_error("dum_ren_mthd: po_create_xpainter_renderer: \n\
Sorry, renderer type 'xpainter' not installed;  using 'p3d' instead.");
  return( po_create_p3d_renderer( device, datastr ) );
}
#endif

#ifndef INCL_PVM
P_Renderer *po_create_pvm_renderer( char *device, char *datastr )
{
  ger_debug("dum_ren_mthd: po_create_pvm_renderer:");
  ger_error("dum_ren_mthd: po_create_pvm_renderer: \n\
Sorry, renderer type 'pvm' not installed;  using 'p3d' instead.");
  return( po_create_p3d_renderer( device, datastr ) );
}
#endif

#ifndef INCL_IV
P_Renderer *po_create_iv_renderer( char *device, char *datastr )
{
  ger_debug("dum_ren_mthd: po_create_iv_renderer:");
  ger_error("dum_ren_mthd: po_create_iv_renderer: \n\
Sorry, renderer type 'iv' not installed;  using 'p3d' instead.");
  return( po_create_p3d_renderer( device, datastr ) );
}

P_Renderer *po_create_vrml_renderer( char *device, char *datastr )
{
  ger_debug("dum_ren_mthd: po_create_vrml_renderer:");
  ger_error("dum_ren_mthd: po_create_vrml_renderer: \n\
Sorry, renderer type 'vrml' not installed;  using 'p3d' instead.");
  return( po_create_p3d_renderer( device, datastr ) );
}
#endif

#ifndef INCL_LVR
P_Renderer *po_create_lvr_renderer( char *device, char *datastr )
{
  ger_debug("dum_ren_mthd: po_create_lvr_renderer:");
  ger_error("dum_ren_mthd: po_create_lvr_renderer: \n\
Sorry, renderer type 'lvr' not installed;  using 'p3d' instead.");
  return( po_create_p3d_renderer( device, datastr ) );
}
#endif

