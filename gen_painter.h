/****************************************************************************
 * painter.h
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Chris Nuuja
 * Modified by Joe Demers
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
#define INITIAL_TEMP_COORDS 256

#undef HUGE
#ifndef HUGE
#define HUGE 1.0e30
#endif

/*  structure creation */
#ifndef NOMALLOCDEF
/* extern char *malloc(); */
#endif

extern void pnt_fillDpoly_rec( P_Renderer *, int, int, primtype );
extern int  pnt_makeDcolor_rec(P_Renderer *, double, double, double, double);
extern int pnt_new_DepthPoly( P_Renderer * );
extern int pnt_new_DLightIndex( P_Renderer * );
extern Pnt_Pointtype *pnt_makepoint_rec(float, float, float);
extern Pnt_Vectortype *pnt_makevector_rec(float, float, float);
extern Pnt_Vectortype *pnt_make_directionvector(Pnt_Pointtype *, 
						Pnt_Pointtype *);

/*	vector operations 	*/
extern void  pnt_calc_normal(P_Renderer *, Pnt_Polytype *, float *,
			     Pnt_Vectortype *);
extern float pnt_vector_length(register Pnt_Vectortype *);
extern float pnt_dotproduct(register Pnt_Vectortype *, 
			    register Pnt_Vectortype *);

/*      high level operations */
#ifdef INCL_PAINTER
extern void pnt_shutdown_renderer();
extern void draw_DepthBuffer(P_Renderer *);
#endif
#ifdef INCL_XPAINTER
extern void xdraw_DepthBuffer(P_Renderer *);
extern void xpnt_shutdown_renderer(P_Renderer *);
#endif

/*      methods initialization */
extern P_Renderer *fill_methods(P_Renderer *);

/*	Matrix operations	*/
extern float *pnt_make3dScale(float, float, float);
extern float *pnt_make3dRotate(float, char *);
extern float *pnt_make3dTrans(float, float, float);
extern float *pnt_mult3dMatrices(register float [16], register float [16]);
extern void pnt_append3dMatrices(float *, float *);
extern Pnt_Vectortype *pnt_vector_matrix_mult3d(Pnt_Vectortype *, float *);

/*  Clipping commands */
extern void pnt_clip_Zpolygon( P_Renderer *, int, int, int *, float * );
extern void pnt_clip_Zline( P_Renderer *, int, int, int *, float * );
extern int pnt_insideZbound( P_Renderer *, float, int );

/*  Coordinate buffers used for transformations and clipping  */
extern float *pnt_Xcoord_buffer,*pnt_Ycoord_buffer,*pnt_Zcoord_buffer;
extern float *pnt_Xclip_buffer, *pnt_Yclip_buffer, *pnt_Zclip_buffer;
