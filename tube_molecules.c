/****************************************************************************
 * tube_molecules.c
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
  This module creates a generalized cylinder from splines, in a shape
  appropriate to represent molecular secondary structure.
*/

#include <stdio.h>
#include <math.h>
#ifndef stardent
#include <stdlib.h>
#endif
#include "p3dgen.h"
#include "pgen_objects.h" /* because we must access vertex list methods */
#include "ge_error.h"
#include "spline.h"

/* Notes-
   -get rid of cast of norm_buf and mesh_buf types by unrolling inner loop
    in emit routine.
*/

/* Number of flavors of ribbon.  (Arrowheads don't count) */
#define N_LOOP_TYPES 5

/* Increasing this value increases the ribbon diameter */
#define LOOP_SIZE_SCALE 0.5

/* This changes the ribbon backbone tension.  The results are most visible
 * in the smoothness of the curve formed by unstructured (tube) elements.
 */
#define SKEL_TENSION 0.75

/* This changes the rate of response of the rotational phase to changes
 * in backbone curvature.  Sensible values are 0.0 to 1.0; higher is
 * faster response.
 */
#define PHASE_ANGLE_SMOOTHING 0.5

/* This factor controls how far an arrowhead extends beyond the ribbon.
 * A value of 1.0 means no extension.
 */
#define ARROWHEAD_BARB 2.0

static Spline* loop_tbl[N_LOOP_TYPES];
static int loop_tbl_initialized= 0;

static P_Point* loop_buf= NULL;
static int loop_buf_size= 0;

static P_Transform skel_matrix;
static P_Vector skel_offset;

static P_Point* mesh_buf= NULL;
static P_Vector* norm_buf= NULL;
static int mesh_buf_size= 0;

/* A hack to smooth phase angles based on factor above and number of
 * steps per backbone control point.
 */
static float working_phase_smooth= 0.0;

static void add_cross(float out[3], P_Point* ctr, P_Point* t1, P_Point* t2) {
  float v1[3];
  float v2[3];
  float cross[3];

  v1[0]= t1->x - ctr->x;
  v1[1]= t1->y - ctr->y;
  v1[2]= t1->z - ctr->z;
  
  v2[0]= t2->x - ctr->x;
  v2[1]= t2->y - ctr->y;
  v2[2]= t2->z - ctr->z;

  cross[0]= v1[1]*v2[2] - v1[2]*v2[1];
  cross[1]= v1[2]*v2[0] - v1[0]*v2[2];
  cross[2]= v1[0]*v2[1] - v1[1]*v2[0];

  out[0] += cross[0];
  out[1] += cross[1];
  out[2] += cross[2];
}

static void v3_normalize( float* a ) {
  float len;
  len= sqrt(a[0]*a[0] +a[1]*a[1] +a[2]*a[2]);
  if (len==0.0) {
    ger_fatal("p3dgen: pg_spline_tube: tried to normalize a zero-length vector!\n");
  }
  else {
    a[0] /= len;
    a[1] /= len;
    a[2] /= len;
  }
}

/* Save a malloc over matrix_vector_mult */
static void matrix_transform_affine(P_Transform* m,
				    P_Point* p_in, P_Point* p_out) {
  float vec[4];
  float result[4];
  int i;
  int j;

  /* We assume m is affine, so we only have to do the first 3 components */

  vec[0]= p_in->x;
  vec[1]= p_in->y;
  vec[2]= p_in->z;

  for (i=0; i<3; i++) {
    result[i]= 0.0;
    for (j=0; j<3; j++) {
      result[i] += m->d[4*i+j] * vec[j];
    }
  }

  p_out->x= result[0];
  p_out->y= result[1];
  p_out->z= result[2];
}

