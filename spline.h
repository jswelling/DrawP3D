/****************************************************************************
 * spline.h
 * Author Joel Welling
 * Copyright 1999, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
 * This module provides an interface to a simple spline class.  It is 
 * derived from the starter code for CMU 15-462 (Intro to Computer Graphics) 
 * Assignment 2, which
 * contained the following information:
 *
 * spline.h 
 * This file has the basic framework for spline control points.
 * Adding this allows unifomity in splines so that spline files
 * can be provided as well as traded between students.  Also 
 * allows a clear method of grading Assignment 2. 
 *
 * Chris Rodriguez 
 *
 * 4 Feb 1999 
 */

#ifndef spline_h
#define spline_h

#include <p3dgen.h>

/* This keeps a standard way of defining all the spline files that you 
can get. */
typedef enum {SPL_CATMULLROM, SPL_HERMITE, SPL_BSPLINE, SPL_BEZIER} SplineType;

typedef struct {
  int numofpoints;
  float aux;
  SplineType type;
  P_Point *points;  
} Spline;

extern Spline* spl_read(char* filename);

extern Spline* spl_create(int npts_in, SplineType type_in, 
			  P_Point* pts_in); /* creates a copy of pts_in */

extern void spl_destroy(Spline*);

Spline* spl_copy(Spline *in);

extern void spl_print(Spline*);

extern Spline* spl_close_loop(Spline* open);

extern void spl_set_tension(Spline* sp, float val);

extern float spl_get_tension(Spline* sp);

extern void spl_calc( P_Point* out, Spline* ctl, float loc);

extern void spl_grad( P_Vector* out, Spline* ctl, float loc);

extern void spl_gradsqr( P_Vector* out, Spline* ctl, float loc);

#endif


