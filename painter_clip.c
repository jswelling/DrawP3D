/****************************************************************************
 * painter_clip.c
 * Author Chris Nuuja
 * Copyright 1991, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides renderer methods for the Painter's Algorithm renderer.
*/

#include <stdio.h>
#include <math.h>
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "gen_paintr_strct.h"
#include "gen_painter.h"


/*
   Global Data Used: NONE
   Expl:  Called by clip_Zline/poly to determine if a z-coordinate value is 
	  within a boundry, either HITHER  or YON;
*/
int pnt_insideZbound(P_Renderer *self, float z1, int edgenum)
{
	switch(edgenum)
		{
		case HITHER_PLANE:
			if (z1>ZMIN(self))	return(1);
			else 			return(0);
		case YON_PLANE:
			if (z1<ZMAX(self)) 	return(1);
			else 			return(0);
		default:
			fprintf(stderr,"zbound-val error \n");
			break;
		}
	return(0);
}

/*
   Global Data Used:  none
   Expl:  used by clip_Zline/poly to find the intersection of
	  (<x1>,<y1>,<z1>) and (<x2>,<y2>,<z2>) with the plane
	  <edgeNum> (valued HITHER_PLANE or YON_PLANE)
*/
static void Zintersect(float zmax, float zmin, float x1, float y1, float z1,
		       float x2, float y2, float z2, int edgeNum,
		       float *interX, float *interY, float *interZ)
{
	float dx,dy,dz,m;
	
	dx =  x2-x1;
	dy =  y2-y1;
	dz =  z2-z1;
	    
	switch(edgeNum)
		{
		case YON_PLANE:
			if (dz == 0.0)
	    			m = 0.0;
			else
	    			m = (zmax - z1)/dz;
			*interX = x1 + m*dx;
			*interY = y1 + m*dy;
			*interZ = zmax;
			break;
		case HITHER_PLANE:
			if (dz == 0.0)
	    			m = 0.0;
			else
	    			m = (zmin-z1)/dz;
			*interZ = zmin;
			*interX = x1 + m*dx;
			*interY = y1 + m*dy;
			break;
		default:
			fprintf(stderr,"Unknown plane in Zintersect\n");
			break;
		}
}