static int emit_geom(int loop_length, int skel_length, P_Vlist* vlist) {
  int i, j;
  float* buf;
  int prim_flag;
  P_Color current_clr;
  int closest_vtx;
  float* runner;
  float* mesh_runner;
  float* norm_runner;
  float capnorm[3];
  float* endcap_buf= NULL;
  
  if (pg_gob_open() == P3D_SUCCESS) {
    
    ger_debug( "p3dgen: pg_spline_tube: adding spline tube, %d long",
	       skel_length);
    
    if (!(buf= (float*)malloc( 2*6*loop_length*sizeof(float) )))
      ger_fatal("p3dgen: pg_spline_tube: unable to allocate %d floats!",
		2*6*loop_length);
    if (!(endcap_buf= (float*)malloc(loop_length*6*sizeof(float))))
      ger_fatal("p3dgen: pg_spline_tube: unable to allocate %d floats!\n",
		loop_length*6);
    
    METHOD_RDY(vlist);
    current_clr.ctype= P3D_RGB;
    if (vlist->type == P3D_CCVTX || vlist->type == P3D_CCNVTX) {
      current_clr.r= (*(vlist->r))(0);
      current_clr.g= (*(vlist->g))(0);
      current_clr.b= (*(vlist->b))(0);
      current_clr.a= (*(vlist->a))(0);
    }
    else if (vlist->type == P3D_CVVTX || vlist->type == P3D_CVNVTX
	     || vlist->type == P3D_CVVVTX) {
      float cval= (*(vlist->v))(0);
      (void)pg_cmap_color( &cval, &(current_clr.r), &(current_clr.r), 
			   &(current_clr.r), &(current_clr.r) );
    }
    else {
      current_clr.r= current_clr.g= current_clr.b= current_clr.a= 1.0;
    }

    prim_flag= pg_open(""); /* outer gob */
    if (prim_flag==P3D_SUCCESS) prim_flag= pg_open(""); /* first color gob */
    
    if (prim_flag==P3D_SUCCESS) { 

      dp_gobcolor(&current_clr);

      /* emit first endcap */
      capnorm[0]= capnorm[1]= capnorm[2]= 0.0;
      add_cross(capnorm, mesh_buf, 
		mesh_buf+(2*loop_length/3), mesh_buf+(loop_length/3) );
      v3_normalize(capnorm);
      for (j=0; j<loop_length; j++) {
	endcap_buf[6*j]= mesh_buf[loop_length-(j+1)].x;
	endcap_buf[6*j+1]= mesh_buf[loop_length-(j+1)].y;
	endcap_buf[6*j+2]= mesh_buf[loop_length-(j+1)].z;
	endcap_buf[6*j+3]= capnorm[0];
	endcap_buf[6*j+4]= capnorm[1];
	endcap_buf[6*j+5]= capnorm[2];
      }
      prim_flag= dp_polygon(P3D_CNVTX, P3D_RGB, endcap_buf, loop_length);
    }

    if (prim_flag==P3D_SUCCESS)
      for (i=0; i<skel_length-1; i++) { /* one less tri strip than row */
	int new_clr= 0;
	int k;
	
	closest_vtx= (int)(0.5+(float)(i*vlist->length)/(float)skel_length);
	if (closest_vtx>=vlist->length) closest_vtx= vlist->length-1;
	
	METHOD_RDY(vlist);
	if (vlist->type == P3D_CCVTX || vlist ->type == P3D_CCNVTX) {
	  /* vlist has color info, so may have to update */
	  if (current_clr.ctype != P3D_RGB
	      || current_clr.r != (*(vlist->r))(closest_vtx)
	      || current_clr.g != (*(vlist->g))(closest_vtx)
	      || current_clr.b != (*(vlist->b))(closest_vtx)
	      || current_clr.a != (*(vlist->a))(closest_vtx)) {
	    new_clr= 1;
	    current_clr.ctype= P3D_RGB;
	    current_clr.r= (*(vlist->r))(closest_vtx);
	    current_clr.g= (*(vlist->g))(closest_vtx);
	    current_clr.b= (*(vlist->b))(closest_vtx);
	    current_clr.a= (*(vlist->a))(closest_vtx);
	  }
	}
	else if (vlist->type == P3D_CVVTX || vlist->type == P3D_CVNVTX
		 || vlist->type == P3D_CVVVTX) {
	  /* May need to update color from vtx value info */
	  P_Color tclr;
	  float cval= (*(vlist->v))(closest_vtx);
	  (void)pg_cmap_color( &cval, &(tclr.r), &(tclr.g), 
			       &(tclr.b), &(tclr.a) );
	  if (current_clr.ctype != P3D_RGB
	      || current_clr.r != tclr.r
	      || current_clr.g != tclr.g
	      || current_clr.b != tclr.b
	      || current_clr.a != tclr.a) {
	    new_clr= 1;
	    current_clr.ctype= P3D_RGB;
	    current_clr.r= tclr.r;
	    current_clr.g= tclr.g;
	    current_clr.b= tclr.b;
	    current_clr.a= tclr.a;
	  }
	}
	
	runner= buf;
	for (j=0; j<loop_length; j++) {	
	  norm_runner= (float*)(norm_buf+((i+1)*loop_length+j));
	  mesh_runner= (float*)(mesh_buf+((i+1)*loop_length+j));
	  for (k=0; k<3; k++) *runner++= *mesh_runner++;
	  for (k=0; k<3; k++) *runner++= *norm_runner++;
	  
	  norm_runner= (float*)(norm_buf+(i*loop_length+j));
	  mesh_runner= (float*)(mesh_buf+(i*loop_length+j));
	  for (k=0; k<3; k++) *runner++= *mesh_runner++;
	  for (k=0; k<3; k++) *runner++= *norm_runner++;
	}
	if (new_clr) {
	  dp_close();
	  dp_open("");
	  dp_gobcolor(&current_clr);
	}
	prim_flag= dp_tristrip(P3D_CNVTX, P3D_RGB, buf, 2*loop_length);
	if (prim_flag != P3D_SUCCESS) break;
      }
    
    if (prim_flag==P3D_SUCCESS) {
      /* emit final endcap */
      mesh_runner= (float*)(mesh_buf+(skel_length-1)*loop_length);
      capnorm[0]= capnorm[1]= capnorm[2]= 0.0;
      add_cross(capnorm, (P_Point*)mesh_runner,
		((P_Point*)mesh_runner)+(loop_length/3), 
		((P_Point*)mesh_runner)+(2*loop_length/3) );
      v3_normalize(capnorm);
      for (j=0; j<loop_length; j++) {
	endcap_buf[6*j]= *mesh_runner++;
	endcap_buf[6*j+1]= *mesh_runner++;
	endcap_buf[6*j+2]= *mesh_runner++;
	endcap_buf[6*j+3]= capnorm[0];
	endcap_buf[6*j+4]= capnorm[1];
	endcap_buf[6*j+5]= capnorm[2];
      }
      dp_polygon(P3D_CNVTX, P3D_RGB, endcap_buf, loop_length);
    }

    dp_close(); /* last color gob */

    dp_close(); /* actual model gob */

    free(buf);
    free(endcap_buf);
    return( prim_flag );
  }
  else {
    ger_error("p3dgen: pg_spline_tube: must have a renderer open first");
    return( P3D_FAILURE );
  }
}

