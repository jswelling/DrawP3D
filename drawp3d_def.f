C****************************************************************************
C* drawp3d_def.f
C* Author Joel Welling
C* Copyright 1990, Pittsburgh Supercomputing Center, Carnegie Mellon University
C*
C* Permission use, copy, and modify this software and its documentation
C* without fee for personal use or use within your organization is hereby
C* granted, provided that the above copyright notice is preserved in all
C* copies and that that copyright and this permission notice appear in
C* supporting documentation.  Permission to redistribute this software to
C* other organizations or individuals is not granted;  that must be
C* negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
C* University make any representations about the suitability of this
C* software for any purpose.  It is provided "as is" without express or
C* implied warranty.
C*****************************************************************************/
C
C This file includes type information and parameters for DrawP3D
C

C     Version information
      real PPGENV, PDP3DV
      parameter ( PPGENV=1.0, PDP3DV=1.0 )

C     Return codes
      integer PPASS, PFAIL
      parameter ( PPASS=1, PFAIL=0 )

C     Booleans
      integer PTRUE, PFALSE
      parameter ( PTRUE=1, PFALSE=0 )

C     Vertex list types
      integer PCVX, PCCVX, PCCNVX, PCNVX, PCVVX, PCVNVX, PCVVVX
      parameter (PCVX=0, PCCVX=1, PCCNVX=2, PCNVX=3, PCVVX=4, PCVNVX=5
     $     PCVVVX=6)

C     Color specification types
      integer PRGB
      parameter ( PRGB=0 )

C     Standard material types
      integer PDFMAT, PDULL, PSHINY, PMETAL, PMATTE, PALUM
      parameter (PDFMAT=0, PDULL=1, PSHINY=2, PMETAL=3, PMATTE=4,
     $     PALUM=5)

C     Maximum object name length
      integer PNAMELN
      parameter ( PNAMELN=64 )

C     Functions
      integer PAMBLT
      integer PASCAL
      integer PAXIS
      integer PBEZP
      integer PBKCUL
      integer PBLATT
      integer PBNDBX
      integer PCAMRA
      integer PCHILD
      integer PCLATT
      integer PCLOSE
      integer PCLSRN
      integer PCMBKG
      integer PCYL
      integer PDEBUG
      integer PFATT
      integer PFREE
      integer PGBCLR
      integer PGBMAT
      integer PIATT
      integer PINTRN
      integer PIRZSF
      integer PIRISO
      integer PISOSF
      integer PLIGHT
      integer PMESH
      integer PMTATT
      integer POPEN
      integer POPNRN
      integer PPLYGN
      integer PPLYLN
      integer PPLYMK
      integer PPRTCM
      integer PPRTGB
      integer PPRTRN
      integer PPTATT
      integer PRNISO
      integer PRNZSF
      integer PROTAT
      integer PSCALE
      integer PSDCMP
      integer PSHTDN
      integer PSHTRN
      integer PSNAP
      integer PSPHR
      integer PSTATT
      integer PSTCMP
      integer PSPLTB
      integer PTEXT
      integer PTNSFM
      integer PTORUS
      integer PTRANS
      integer PTRATT
      integer PTRIST
      integer PTXTHT
      integer PVCATT

