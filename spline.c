/****************************************************************************
 * spline.c
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
 * This module provides a simple spline class.  It is derived from the starter
 * code for CMU 15-462 (Intro to Computer Graphics) Assignment 2, which
 * contained the following information:
 *
 *
 * spline.c
 * Starter code for 15-462, Computer Graphics 1,
 * Assignment 2: Generalized Cylinders
 *
 * This is a skeleton for reading in and freeing control points read 
 * from a file.  
 *
 * Feel free to modify the code that is already here.  Just remember that
 * you still have to provide the same functionality (of reading in spline 
 * files).  Also, if you want to add things to your own files (store more
 * data or whatever) just make sure that your reader still works on the 
 * files provided.
 *
 * Chris Rodriguez
 *
 * 4 Feb 1999
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "spline.h"

#define GETXYZ(p3d) &(p3d.x),&(p3d.y),&(p3d.z) 
#define SHOWXYZ(p3d) p3d.x,p3d.y,p3d.z 

/* We need a constant to tell us when to actually worry about a spline
 * calc with a value not between 0.0 and 1.0 .  Some small outside values
 * can arise by rounding errors.
 */
#define LOC_TOL 0.0001

static void* safe_malloc( int nbytes ) {
  void* result;
  if (!(result= malloc(nbytes))) {
    fprintf(stderr,"spline:safe_malloc: Unable to allocate %d bytes!\n",
	    nbytes);
    exit(-1);
  }
  return result;
}

/* Reads in all the points.  It's your job to know what to do with them. */
Spline* spl_read(char *dataname) {
  Spline *temp = (Spline *)safe_malloc(sizeof(Spline));
  int i; /* Number of Control Points */
  FILE *splinefile = fopen(dataname, "r");
  
  if (splinefile == NULL) {
    printf("Cannot open file %s\n", dataname);
    return(NULL);
  } else
    printf("%s file opened.\n", dataname);
  
  fscanf(splinefile, "%d %d\n", &temp->numofpoints, &temp->type);
  
  temp->aux= 0.0;
  temp->points = (P_Point *)safe_malloc(temp->numofpoints * sizeof(P_Point));

  if (temp->type==SPL_BEZIER && (temp->numofpoints % 3) != 1) {
    fprintf(stderr,"Invalid Bezier file has %d points!\n",temp->numofpoints);
    exit(-1);
  }
  
  for (i=0; i<temp->numofpoints; i++) {
    fscanf(splinefile, "%f %f %f\n", GETXYZ(temp->points[i]));
   }
 
  fclose (splinefile);
  return (temp);
}

/* Reads in all the points.  It's your job to know what to do with them. */
Spline* spl_create(int npts_in, SplineType type_in, P_Point* pts_in)
{
  Spline *temp = (Spline *)safe_malloc(sizeof(Spline));
  int i;

  temp->numofpoints= npts_in;
  temp->type= type_in;
  temp->aux= 0.0;
  
  if (temp->type==SPL_BEZIER && (temp->numofpoints % 3) != 1) {
    fprintf(stderr,"Invalid Bezier file has %d points!\n",temp->numofpoints);
    exit(-1);
  }
  
  temp->points = (P_Point *)safe_malloc(temp->numofpoints * sizeof(P_Point));

  for (i=0; i<temp->numofpoints; i++) {
    temp->points[i].x= pts_in[i].x;
    temp->points[i].y= pts_in[i].y;
    temp->points[i].z= pts_in[i].z;
   }
 
  return (temp);
}

void spl_destroy(Spline *cp) {
  free(cp->points);
  free(cp);
}

void spl_set_tension(Spline *sp, float val) {
  sp->aux= val;
}

float spl_get_tension(Spline *sp) {
  return sp->aux;
}

void spl_print(Spline *cp) {
  int i;
  switch(cp->type) {
    case SPL_BEZIER:
      printf("Bezier Spline.\n");
      break;
    case SPL_CATMULLROM:
      printf("Catmull Rom Spline; tension %f\n",cp->aux);
      break;
    default:
      printf("Unknown Spline; tension %f\n\n",cp->aux);
      break;
  }
  printf("Points: \n");
  for(i=0; i<cp->numofpoints; i++)
    printf("%f %f %f\n", SHOWXYZ(cp->points[i]));
}

Spline* spl_copy(Spline *in) {
  int i;
  Spline* out= (Spline*)safe_malloc(sizeof(Spline));
  out->type= in->type;
  out->aux= in->aux;
  out->numofpoints= in->numofpoints;
  out->points= (P_Point*)safe_malloc((in->numofpoints)*sizeof(P_Point));
  for (i=0; i<out->numofpoints; i++) {
    out->points[i]= in->points[i];
  }
  return out;
}