static void robust_spl_grad(P_Vector* gradvec, Spline* skel_ctl, float v)
{
  /* We'll try to get the gradient correctly, but it can be zero 
   * if the tension parameter is zero.  If that happens, we'll do a 
   * simple forward difference.  If that too fails, make a guess.
   */
  spl_grad(gradvec, skel_ctl, v);

  if (vec_dot_product(gradvec,gradvec)<=EPSILON) {
    /* Gradient is locally zero */
    float dv= 0.05;
    P_Point herept;
    P_Point steppt;
    spl_calc(&herept, skel_ctl, v);
    spl_calc(&steppt, skel_ctl, v+dv);
    gradvec->x= (steppt.x - herept.x)/dv;
    gradvec->y= (steppt.y - herept.y)/dv;
    gradvec->z= (steppt.z - herept.z)/dv;
  }
}

static void vec_cross(P_Vector* out, P_Vector* v1, P_Vector* v2) 
{
  out->x= v1->y*v2->z - v1->z*v2->y;
  out->y= v1->z*v2->x - v1->x*v2->z;
  out->z= v1->x*v2->y - v1->y*v2->x;
}

static void robust_calc_phase(P_Vector* phasevec, Spline* skel_ctl, 
			      P_Vector* gradvec, float v)
{
  /* Idea is to return a normalized vector in the direction of the
   * cross product of grad and gradsqr, or a zero vector if that
   * fails.
   */
  P_Vector gradsqrvec;
  P_Vector tvec;
  float len;
  float lensqr;

  if (vec_dot_product(gradvec,gradvec)<=EPSILON) {
    /* No hope; let calling routine find a work-around */
    phasevec->x= phasevec->y= phasevec->z= 0.0;
    return;
  }

  spl_gradsqr(&gradsqrvec, skel_ctl, v);

  (void)vec_make_unit(gradvec,gradvec);
  if (vec_length(&gradsqrvec)<=EPSILON) {
    P_Vector tryme;
    tryme.x= 0.0; tryme.y= 1.0; tryme.z= 0.0;
    vec_cross(phasevec, gradvec, &tryme);
    if (vec_dot_product(phasevec,phasevec)<=EPSILON) {
      tryme.x= 1.0; tryme.y= 0.0; tryme.z= 0.0;
      vec_cross(phasevec, gradvec, &tryme);
    }
    if (vec_dot_product(phasevec,phasevec)<=EPSILON) {
      /* OK, I give up */
      phasevec->x= phasevec->y= phasevec->z= 0.0;
      return;
    }
  }
  (void)vec_make_unit(&gradsqrvec, &gradsqrvec);
  vec_sub( phasevec, 
	   &gradsqrvec, vec_scale( &tvec, gradvec,
				   vec_dot_product( gradvec, &gradsqrvec )));
  vec_make_unit(phasevec, phasevec);
}

