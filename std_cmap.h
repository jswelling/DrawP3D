/****************************************************************************
 * std_cmap.h
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
This module provides information on standard color maps.
*/

#define NUM_STANDARD_MAPFUNS 5

/* The following is necessary to assure that po_standard_mapfuns is
 * properly initialized under VAX VMS C.
 */
#ifdef VMS
globalref
#else
extern
#endif
void (*po_standard_mapfuns[NUM_STANDARD_MAPFUNS])
                           ( float *, float *, float *, float *, float * );
