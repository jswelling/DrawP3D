#include <math.h>
#include <stdio.h>
#include "p3dgen.h"
#include "ge_error.h"
#include "dirichlet.h"

#define DIM 3
#define NPTS 1000

#ifdef never
static float center[DIM]= { 0.5, 0.5 };
static float range[DIM]= {1.0, 1.0};
static float center[DIM]= { 0.5, 0.5, 0.5 };
static float range[DIM]= {1.0, 1.0, 1.0};
#endif
static float center[DIM]= { 1.0, 2.0, 3.0 };
static float range[DIM]= {0.5, 0.75, 1.0};

static float *access_coords( float *coorddata, int i, P_Void_ptr *user_hook )
/* This function returns the i'th set of coordinates from the list */
{
  return( coorddata + i*DIM );
}

static float frandom()
{
  return( (float)(0.5*(double)random()/(double)(2<<30-1)) );
}

static void random_fill_data( float *data, float *center, float *range )
{
  int i, j, coord_offset= 0;

  for (i=0; i<NPTS; i++)
    for (j=0; j<DIM; j++) {
      *data++= *(center+coord_offset) 
	+ (frandom()-0.5) * *(range+coord_offset);
      coord_offset= (coord_offset+1) % DIM;
    }
}

static void ordered_fill_data( float *data, float *center, float *range )
{
  int i, j;
  int imax, jmax;
  float x, y, xmin, ymin, xstep, ystep;

  if (DIM!=2) ger_fatal("ordered_fill_data only works with dimension 2");

  imax= sqrt(NPTS);
  jmax= NPTS/imax;
  fprintf(stderr,"ordered_fill_data: imax= %d, jmax= %d\n",imax,jmax);
  xstep= range[0]/imax;
  ystep= range[1]/jmax;
  xmin= center[0] - 0.5*((imax-1)*xstep);
  ymin= center[1] - 0.5*((jmax-1)*ystep);

  x= xmin;
  y= ymin;
  for (i=0; i<imax; i++) {
    for (j=0; j<jmax; j++) {
fprintf(stderr,"%d %d: ( %f %f )\n",i,j,x,y);
      *data++= x;
      *data++= y;
      y += ystep;
    }
    x += xstep;
    y= ymin;
  }
}

static void ordered_3d_fill_data( float *data, float *center, float *range )
{
  int i, j, k;
  int imax, jmax, kmax;
  float x, y, z, xmin, ymin, zmin, xstep, ystep, zstep;

  if (DIM!=3) ger_fatal("ordered_3d_fill_data only works with dimension 3");

  for (i=0; ((i*i*i)<NPTS); i++);

  imax= i;
  jmax= NPTS/(imax*imax);
  kmax= NPTS/(imax*jmax);
  fprintf(stderr,"ordered_3d_fill_data: imax= %d, jmax= %d, kmax= %d\n",
	  imax,jmax,kmax);
  xstep= range[0]/imax;
  ystep= range[1]/jmax;
  zstep= range[2]/kmax;
  xmin= center[0] - 0.5*((imax-1)*xstep);
  ymin= center[1] - 0.5*((jmax-1)*ystep);
  zmin= center[2] - 0.5*((kmax-1)*zstep);

  x= xmin;
  y= ymin;
  z= zmin;
  for (i=0; i<imax; i++) {
    for (j=0; j<jmax; j++) {
      for (k=0; k<kmax; k++) {
	*data++= x;
	*data++= y;
	*data++= z;
	z += ystep;
      }
      y += ystep;
      z= zmin;
    }
    x += xstep;
    y= ymin;
  }
}

static void print_statistics( dch_Tess *tess )
{
  dch_Vtx_list *vlist1, *vlist2;
  dch_Pt_list *plist1, *plist2;
  int vtxs_in_main_list= 0, vtxs_in_neighbor_lists= 0, vtxs_in_vert_lists= 0;
  int pts_in_main_list= 0, pts_in_fp_lists= 0, pts_in_neighbor_lists= 0;
  int vtx_total, pt_total;

  vlist1= tess->vtx_list;
  while (vlist1) {
    vtxs_in_main_list++;
    vlist2= vlist1->vtx->neighbors;
    while (vlist2) {
      vtxs_in_neighbor_lists++;
      vlist2= vlist2->next;
    }
    plist1= vlist1->vtx->forming_pts;
    while (plist1) {
      pts_in_fp_lists++;
      plist1= plist1->next;
    }
    vlist1= vlist1->next;
  }

  plist1= tess->pt_list;
  while (plist1) {
    pts_in_main_list++;
    plist2= plist1->pt->neighbors;
    while (plist2) {
      pts_in_neighbor_lists++;
      plist2= plist2->next;
    }
    vlist1= plist1->pt->verts;
    while (vlist1) {
      vtxs_in_vert_lists++;
      vlist1= vlist1->next;
    }
    plist1= plist1->next;
  }

  vtx_total= vtxs_in_main_list + vtxs_in_neighbor_lists + vtxs_in_vert_lists;
  fprintf(stderr,"Vertex list statistics: %d total\n", vtx_total);
  fprintf(stderr,"       in main list:        %d (%f of total)\n",
	  vtxs_in_main_list, ((float)vtxs_in_main_list)/((float)vtx_total));
  fprintf(stderr,"       in neighbor lists:   %d (%f of total)\n",
	  vtxs_in_neighbor_lists, 
	  ((float)vtxs_in_neighbor_lists)/((float)vtx_total));
  fprintf(stderr,"       in pt vertex lists:  %d (%f of total)\n",
	  vtxs_in_vert_lists, ((float)vtxs_in_vert_lists)/((float)vtx_total));
  
  pt_total= pts_in_main_list + pts_in_neighbor_lists + pts_in_fp_lists;
  fprintf(stderr,"Point list statistics:  %d total\n", pt_total);
  fprintf(stderr,"       in main list:        %d (%f of total)\n",
	  pts_in_main_list, ((float)pts_in_main_list)/((float)pt_total));
  fprintf(stderr,"       in neighbor lists:   %d (%f of total)\n",
	  pts_in_neighbor_lists, 
	  ((float)pts_in_neighbor_lists)/((float)pt_total));
  fprintf(stderr,"       in forming pt lists: %d (%f of total)\n",
	  pts_in_fp_lists, ((float)pts_in_fp_lists)/((float)pt_total));
  
}

main()
{
  dch_Tess *tess;
  static float data[NPTS*DIM];

  srandom(1);
  random_fill_data(data,center,range);

  tess= dch_create_dirichlet_tesselation(data, NPTS, DIM, access_coords);

  print_statistics( tess );

  dch_destroy_tesselation(tess);

}