static void step_skel_trans(Spline* skel_ctl, float v, int first) {
  P_Point herept;
  P_Vector gradvec;
  P_Vector phasevec;
  P_Vector crossvec;
  static P_Vector oldgrad;
  static P_Vector oldphase;
  static P_Vector rotoldphase;
  static P_Vector x_axis= {1.0,0.0,0.0};
  static P_Vector y_axis= {0.0,1.0,0.0};
  P_Transform* align;
  float phase_dot;
  double phase_delta;
  static double prev_phase_delta;

  spl_calc(&herept, skel_ctl, v);

  robust_spl_grad(&gradvec, skel_ctl, v);
  if (vec_dot_product(&gradvec,&gradvec)<=EPSILON) {
    /* Sigh; still zero */
    if (first) {
      gradvec.x= 1.0; gradvec.y= 0.0; gradvec.z= 0.0; /* arbitrary in xy */
    }
    else {
      gradvec.x= oldgrad.x; gradvec.y= oldgrad.y; gradvec.z= oldgrad.z; 
    }
  }
  robust_calc_phase(&phasevec, skel_ctl, &gradvec, v);
  if (vec_dot_product(&phasevec,&phasevec)<=EPSILON) {
    /* Sigh; still zero.  Make up an initial phase. */
    if (first) {
      phasevec.x= 0.0; phasevec.y= 1.0; phasevec.z= 0.0;
    }
    else {
      phasevec.x= oldphase.x; phasevec.y= oldphase.y; phasevec.z= oldphase.z; 
    }
  }
  if (first) {
    /* want identity in skel matrix */
    copy_trans( &skel_matrix, Identity_trans );

    /* want to start with a rot that will flip z direction into grad dir */
    /* and x direction into phase dir */
    oldgrad.x= 0.0; oldgrad.y= 0.0; oldgrad.z= 1.0;
    oldphase.x= 1.0; oldphase.y= 0.0; oldphase.z= 0.0;
  }

  /* Want to tag the new rotation on *before* previous rots */
  align= make_aligning_rotation(&oldgrad,&gradvec);
  premult_trans( align, &skel_matrix );
  destroy_trans(align);
  matrix_transform_affine(&skel_matrix, 
			  (P_Point*)&y_axis, (P_Point*)&rotoldphase);
  phase_dot= vec_dot_product(&rotoldphase, &phasevec);
  if (phase_dot>1.0) phase_dot= 1.0;
  if (phase_dot<-1.0) phase_dot= -1.0;
  if (phase_dot<0.0) {
    /* Use symmetry of the tube to avoid a big rotation */
    phase_dot= -phase_dot;
    vec_scale(&rotoldphase, &rotoldphase, -1);
  }
  phase_delta= acos(phase_dot);
  vec_cross(&crossvec,&rotoldphase, &phasevec);
  if (vec_dot_product(&gradvec, &crossvec) < 0.0) phase_delta *= -1;

  if (first) prev_phase_delta= phase_delta;
  phase_delta= working_phase_smooth*phase_delta
    +(1.0-working_phase_smooth)*prev_phase_delta;
  align= rotate_trans( &gradvec, phase_delta*180.0/M_PI );

  prev_phase_delta= phase_delta;
  premult_trans( align, &skel_matrix );
  destroy_trans(align);

  skel_offset.x= herept.x;
  skel_offset.y= herept.y;
  skel_offset.z= herept.z;

  vec_copy(&oldgrad,&gradvec);
  vec_copy(&oldphase,&phasevec);

}