/*
   Global Data Used: pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer,
		     pnt_Xclip_buffer, pnt_Yclip_buffer, pnt_Zclip_buffer
   Expl: Clips every consecutive pair of vertices in a polyrecord
	 against the plane <edgeNum> (either HITHER_PLANE or YON_PLANE).  
	 The polyrecord's vertices are located in pnt_Xcoord_buffer, 
	 pnt_Ycoord_buffer, and pnt_Zcoord_buffer if <edgenum> is 
	 HITHER_PLANE. If <edgenum> is YON_PLANE, then they are in 
	 pnt_Xclip_buffer, pnt_Yclip_buffer, and pnt_Zclip_buffer.  
	 The clipped points are stored in the coordinate buffers
	 that did not originally contain the points.  It is important, 
	 therefore, that the HITHER_PLANE is always clipped before the 
	 YON_PLANE is. The average z value for this polyrecord is 
	 computed in this procedure, and the pointer <zdepth> saves it.
*/
void pnt_clip_Zpolygon(P_Renderer *self, int edgeNum, int numcoords,
		   int *newnumcoords, float *zdepth)
{
  int old_points=0,new_points=0;
  float newx,newy,newz;
  float *xbuff_old,*ybuff_old,*zbuff_old,
  *xbuff_new,*ybuff_new,*zbuff_new;
  float zmax, zmin;
  
  *zdepth = 0.0;	/* clear out any old value of zdepth  */
  ger_debug("pnt_clip_Zpolygon with %d points",numcoords);
  if (numcoords < 2)  
    {	   
      *newnumcoords = 0;
      return;
    };
  
  /* Grab local copies of clipping distances */
  zmax= ZMAX(self);
  zmin= ZMIN(self);
  
  /* Figure out which buffer holds the old values, and which gets 
   * the new values 
   */
  if (edgeNum == HITHER_PLANE)
    {
      xbuff_old = pnt_Xcoord_buffer;
      ybuff_old = pnt_Ycoord_buffer;
      zbuff_old = pnt_Zcoord_buffer;
      xbuff_new = pnt_Xclip_buffer;
      ybuff_new = pnt_Yclip_buffer;
      zbuff_new = pnt_Zclip_buffer;
    }
  else
    {
      xbuff_old = pnt_Xclip_buffer;
      ybuff_old = pnt_Yclip_buffer;
      zbuff_old = pnt_Zclip_buffer;
      xbuff_new = pnt_Xcoord_buffer;
      ybuff_new = pnt_Ycoord_buffer;
      zbuff_new = pnt_Zcoord_buffer;
    }
  /*  loop over all points in polyrecord */
  while (old_points < numcoords)
    {
      /* Points inside */
      while ( (old_points < numcoords) && 
	     (pnt_insideZbound(self,zbuff_old[old_points],edgeNum)))
	{
	  xbuff_new[new_points]=xbuff_old[old_points];
	  ybuff_new[new_points]=ybuff_old[old_points];
	  *zdepth += zbuff_old[old_points];
	  zbuff_new[new_points++]=zbuff_old[old_points++];
	}
      
      if (old_points < numcoords)
	{
	  /*	Intersection point   	*/
	  if (old_points > 0)
	    {
	      Zintersect(zmax, zmin,
			 xbuff_old[old_points-1],
			 ybuff_old[old_points-1], 
			 zbuff_old[old_points-1],
			 xbuff_old[old_points], 
			 ybuff_old[old_points], 
			 zbuff_old[old_points],edgeNum,
			 &newx,&newy,&newz);
	      xbuff_new[new_points]=newx;
	      ybuff_new[new_points]=newy;
	      *zdepth += newz;
	      zbuff_new[new_points++]=newz;
	    }
	  
	  /*	Points outside		*/
	  while ( (old_points < numcoords) && 
		 (!(pnt_insideZbound(self,zbuff_old[old_points],
				     edgeNum))))
	    {
	      old_points++;
	    }
	  
	  /*	Intersection point	*/
	  if (old_points < numcoords) 
	    {
	      Zintersect(zmax, zmin,
			 xbuff_old[old_points],
			 ybuff_old[old_points], 
			 zbuff_old[old_points], 
			 xbuff_old[old_points-1], 
			 ybuff_old[old_points-1], 
			 zbuff_old[old_points-1],edgeNum,
			 &newx,&newy,&newz);
	      xbuff_new[new_points]=newx;
	      ybuff_new[new_points]=newy;
	      *zdepth += newz;
	      zbuff_new[new_points++]=newz;
	    }
	  else
	    {
	      /* first point inside, last point outside ? */
	      if (pnt_insideZbound(self,zbuff_old[0],edgeNum))
		{
		  Zintersect( zmax, zmin,
			     xbuff_old[numcoords-1],
			     ybuff_old[numcoords-1], 
			     zbuff_old[numcoords-1], 
			     xbuff_old[0],
			     ybuff_old[0], 
			     zbuff_old[0],edgeNum, 
			     &newx,&newy,&newz
			     );
		  xbuff_new[new_points]=newx;
		  ybuff_new[new_points]=newy;
		  *zdepth += newz;
		  zbuff_new[new_points++]=newz;
		}
	    }
	}
    }
  /* check if first point outside, last point inside */
  if ( (pnt_insideZbound(self,zbuff_old[numcoords-1],edgeNum)) &&
      (!pnt_insideZbound(self,zbuff_old[0],edgeNum)) )
    {
      Zintersect( zmax, zmin,
		 xbuff_old[numcoords-1],
		 ybuff_old[numcoords-1], 
		 zbuff_old[numcoords-1], 
		 xbuff_old[0],
		 ybuff_old[0], 
		 zbuff_old[0],edgeNum, &newx,&newy,&newz);
      xbuff_new[new_points]=newx;
      ybuff_new[new_points]=newy;
      *zdepth += newz;
      zbuff_new[new_points++]=newz;
    }
  *newnumcoords = new_points;
  if (new_points != 0)
    *zdepth = *zdepth / (float) new_points;

}

