/****************************************************************************
 * gl_incl.h
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Robert Earhart
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

/* Buffer sizes */
#define INITIAL_MAX_DEPTHPOLY 2500
#define INITIAL_MAX_DCOORD 1000
#define INITIAL_MAX_DCOLOR 1000
#define INITIAL_MAX_DLIGHTS 10
#define INITIAL_TEMP_COORDS 256;

#ifndef HUGE
#define HUGE 1.0e30
#endif

/*  structure creation */
#ifndef NOMALLOCDEF
extern char *malloc();
#endif