static void calc_mesh_norms(int loop_length, int skel_length) {
  int i,j;
  P_Point* mhere;
  P_Vector* here;
  float tmp[3];

  if (!loop_length || !skel_length) return;

  /* Sum the normals of all adjacent triangles */
  for (i=0; i<skel_length; i++) {
    for (j=0; j<loop_length; j++) {
      here= norm_buf+(i*loop_length+j);
      mhere= mesh_buf+(i*loop_length+j);
      
      tmp[0]= tmp[1]= tmp[2]= 0.0;

      if (i<(skel_length-1) && j>0) 
	add_cross(tmp, mhere, mhere+loop_length, mhere-1);

      if (i<(skel_length-1) && j<(loop_length-1)) 
	add_cross(tmp, mhere, mhere+1, mhere+loop_length);

      if (i<(skel_length-1) && j==0)
	add_cross(tmp, mhere, mhere+loop_length, mhere+(loop_length-1));

      if (i<(skel_length-1) && j==(loop_length-1))
	add_cross(tmp,mhere, mhere-(loop_length-1), mhere+loop_length);

      if (i>0 && j<(loop_length-1)) 
	add_cross(tmp, mhere, mhere-loop_length, mhere+1);

      if (i>0 && j==(loop_length-1))
	add_cross(tmp, mhere, mhere-loop_length, mhere-(loop_length-1));

      if (i>0 && j>0) 
	add_cross(tmp, mhere, mhere-1, mhere-loop_length);

      if (i>0 && j==0)
	add_cross(tmp, mhere, mhere+(loop_length-1), mhere+(loop_length-1));

      if (loop_length*skel_length > 1) v3_normalize(tmp);
      else {
	tmp[0]= 0.0; tmp[1]= 0.0; tmp[2]= 1.0; /* 1-vertex mesh; paranoia */
      }
      here->x= tmp[0]; here->y= tmp[1]; here->z= tmp[2];
    }
  }
}


static void loop_table_init()
{
  static P_Point circle_coords[]= {
    {-0.500000, -0.866025, 0.000000},
    {1.000000, 0.000000, 0.000000},
    {-0.500000, 0.866025, 0.000000},
    {-0.500000, -0.866025, 0.000000},
    {1.000000, 0.000000, 0.000000},
    {-0.500000, 0.866025, 0.000000},
  };
  static P_Point star_coords[]= {
    {.404, -.294, 0 },
    {1, 0, 0},
    {.404, .294, 0},
    {.309, .951, 0},
    {-.155, .476, 0},
    {-.809, .588, 0},
    {-.5, 0, 0},
    {-.809, -.588, 0},
    {-.155, -.476, 0},
    {.309, -.951, 0},
    {.404, -.294, 0},
    {1, 0, 0},
    {.404, .294, 0}
  };
  static P_Point bigcircle_coords[]= {
    {2.385819, -0.574025, 0.000000},
    {2.500000, 0.000000, 0.000000},
    {2.385819, 0.574025, 0.000000},
    {2.060660, 1.060660, 0.000000},
    {1.574025, 1.385819, 0.000000},
    {1.000000, 1.500000, 0.000000},
    {0.425975, 1.385819, 0.000000},
    {-0.060660, 1.060660, 0.000000},
    {-0.385819, 0.574025, 0.000000},
    {-0.500000, 0.000000, 0.000000},
    {-0.385819, -0.574025, 0.000000},
    {-0.060660, -1.060660, 0.000000},
    {0.425975, -1.385819, 0.000000},
    {1.000000, -1.500000, 0.000000},
    {1.574025, -1.385819, 0.000000},
    {2.060660, -1.060660, 0.000000},
    {2.385819, -0.574025, 0.000000},
    {2.500000, 0.000000, 0.000000},
    {2.385819, 0.574025, 0.000000}
  };
  static P_Point slab_coords[]= {
    {-2.0, -0.75, 0.0},
    {2.0, -0.75, 0.0},
    {2.0, 0.75, 0.0},
    {-2.0, 0.75, 0.0},
    {-2.0, -0.75, 0.0},
    {2.0, -0.75, 0.0},
    {2.0, 0.75, 0.0},
  };

  static P_Point* loops[]= {
    slab_coords, 
    slab_coords,
    circle_coords, 
    bigcircle_coords,
    star_coords
  };
  int loop_counts[]= {7,7,6,19,13};
  float loop_tensions[]= {0.5,0.5,1.50,0.5,0.5};

  int i;

  if (!loop_tbl_initialized) {
    for (i=0; i<N_LOOP_TYPES; i++) {
      loop_tbl[i]= spl_create(loop_counts[i], SPL_CATMULLROM, loops[i]);
      spl_set_tension(loop_tbl[i],loop_tensions[i]);
    }
    loop_tbl_initialized= 1;
  }
}