void pnt_clip_Zline(P_Renderer *self, int edgeNum, int numcoords,
		int *newnumcoords, float *zdepth)
{
  int old_points=0,new_points=0;
  float newx,newy,newz,*xbuff_old,*ybuff_old,*zbuff_old,
  *xbuff_new,*ybuff_new,*zbuff_new;
  float zmax, zmin;
  
  *zdepth = 0.0;
  ger_debug("pnt_clip_Zline with %d points",numcoords);
  ger_debug("        Zmax:%f, Zmin:%f \n",ZMAX(self),ZMIN(self));
  if (numcoords < 2)
    {
      *newnumcoords = 0;
      return;
    };
  
  /* Grab local copies of clipping distances */
  zmax= ZMAX(self);
  zmin= ZMIN(self);
  
  if (edgeNum == HITHER_PLANE)
    {
      xbuff_old = pnt_Xcoord_buffer;
      ybuff_old = pnt_Ycoord_buffer;
      zbuff_old = pnt_Zcoord_buffer;
      xbuff_new = pnt_Xclip_buffer;
      ybuff_new = pnt_Yclip_buffer;
      zbuff_new = pnt_Zclip_buffer;
    }
  else
    {
      xbuff_old = pnt_Xclip_buffer;
      ybuff_old = pnt_Yclip_buffer;
      zbuff_old = pnt_Zclip_buffer;
      xbuff_new = pnt_Xcoord_buffer;
      ybuff_new = pnt_Ycoord_buffer;
      zbuff_new = pnt_Zcoord_buffer;
    }
  while (old_points < numcoords)
    {
      /* Points inside */
      while ( (old_points < numcoords) && 
	     (pnt_insideZbound(self,zbuff_old[old_points],edgeNum)))
	{
	  xbuff_new[new_points]=xbuff_old[old_points];
	  ybuff_new[new_points]=ybuff_old[old_points];
	  *zdepth += zbuff_old[old_points];
	  zbuff_new[new_points++]=zbuff_old[old_points++];
	}
      
      if (old_points < numcoords)
	{
	  /*	Intersection point   	*/
	  if (old_points > 0)
	    {
	      Zintersect(zmax, zmin,
			 xbuff_old[old_points-1],
			 ybuff_old[old_points-1], 
			 zbuff_old[old_points-1],
			 xbuff_old[old_points], 
			 ybuff_old[old_points], 
			 zbuff_old[old_points],edgeNum,
			 &newx,&newy,&newz);
	      xbuff_new[new_points]=newx;
	      ybuff_new[new_points]=newy;
	      *zdepth += newz;
	      zbuff_new[new_points++]=newz;
	    }
	  
	  /*	Points outside		*/
	  while ( (old_points < numcoords) && 
		 (!(pnt_insideZbound(self,zbuff_old[old_points],
				 edgeNum))))
	    {
	      old_points++;
	    }
	  
	  /*	Intersection point	*/
	  if (old_points < numcoords) 
	    {
	      Zintersect(zmax, zmin,
			 xbuff_old[old_points],
			 ybuff_old[old_points], 
			 zbuff_old[old_points], 
			 xbuff_old[old_points-1], 
			 ybuff_old[old_points-1], 
			 zbuff_old[old_points-1],edgeNum,
			 &newx,&newy,&newz);
	      xbuff_new[new_points]=newx;
	      ybuff_new[new_points]=newy;
	      *zdepth += newz;
	      zbuff_new[new_points++]=newz;
	    }
	}
    }
  *newnumcoords = new_points;
  if (new_points != 0)
    *zdepth /= (float) new_points;

}
/*   OLD 2-D clipping routines		*/

