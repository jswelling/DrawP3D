/* 
	This include file establishes the entry point equivalence needed by
	certain Unix systems including Crays under UNICOS.  With these
	equivalences in place, Fortran routines will be able to call
	the C routines in cgmgen (and cgmgen_test2).
*/

/* Definition for cgmgen_test2 routine */
#define 	tpcla 		TPCLA

/* Definitions for cgmgen.c routines */
#define		stpcnm		STPCNM
#define		setdev		SETDEV
#define		setwcd		SETWCD
#define		tgldbg		TGLDBG
#define 	wrtopn		WRTOPN
#define 	wrmxci		WRMXCI
#define 	wrtend		WRTEND
#define 	wrbegp		WRBEGP
#define 	wrbgpb		WRBGPB
#define 	wristl		WRISTL
#define 	wrctbl		WRCTBL
#define 	wrtxtc		WRTXTC
#define 	wrtxts		WRTXTS
#define		wrtxte		WRTXTE
#define		wtxtsp		WTXTSP
#define		wrtxtf		WRTXTF
#define		wrtxtp		WRTXTP
#define		wrtxto		WRTXTO
#define		wrtxta		WRTXTA
#define 	wrftxt		WRFTXT
#define 	wrplnc	        WRPLNC
#define         wrplnw          WRPLNW
#define 	wrplin		WRPLIN
#define 	wrpgnc		WRPGNC
#define 	wrtpgn		WRTPGN
#define 	wrpmkc		WRPMKC
#define 	wrpmkt		WRPMKT
#define 	wrpmks		WRPMKS
#define 	wrtpmk		WRTPMK
#define 	wrtcla		WRTCLA
#define 	wrendp		WRENDP
#define 	wrtcsm		WRTCSM
#define 	wrbgdc		WRBGDC
#define 	wtxtdc		WTXTDC
#define		wtxtpr		WTXTPR
#define 	wplndc		WPLNDC
#define 	wpgndc		WPGNDC
#define 	wpmkdc		WPMKDC
#define 	wcladc		WCLADC
#define 	wrgcla		WRGCLA
#define 	wqcadc		WQCADC
#define 	qclarw		QCLARW
#define 	wrttxt		WRTTXT
#define 	wrpcla		WRPCLA
#define 	igtmem		IGTMEM
#define 	ifrmem		IFRMEM
#define 	wrtimg		WRTIMG
#define 	rdimg 		RDIMG