static Spline* skel_spl_init(P_Vlist *vlist)
{
  Spline* result;
  P_Point* pts;
  int i;

  if (!(pts= (P_Point*)malloc((vlist->length+2)*sizeof(P_Point))))
    ger_fatal("pg_spline_tube: unable to allocate %d bytes!",
	      vlist->length*sizeof(P_Point));

  /* Remember that we have to double the first and last points, since
   * Catmull-Rom splines don't include their first and last control points.
   */
  METHOD_RDY(vlist);
  pts[0].x= (*(vlist->x))(0);
  pts[0].y= (*(vlist->y))(0);
  pts[0].z= (*(vlist->z))(0);
  for (i=0; i<vlist->length; i++) {
    pts[i+1].x= (*(vlist->x))(i);
    pts[i+1].y= (*(vlist->y))(i);
    pts[i+1].z= (*(vlist->z))(i);
  }
  pts[vlist->length+1].x= (*(vlist->x))(vlist->length-1);
  pts[vlist->length+1].y= (*(vlist->y))(vlist->length-1);
  pts[vlist->length+1].z= (*(vlist->z))(vlist->length-1);

  result= spl_create(vlist->length+2, SPL_CATMULLROM, pts);
  spl_set_tension(result,SKEL_TENSION);
  free(pts);
  return(result);
}

static int arrowhead_test( int curve_here, int curve_next )
{
  /* If the two curves are the same, no arrowhead. */
  if (curve_here == curve_next) return 0;
  /* Non-helix Ribbons becoming an unstructured gets an arrowhead */
  else if (curve_next= 3 && curve_here==2)
    return 1;
  else return 0;
}