Spline* spl_close_loop(Spline* open) {
  Spline* temp;
  int i;
  temp= (Spline*)safe_malloc(sizeof(Spline));
  temp->type= open->type;
  if (open->type==SPL_CATMULLROM) {
    temp->numofpoints= open->numofpoints+2;
    temp->points= (P_Point*)safe_malloc((temp->numofpoints)*sizeof(P_Point));
    for (i=1;i<=open->numofpoints; i++) 
      temp->points[i]= open->points[i-1];
    temp->points[0]= open->points[open->numofpoints - 1];
    temp->points[temp->numofpoints - 1]= open->points[0];
  }
  else if (open->type==SPL_BEZIER) {
    return spl_copy(open);
  }
  else {
    fprintf(stderr,"Cannot close loop on unimplemented spline type %d\n",
	    open->type);
    return spl_copy(open);
  }
  return temp;
}

void spl_calc( P_Point* out, Spline* ctl, float loc ) {
  P_Point tmp;

  if (loc<0.0 || loc>1.0) {
    if (loc<-1.0*LOC_TOL || loc>1.0+LOC_TOL)
      fprintf(stderr,"Bad loc %f in spl_calc\n",loc);
    if (loc<0.0) loc= 0.0;
    if (loc>1.0) loc= 1.0;
  }

  switch (ctl->type) {
  case SPL_BEZIER:
    {
      float u;
      int nseg;
      int seg;
      float A,B,C,D;
      P_Point* c1;
      P_Point* c2;
      P_Point* c3;
      P_Point* c4;

      nseg= (ctl->numofpoints - 1)/3;
      u= nseg*loc;
      seg= (int)u;
      if (seg==nseg) seg--;
      u -= seg;

      A= (1.0-u)*(1.0-u)*(1.0-u);
      B= 3*u*(1.0-u)*(1.0-u);
      C= 3*u*u*(1.0-u);
      D= u*u*u;

      c1= ctl->points + 3*seg;
      c2= c1+1;
      c3= c2+1;
      c4= c3+1;
      tmp.x= A*c1->x + B*c2->x + C*c3->x + D*c4->x;
      tmp.y= A*c1->y + B*c2->y + C*c3->y + D*c4->y;
      tmp.z= A*c1->z + B*c2->z + C*c3->z + D*c4->z;
      *out= tmp;
    }
    break;
  case SPL_CATMULLROM:
    {
      float u;
      float aux;
      int seg;
      float A,B,C,D;
      P_Point* c1;
      P_Point* c2;
      P_Point* c3;
      P_Point* c4;

      aux= ctl->aux;
      u= ((ctl->numofpoints - 3)*loc);
      seg= (int)u;
      if (seg==ctl->numofpoints-3) seg--;
      u -= seg;
#ifdef never
      A= ((-aux*u*u*u)+(2.0*aux*u*u)+(-aux*u)+0.0);
      B= (((2-aux)*u*u*u)+((aux-3)*u*u)+1.0);
      C= (((aux-2)*u*u*u)+((3.0-2.0*aux)*u*u)+(aux*u));
      D= ((aux*u*u*u)-(aux*u*u));
#endif
      A= ((-aux*u+2.0*aux)*u - aux)*u+0.0;
      B= (((2-aux)*u+(aux-3))*u*u)+1.0;
      C= (((aux-2)*u+(3.0-2.0*aux))*u + aux)*u;
      D= aux*(u-1)*u*u;
      c1= ctl->points + seg;
      c2= c1+1;
      c3= c2+1;
      c4= c3+1;
      tmp.x= A*c1->x + B*c2->x + C*c3->x + D*c4->x;
      tmp.y= A*c1->y + B*c2->y + C*c3->y + D*c4->y;
      tmp.z= A*c1->z + B*c2->z + C*c3->z + D*c4->z;
      *out= tmp;
    }
    break;
  default: 
    {
      fprintf(stderr,"spl_calc: Unimplemented spline type %d\n",ctl->type);
      out->x= out->y= out->z= 0.0;
    }
  }


}

