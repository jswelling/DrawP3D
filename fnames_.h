/****************************************************************************
 * fnames_.h
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
lower case and end in an underscore.
*/

/* Overall control routines */
#define pshtdn pshtdn_

/* Renderer control routines */
#define pintrn pintrn_
#define popnrn popnrn_
#define pclsrn pclsrn_
#define pshtrn pshtrn_
#define pprtrn pprtrn_

/* Gob routines */
#define popen  popen_
#define pclose pclose_
#define pfree  pfree_
#define piatt  piatt_
#define pblatt pblatt_
#define pfatt  pfatt_
#define pstatt pstatt_
#define pclatt pclatt_
#define pptatt pptatt_
#define pvcatt pvcatt_
#define ptratt ptratt_
#define pmtatt pmtatt_
#define pgbclr pgbclr_
#define ptxtht ptxtht_
#define pbkcul pbkcul_
#define pgbmat pgbmat_
#define ptnsfm ptnsfm_
#define ptrans ptrans_
#define protat protat_
#define pscale pscale_
#define pascal pascal_
#define pchild pchild_
#define pprtgb pprtgb_

/* Primitive routines */
#define pcyl   pcyl_
#define psphr  psphr_
#define ptorus ptorus_
#define pplymk pplymk_
#define pplyln pplyln_
#define pplygn pplygn_
#define ptrist ptrist_
#define pmesh  pmesh_
#define pbezp  pbezp_
#define ptext  ptext_
#define plight plight_
#define pamblt pamblt_

/* Composite routines */
#define paxis  paxis_
#define pbndbx pbndbx_
#define pisosf pisosf_
#define pzsurf pzsurf_
#define prnzsf prnzsf_
#define prniso prniso_
#define pirzsf pirzsf_
#define piriso piriso_
#define ptbmol ptbmol_

/* Camera */
#define pcamra pcamra_
#define pprtcm pprtcm_
#define pcmbkg pcmbkg_

/* Snap */
#define psnap psnap_

/* Color map */
#define pstcmp pstcmp_
#define psdcmp psdcmp_

/* Debugging */
#define pdebug pdebug_