static void regen_mesh(int skel_length, int loop_length, 
		       Spline* skel_ctl, int* which_cross, P_Vlist* vlist)
{
  int i,j;
  int mesh_length;
  P_Point* here;
  float v;
  float dv;
  float u;
  float du;

  if (!loop_length || !skel_length) return;

  if (loop_length*skel_length>mesh_buf_size) {
    if (mesh_buf) free(mesh_buf);
    if (!(mesh_buf=(P_Point*)malloc(loop_length*skel_length
				    *sizeof(P_Point)))) {
      ger_fatal("Unable to allocate %d bytes!",
		loop_length*skel_length*sizeof(P_Point));
    }
    if (norm_buf) free(norm_buf);
    if (!(norm_buf=(P_Vector*)malloc(loop_length*skel_length
				    *sizeof(P_Vector)))) {
      ger_fatal("Unable to allocate %d bytes!",
		loop_length*skel_length*sizeof(P_Vector));
    }
    mesh_buf_size= loop_length*skel_length;
  }

  v= 0.0;
  dv= 1.0/(float)(skel_length-1);

  for (i=0; i<skel_length; i++) {
    float vtx_off;
    int closest_vtx;
    int next_vtx;

    vtx_off= (0.5+(float)(i*vlist->length)/(float)skel_length);
    closest_vtx= (int)vtx_off;
    vtx_off -= closest_vtx;

    if (closest_vtx>=vlist->length) {
      closest_vtx= vlist->length-1;
      vtx_off= 0.0;
    }

    if (closest_vtx < vlist->length-1) next_vtx= closest_vtx+1;
    else next_vtx= closest_vtx;

    u= 0.0;
    du= 1.0/(float)(loop_length-1);
    step_skel_trans(skel_ctl,v,(i==0));
    if (arrowhead_test(which_cross[closest_vtx], which_cross[next_vtx])) {
      /* Loop scaled for arrowhead */
      for (j=0; j<loop_length; j++) {
	P_Point tp_closest;
	P_Point tp_next;
	P_Point tp_here;
	here= mesh_buf+(i*loop_length+j);
	spl_calc(&tp_closest,loop_tbl[which_cross[closest_vtx]-1],u);
	spl_calc(&tp_next,loop_tbl[which_cross[next_vtx]-1],u);
	tp_here.x= LOOP_SIZE_SCALE*(ARROWHEAD_BARB*(1.0-vtx_off)*tp_closest.x
				    + vtx_off*tp_next.x);
	tp_here.y= LOOP_SIZE_SCALE*(ARROWHEAD_BARB*(1.0-vtx_off)*tp_closest.y
				    + vtx_off*tp_next.y);
	tp_here.z= LOOP_SIZE_SCALE*(ARROWHEAD_BARB*(1.0-vtx_off)*tp_closest.z
				    + vtx_off*tp_next.z);
	matrix_transform_affine(&skel_matrix,&tp_here,here);
	here->x += skel_offset.x;
	here->y += skel_offset.y;
	here->z += skel_offset.z;
	u += du;
      }
    }
    else {
      /* No arrowhead */
      for (j=0; j<loop_length; j++) {
	P_Point tp;
	here= mesh_buf+(i*loop_length+j);
	spl_calc(&tp,loop_tbl[which_cross[closest_vtx]-1],u);
	tp.x *= LOOP_SIZE_SCALE;
	tp.y *= LOOP_SIZE_SCALE;
	tp.z *= LOOP_SIZE_SCALE;
	matrix_transform_affine(&skel_matrix,&tp,here);
	here->x += skel_offset.x;
	here->y += skel_offset.y;
	here->z += skel_offset.z;
	u += du;
      }
    }
    v += dv;
  }

  calc_mesh_norms(loop_length, skel_length);
}

/*
  Builds a generalized cylinder with spline backbone and crosssection
*/
int pg_spline_tube( P_Vlist *vlist, int* which_cross, int bres, int cres )
{
  int loop_length= 0;
  Spline* skel_ctl= NULL;
  int skel_length= 0;
  int success_flag= P3D_FAILURE;
  int i;

  /* Reality check the resolutions */
  if (bres<1) {
    ger_error("pg_spline_tube: backbone resolution too low; call ignored.");
    return(P3D_FAILURE);
  }
  if (cres<4) {
    ger_error("pg_spline_tube: cross section resolution too low; call ignored.");
    return(P3D_FAILURE);
  }

  /* Quit if no gob is open */
  if (!pg_gob_open()) {
    ger_error("pg_spline_tube: No gob is currently open; call ignored.");
    return(P3D_FAILURE);
  }

  /* Quit if vlist is too short, or if an invalid crosssection ID is given */
  if (vlist->length<2) {
    ger_error("pg_spline_tube: not enough control points to build tube!.");
    return(P3D_FAILURE);
  }

  for (i=0; i<vlist->length; i++) 
    if (which_cross[i]<1 || which_cross[i]>N_LOOP_TYPES) {
      ger_error("pg_spline_tube: unknown crossection type %d!\n",
		which_cross[i]);
      return(P3D_FAILURE);
    }

  ger_debug("p3dgen: pg_spline_tube: adding spline tube with backbone of %d vertices",
            vlist->length);

  skel_length= vlist->length * bres;
  loop_length= cres;
  working_phase_smooth= PHASE_ANGLE_SMOOTHING/(float)bres;

  /* Initialize the cross-sectional spline table if necessary */
  loop_table_init();

  /* Get the skeleton spline curve */
  skel_ctl= skel_spl_init(vlist);

  /* Generate mesh and normals */
  regen_mesh(skel_length, loop_length, skel_ctl, which_cross, vlist);

  /* Emit the object.  The vlist is passed to get color info */
  pg_open("");
  success_flag = emit_geom(loop_length,skel_length,vlist);
  pg_close();

  /* Free the skeleton spline */
  spl_destroy(skel_ctl);

  return( success_flag );
}