void spl_grad( P_Vector* out, Spline* ctl, float loc ) {
  P_Vector tmp;

  if (loc<0.0 || loc>1.0) {
    if (loc<-1.0*LOC_TOL || loc>1.0+LOC_TOL)
      fprintf(stderr,"Bad loc %f in spl_grad\n",loc);
    if (loc<0.0) loc= 0.0;
    if (loc>1.0) loc= 1.0;
  }

  switch (ctl->type) {
  case SPL_BEZIER:
    {
      float u;
      int nseg;
      int seg;
      float A,B,C,D;
      P_Point* c1;
      P_Point* c2;
      P_Point* c3;
      P_Point* c4;

      nseg= (ctl->numofpoints - 1)/3;
      u= nseg*loc;
      seg= (int)u;
      if (seg==nseg) seg--;
      u -= seg;

      A= -3.0*(1.0-u)*(1.0-u);
      B= (3.0-9.0*u)*(1.0-u);
      C= (6.0-9.0*u)*u;
      D= 3.0*u*u;

      c1= ctl->points + 3*seg;
      c2= c1+1;
      c3= c2+1;
      c4= c3+1;
      tmp.x= A*c1->x + B*c2->x + C*c3->x + D*c4->x;
      tmp.y= A*c1->y + B*c2->y + C*c3->y + D*c4->y;
      tmp.z= A*c1->z + B*c2->z + C*c3->z + D*c4->z;
      *out= tmp;
    }
    break;
  case SPL_CATMULLROM:
    {
      float u;
      float aux;
      int seg;
      float A,B,C,D;
      P_Point* c1;
      P_Point* c2;
      P_Point* c3;
      P_Point* c4;

      aux= ctl->aux;
      u= ((ctl->numofpoints - 3)*loc);
      seg= (int)u;
      if (seg==ctl->numofpoints-3) seg--;
      u -= seg;

#ifdef never
      A= ((-3.0*aux*u*u)+(4.0*aux*u)-aux);
      B= ((3.0*(2-aux)*u*u)+(2.0*(aux-3)*u));
      C= ((3.0*(aux-2)*u*u)+(2.0*(3.0-2.0*aux)*u)+aux);
      D= ((3.0*aux*u*u)-(2.0aux*u));
#endif
      A= ((-3.0*aux*u)+(4.0*aux))*u - aux;
      B= ((3.0*(2-aux)*u)+(2.0*(aux-3)))*u;
      C= ((3.0*(aux-2)*u)+(2.0*(3.0-2.0*aux)))*u + aux;
      D= ((3.0*aux*u)-(2.0*aux))*u;

      c1= ctl->points + seg;
      c2= c1+1;
      c3= c2+1;
      c4= c3+1;
      tmp.x= A*c1->x + B*c2->x + C*c3->x + D*c4->x;
      tmp.y= A*c1->y + B*c2->y + C*c3->y + D*c4->y;
      tmp.z= A*c1->z + B*c2->z + C*c3->z + D*c4->z;
      *out= tmp;
    }
    break;
  default: 
    {
      fprintf(stderr,"spl_grad: Unimplemented spline type %d\n",ctl->type);
      out->x= out->y= out->z= 0.0;
    }
  }


}
void spl_gradsqr( P_Vector* out, Spline* ctl, float loc ) {
  P_Vector tmp;

  if (loc<0.0 || loc>1.0) {
    if (loc<-1.0*LOC_TOL || loc>1.0+LOC_TOL)
      fprintf(stderr,"Bad loc %f in spl_gradsqr\n",loc);
    if (loc<0.0) loc= 0.0;
    if (loc>1.0) loc= 1.0;
  }

  switch (ctl->type) {
  case SPL_BEZIER:
    {
      float u;
      int nseg;
      int seg;
      float A,B,C,D;
      P_Point* c1;
      P_Point* c2;
      P_Point* c3;
      P_Point* c4;

      nseg= (ctl->numofpoints - 1)/3;
      u= nseg*loc;
      seg= (int)u;
      if (seg==nseg) seg--;
      u -= seg;

      A= 6.0*(1.0-u);
      B= (-12.0+18.0*u);
      C= (6.0-18.0*u);
      D= 6.0*u;

      c1= ctl->points + 3*seg;
      c2= c1+1;
      c3= c2+1;
      c4= c3+1;
      tmp.x= A*c1->x + B*c2->x + C*c3->x + D*c4->x;
      tmp.y= A*c1->y + B*c2->y + C*c3->y + D*c4->y;
      tmp.z= A*c1->z + B*c2->z + C*c3->z + D*c4->z;
      *out= tmp;
    }
    break;
  case SPL_CATMULLROM:
    {
      float u;
      float aux;
      int seg;
      float A,B,C,D;
      P_Point* c1;
      P_Point* c2;
      P_Point* c3;
      P_Point* c4;

      aux= ctl->aux;
      u= ((ctl->numofpoints - 3)*loc);
      seg= (int)u;
      if (seg==ctl->numofpoints-3) seg--;
      u -= seg;

      A= (-6.0*aux*u)+(4.0*aux);
      B= (6.0*(2-aux)*u)+(2.0*(aux-3.0));
      C= (6.0*(aux-2)*u)+(2.0*(3.0-2.0*aux));
      D= (6.0*aux*u)-(2.0*aux);

      c1= ctl->points + seg;
      c2= c1+1;
      c3= c2+1;
      c4= c3+1;
      tmp.x= A*c1->x + B*c2->x + C*c3->x + D*c4->x;
      tmp.y= A*c1->y + B*c2->y + C*c3->y + D*c4->y;
      tmp.z= A*c1->z + B*c2->z + C*c3->z + D*c4->z;
      *out= tmp;
    }
    break;
  default: 
    {
      fprintf(stderr,"spl_grad: Unimplemented spline type %d\n",ctl->type);
      out->x= out->y= out->z= 0.0;
    }
  }


}

