/****************************************************************************
 * unix_defs.h
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Joel Welling
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
	This include file establishes the entry point equivalence needed by
	Unix systems based on the 'portable F77 compiler'.  With these
	equivalences in place, Fortran routines will be able to call
	the C routines in cgmgen (and cgmgen_test2).
*/

/* Definition for cgmgen_test2 routine */
#define 	tpcla 		tpcla_

/* Definitions for cgmgen.c routines */
#define		setdev		setdev_
#define		setwcd		setwcd_
#define		tgldbg		tgldbg_
#define 	wrtopn		wrtopn_
#define 	wrmxci		wrmxci_
#define 	wrtend		wrtend_
#define 	wrbegp		wrbegp_
#define 	wrbgpb		wrbgpb_
#define 	wristl		wristl_
#define 	wrctbl		wrctbl_
#define 	wrtxtc		wrtxtc_
#define 	wrtxts		wrtxts_
#define		wrtxte		wrtxte_
#define		wtxtsp		wtxtsp_
#define		wrtxtf		wrtxtf_
#define		wrtxtp		wrtxtp_
#define		wrtxto		wrtxto_
#define		wrtxta		wrtxta_
#define 	wrftxt		wrftxt_
#define 	wrplnc		wrplnc_
#define 	wrplin		wrplin_
#define 	wrpgnc		wrpgnc_
#define 	wrtpgn		wrtpgn_
#define 	wrpmkc		wrpmkc_
#define 	wrpmkt		wrpmkt_
#define 	wrpmks		wrpmks_
#define 	wrtpmk		wrtpmk_
#define 	wrtcla		wrtcla_
#define 	wrendp		wrendp_
#define 	wrtcsm		wrtcsm_
#define 	wrbgdc		wrbgdc_
#define 	wtxtdc		wtxtdc_
#define		wtxtpr		wtxtpr_
#define 	wplndc		wplndc_
#define 	wpgndc		wpgndc_
#define 	wpmkdc		wpmkdc_
#define 	wcladc		wcladc_
#define 	wrgcla		wrgcla_
#define 	wqcadc		wqcadc_
#define 	qclarw		qclarw_
#define 	wrttxt		wrttxt_
#define 	wrpcla		wrpcla_
#define 	igtmem		igtmem_
#define 	ifrmem		ifrmem_
#define 	wrtimg		wrtimg_
#define 	rdimg 		rdimg_
