/****************************************************************************
 * indent.h
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This file provides entry points for the 'indented writing' package
indent.c .
*/
extern void ind_setup();
#ifdef __STDC__
extern void ind_write( char *, ... );
#else
extern void ind_write();
#endif
extern void ind_push();
extern void ind_pop();
extern void ind_eol();