/*
static void intersect(x1,y1,z1,x2,y2,z2,edgeNum,windNum,interX,interY,interZ)
float x1,y1,z1,x2,y2,z2,*interX,*interY,*interZ;
int edgeNum,windNum;
{
	 float dx,dy,dz,m,bminc,bminr,sminc,sminr,bmaxc,bmaxr,smaxc,smaxr;
	
	
	dx =  x2-x1;
	dy =  y1-y2;
	dz =  z1-z2;
	if (dx == 0.0)
	    m = HUGE;
	else
	    m = dy/dx;
	bminc = (float) bpicw_minc + 1.0;
	sminc = (float) spicw_minc + 1.0;
	bmaxc = (float) bpicw_maxc - 1.0;
	smaxc = (float) spicw_maxc - 1.0;
	bminr = (float) bpicw_minr + 1.0;
	sminr = (float) spicw_minr + 1.0;
	bmaxr = (float) bpicw_maxr - 1.0;
	smaxr = (float) spicw_maxr - 1.0;
	switch(edgeNum)
		{
		case 0:
			if (windNum==1) 
				{
				*interX = bminc;
				*interY = y1 - 
				    m*(bminc - x1);
				*interZ = z1 - 
				    m*(bminc - x1);
				}
			else
				{
				*interX = sminc;
				*interY = y1 - 
				   m*(sminc - x1);
				*interZ = z1 - 
				   m*(sminc - x1);
				}
			break;
		case 1:
			if (windNum==1) 
				{
				*interX = bmaxc;
				*interY = y1 - 
				   m*(bmaxc - x1);
				*interZ = z1 - 
				   m*(bmaxc - x1);
				}
			else
				{
				*interX = smaxc;
				*interY = y1 - 
				   m*(smaxc - x1);
				*interZ = z1 - 
				   m*(smaxc - x1);
				}
			break;
		case 2:
			if (windNum==1) 
				{
				*interX = x1 + 
				   (y1-bminr)/m;	
				*interZ = z1 + 
				   (y1-bminr)/m;	
				*interY = bminr;
				}
			else
				{
				*interX = x1 + 
				   (y1-sminr)/m;	
				*interZ = z1 + 
				   (y1-sminr)/m;	
				*interY = sminr;
				}
			break;
		case 3:
			if (windNum==1) 
				{
				*interX = x1 + 
				   (y1-bmaxr)/m;	
				*interZ = z1 + 
				   (y1-bmaxr)/m;	
				*interY = bmaxr;
				}
			else
				{
				*interX = x1 + 
				   (y1-smaxr)/m;	
				*interZ = z1 + 
				   (y1-smaxr)/m;	
				*interY = smaxr;
				}
			break;
		default:
			printf("ERROR \n");
			break;
		}
}


static int inside(x1,y1,edgenum,windNum)
float x1,y1;
int edgenum,windNum;
{
	int x,y;

	x= (int) x1;
	y= (int) y1;
	switch(edgenum)
		{
			if (windNum == 1)
				if (x>bpicw_minc) 	return(1);
				else 			return(0);
			else 
				if (x>spicw_minc) 	return(1);
				else 			return(0);
			if (windNum == 1)
				if (x<bpicw_maxc) 	return(1);
				else 			return(0);
			else 
				if (x<spicw_maxc) 	return(1);
				else 			return(0);

		case 2:	
			if (windNum == 1)
				if (y>bpicw_minr) 	return(1);
				else 			return(0);
			else 
				if (y>spicw_minr) 	return(1);
				else 			return(0);
		case 3:	
			if (windNum == 1)
				if (y<bpicw_maxr) 	return(1);
				else 			return(0);
			else 
				if (y<spicw_maxr) 	return(1);
				else 			return(0);
		default:
			printf("ERROR2 \n");
			break;
		}
}

static clip_edge(poly,edgeNum,new_poly,windNum)
Pnt_DPolytype *poly,*new_poly;
int edgeNum,windNum;
{
	int old_points,totPoints,new_points=0;
	float newx,newy,newz;

	old_points = 0;
	totPoints = poly->numcoords2d;
	while (old_points < totPoints)
		{
		while ( (old_points < totPoints) && (inside(
		   poly->xcoord2d[old_points],poly->ycoord2d[old_points],
		   edgeNum,windNum)))
			{
			new_poly->xcoord2d[new_points] = poly->xcoord2d[old_points];
			new_poly->ycoord2d[new_points] = poly->ycoord2d[old_points];
			new_poly->zcoord2d[new_points++] = poly->zcoord2d[old_points];
			old_points++;
			}
		if (old_points < totPoints)
			{
			if (old_points > 0)
				{
				intersect(poly->xcoord2d[old_points-1],
				   poly->ycoord2d[old_points-1], 
				   poly->zcoord2d[old_points-1],
				   poly->xcoord2d[old_points], 
				   poly->ycoord2d[old_points], 
				   poly->zcoord2d[old_points],
				   edgeNum,windNum, &newx,&newy,&newz);
				new_poly->xcoord2d[new_points]=newx;
				new_poly->ycoord2d[new_points]=newy;
				new_poly->zcoord2d[new_points++] =newz;
			        }
			while ( (old_points < totPoints) && (!(inside(
			   poly->xcoord2d[old_points],poly->ycoord2d[old_points],
			   edgeNum,windNum))))
				{
				old_points++;
				}
			if (old_points < totPoints) 
				{
				intersect(poly->xcoord2d[old_points],
				   poly->ycoord2d[old_points], 
				   poly->zcoord2d[old_points], 
				   poly->xcoord2d[old_points-1], 
				   poly->ycoord2d[old_points-1], 
				   poly->zcoord2d[old_points-1], 
			           edgeNum,windNum, &newx,&newy,&newz);
				new_poly->xcoord2d[new_points]=newx;
				new_poly->ycoord2d[new_points]=newy;
				new_poly->zcoord2d[new_points++] = newz;
				}
			else
				{
				if (inside(poly->xcoord2d[0], poly->ycoord2d[0],
			    	    edgeNum,windNum))
					{
					intersect(poly->xcoord2d[totPoints-1],
				   	   poly->ycoord2d[totPoints-1], 
				   	   poly->zcoord2d[totPoints-1], 
				   	   poly->xcoord2d[0],
					   poly->ycoord2d[0], 
					   poly->zcoord2d[0], 
				   	   edgeNum,windNum, &newx,&newy,&newz);
					new_poly->xcoord2d[new_points]=newx;
					new_poly->ycoord2d[new_points]=newy;
					new_poly->zcoord2d[new_points++] = newz;
					}
				}
			}
		}
	if ( (inside(poly->xcoord2d[totPoints-1],
	            poly->ycoord2d[totPoints-1],edgeNum,windNum)) &&
	     (!inside(poly->xcoord2d[0],poly->ycoord2d[0],edgeNum,
		     windNum)) )
			{
			intersect(poly->xcoord2d[totPoints-1],
			   poly->ycoord2d[totPoints-1], 
			   poly->zcoord2d[totPoints-1], 
			   poly->xcoord2d[0],
			   poly->ycoord2d[0], 
			   poly->zcoord2d[0], 
			   edgeNum,windNum, &newx,&newy,&newz);
			new_poly->xcoord2d[new_points]=newx;
			new_poly->ycoord2d[new_points]=newy;
			new_poly->zcoord2d[new_points++] = newz;
			}
	new_poly->numcoords2d=new_points;
	new_poly->colorind = poly->colorind;
}
*/






