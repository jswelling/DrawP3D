/****************************************************************************
 * FNAMES.h
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
This include file transforms the names of drawp3d fortran interface functions
to the form appropriate for machines where fortran external symbols are
upper case with no trailing underscores.
*/

/* Overall control routines */
#define pshtdn PSHTDN

/* Renderer control routines */
#define pintrn PINTRN
#define popnrn POPNRN
#define pclsrn PCLSRN
#define pshtrn PSHTRN
#define pprtrn PPRTRN

/* Gob routines */
#define popen  POPEN
#define pclose PCLOSE
#define pfree  PFREE
#define piatt  PIATT
#define pblatt PBLATT
#define pfatt  PFATT
#define pstatt PSTATT
#define pclatt PCLATT
#define pptatt PPTATT
#define pvcatt PVCATT
#define ptratt PTRATT
#define pmtatt PMTATT
#define pgbclr PGBCLR
#define ptxtht PTXTHT
#define pbkcul PBKCUL
#define pgbmat PGBMAT
#define ptnsfm PTNSFM
#define ptrans PTRANS
#define protat PROTAT
#define pscale PSCALE
#define pascal PASCAL
#define pchild PCHILD
#define pprtgb PPRTGB

/* Primitive routines */
#define pcyl   PCYL
#define psphr  PSPHR
#define ptorus PTORUS
#define pplymk PPLYMK
#define pplyln PPLYLN
#define pplygn PPLYGN
#define ptrist PTRIST
#define pmesh  PMESH
#define pbezp  PBEZP
#define ptext  PTEXT
#define plight PLIGHT
#define pamblt PAMBLT

/* Composite routines */
#define paxis  PAXIS
#define pbndbx PBNDBX
#define pisosf PISOSF
#define pzsurf PZSURF
#define prnzsf PRNZSF
#define prniso PRNISO
#define pirzsf PIRZSF
#define piriso PIRISO
#define ptbmol PTBMOL

/* Camera */
#define pcamra PCAMRA
#define pprtcm PPRTCM
#define pcmbkg PCMBKG

/* Snap */
#define psnap PSNAP

/* Color map */
#define pstcmp PSTCMP
#define psdcmp PSDCMP

/* Debugging */
#define pdebug PDEBUG
