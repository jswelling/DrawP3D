/****************************************************************************
 * painter.c
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
/* This module is the core of the Painter renderer */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "gen_paintr_strct.h"
#include "gen_painter.h"

/* 
The following is designed to figure out what machine this is running
on, and include appropriate definition files.
*/
#ifdef unix
#define USE_UNIX
#endif
#ifdef CRAY
#undef USE_UNIX
#endif
#ifdef ardent
#undef USE_UNIX
#endif
#ifdef CRAY
#include "unicos_defs.h"
#endif
#ifdef ardent
#include "unicos_defs.h"
#endif
#ifdef USE_UNIX
#include "unix_defs.h"
#endif


/* This renderer caches some transformation information in local memory,
 * to accelerate rendering traversal.  The current_renderer variable
 * is used to test to see if these caches need to be updated.
 */
static P_Renderer *current_renderer= (P_Renderer *)0;

/* The following structures are used across all instances of the renderer.
 * They are buffers for storing coordinates in the process of being
 * transformed from world to screen space and clipped.
 */
int TempCoordBuffSz= INITIAL_TEMP_COORDS;
/*  Coordinate buffers used for transformations and clipping  */
float *pnt_Xcoord_buffer,*pnt_Ycoord_buffer,*pnt_Zcoord_buffer;
float *pnt_Xclip_buffer, *pnt_Yclip_buffer, *pnt_Zclip_buffer;

/*  
 * The following variables hold the values  of the transformation
 * matrix EyeToImage, the eye coordinates to 2d (virtual) screen coordinate
 * transform, during rendering traversal.  They are stored this way
 * to accelerate rendering;  the rest of the time they live in the
 * renderer's EyeMatrix slot.  The value at EI[3][3] is held in 
 * (3+3*4 =) EI15.  The current_renderer slot is used to identify when
 * these variables need to be read out of EyeMatrix.
 */
static float EI0,  EI1,  EI2,  EI3,   EI4,   EI5,   EI6,   EI7;
static float EI8,  EI9,  EI10, EI11,  EI12,  EI13,  EI14,  EI15;

/* 
   Utility function to enlarge the size of the temporary coordinate buffers if
   needed
*/
static void check_coordbuffsz(P_Renderer *self, int numcoords)
{
  if (numcoords >= TempCoordBuffSz) {
    while(numcoords >= TempCoordBuffSz)
      TempCoordBuffSz  *= 2;
    pnt_Xcoord_buffer = (float *) 
      realloc(pnt_Xcoord_buffer,TempCoordBuffSz*sizeof(float));
    pnt_Ycoord_buffer = (float *) 
      realloc(pnt_Ycoord_buffer,TempCoordBuffSz*sizeof(float));
    pnt_Zcoord_buffer = (float *) 
      realloc(pnt_Zcoord_buffer,TempCoordBuffSz*sizeof(float));
    pnt_Xclip_buffer = (float *) 
      realloc(pnt_Xclip_buffer,TempCoordBuffSz*sizeof(float));
    pnt_Yclip_buffer = (float *) 
      realloc(pnt_Yclip_buffer,TempCoordBuffSz*sizeof(float));
    pnt_Zclip_buffer = (float *) 
      realloc(pnt_Zclip_buffer,TempCoordBuffSz*sizeof(float));
    if ( !pnt_Xcoord_buffer || !pnt_Ycoord_buffer || !pnt_Zcoord_buffer
	|| !pnt_Xclip_buffer || !pnt_Yclip_buffer || !pnt_Zclip_buffer )
      ger_fatal("xpainter: pnt_init_renderer: memory allocation failed!\n");
  }
}

/*
   Global Data Used: pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer
   Expl:  This routine adds a record to the depth buffer, which is later
	  sorted to achieve painters algorithm for hidden surface handeling.
	  It creates a polygon record using the coordinates in the
	  global coordinate buffers and the input parameters.  This polygon
	  is then added to DepthBuffer at index DepthCount.
*/
static void insert_depthBuff(P_Renderer *self, int numcoords, float zdepth,
		      primtype type, int color)
{
  int i,newpoly,x_index,y_index,z_index;
  
  newpoly = pnt_new_DepthPoly(self);
  pnt_fillDpoly_rec(self,newpoly,numcoords,type);
  
  x_index = DPOLYBUFFER(self)[newpoly].x_index;
  y_index = DPOLYBUFFER(self)[newpoly].y_index;
  z_index = DPOLYBUFFER(self)[newpoly].z_index;
  for (i=0;i<numcoords;i++) {
    DCOORDBUFFER(self)[x_index++] = pnt_Xcoord_buffer[i];
    DCOORDBUFFER(self)[y_index++] = pnt_Ycoord_buffer[i];
    DCOORDBUFFER(self)[z_index++] = pnt_Zcoord_buffer[i];
  };
  DEPTHBUFFER(self)[DEPTHCOUNT(self)].poly = newpoly;
  DEPTHBUFFER(self)[DEPTHCOUNT(self)].key = zdepth;
  DEPTHCOUNT(self)++;
  DPOLYBUFFER(self)[newpoly].color = color;
  if (DEPTHCOUNT(self) > MAXDEPTHPOLY(self))
    printf("ERROR: Depth Buffer Overflow: %d > %d\n",
	   DEPTHCOUNT(self), MAXDEPTHPOLY(self));
  
}

/*
   Global Data Used: none

   Expl:  This routine alters the intensity of <poly>'s color record
	  depending on its orientation to the lighting source(s).  
*/
static int calc_intensity(P_Renderer *self, Pnt_Colortype *oldcolor, 
			  Pnt_Polytype *polygon, float *trans, 
			  Pnt_Vectortype *normal)
{
  Pnt_Vectortype light;
  float red, green, blue, mx, my, mz, x, y, z, distance, percent;
  float source_red, source_green, source_blue;
  int newcolor, lightnum;
  register float *rtrans= trans;
  
  red   = oldcolor->r;
  green = oldcolor->g;
  blue  = oldcolor->b;
  
  source_red = 0.0;
  source_green = 0.0;
  source_blue = 0.0;
  
  /* Translate vertex coords from model to world coordinate system */
  mx= *(polygon->xcoords);
  my= *(polygon->ycoords);
  mz= *(polygon->zcoords);
  x = rtrans[0]*mx + rtrans[4]*my + rtrans[8]*mz + rtrans[12];
  y = rtrans[1]*mx + rtrans[5]*my + rtrans[9]*mz + rtrans[13];
  z = rtrans[2]*mx + rtrans[6]*my + rtrans[10]*mz + rtrans[14];
  
  for (lightnum = 0; lightnum < DLIGHTCOUNT(self); lightnum++) {
    light.x = DLIGHTBUFFER(self)[lightnum].loc.x - x;
    light.y = DLIGHTBUFFER(self)[lightnum].loc.y - y;
    light.z = DLIGHTBUFFER(self)[lightnum].loc.z - z;
    distance = sqrt( (light.x * light.x) + (light.y * light.y)
		    + (light.z * light.z) );
    if (distance == 0.0) percent= 1.0;
    else {
      light.x /= distance; light.y /= distance; light.z /= distance;
      percent = pnt_dotproduct( normal, &light );
    }
    if (percent < 0.0)
      percent = 0.0;
    source_red += (percent * DLIGHTBUFFER(self)[lightnum].color.r);
    source_green += (percent * DLIGHTBUFFER(self)[lightnum].color.g);
    source_blue += (percent * DLIGHTBUFFER(self)[lightnum].color.b);
  }
  
  if ((red= red*( source_red + AMBIENTCOLOR(self).r ) ) > 1.0)
    red = 1.0;
  if ((green= green*( source_green + AMBIENTCOLOR(self).g ) ) > 1.0)
    green = 1.0;
  if ((blue= blue*( source_blue + AMBIENTCOLOR(self).b ) ) > 1.0)
    blue = 1.0;
  /*	the <a> in a color rec is not used yet 		*/
  newcolor = pnt_makeDcolor_rec(self,red,green,blue,1.0);
  return(newcolor);
}


/*
   Global Data Used:  pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer
   Expl:  This routine transforms the polygon cooridnates in the
	  global coordinate buffers, which are positions in 3 dimensional
	  space, to their corresponding positions in 2-dimensional space.
	  The perspective transformation happens here, as well as normalization
	  of the viewing fustrum.  All these transformations are contained
	  in the transformation matrix represented by the global variables
	  EIx where x is between 0 and 15.  Note that only the EIx values with
   	  non-zero values are used in the calculations.
*/
static void trans3Dto2D(int numcoords)
{ 
  float x,y;
  float z,w;
  int i;
  
  for (i=0;i<numcoords;i++) {
    x = pnt_Xcoord_buffer[i];
    y = pnt_Ycoord_buffer[i];
    z = pnt_Zcoord_buffer[i];
    
    pnt_Xcoord_buffer[i] = EI0*x + EI8*z;
    pnt_Ycoord_buffer[i] = EI5*y + EI9*z;
    pnt_Zcoord_buffer[i] = EI10*z + EI14;
    w =  EI11*z;
    if (w == 0.0) w= HUGE;
    
/* must normalize the coordinates */
    pnt_Xcoord_buffer[i] /=  w;
    pnt_Ycoord_buffer[i] /=  w;
    pnt_Zcoord_buffer[i] /=  w;
  }
}

/*
   Global Data Used: pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer

   Expl:  This routine transforms <polygon>'s coordinates from its position in
	  the world coordinate system to its position with respect to the 
	  camera.  Also, any transformations of gob-children relative to gobs 
	  have been pre-concatinated onto ViewMatrix, so these transformations 
	  happen here as well.  The transformed coordinates are placed in the
	  global coordinate buffers.
*/
static void translateWtoI(P_Renderer *self, Pnt_Polytype *poly, int numcoords)
{
  register float *xp, *yp, *zp;
  register float *ViewM,x,y,z;
  int i;

  ViewM= VIEWMATRIX(self);
  if (poly->numcoords == -1)
    ger_error("ERROR, Bad Polygon In WtoI\n");
  xp= poly->xcoords;
  yp= poly->ycoords;
  zp= poly->zcoords;
  for (i=0; i<numcoords;i++) {
    x = *xp++;
    y = *yp++;
    z = *zp++;
    
    pnt_Xcoord_buffer[i] = ViewM[0]*x + ViewM[4]*y + ViewM[8]*z + ViewM[12];
    pnt_Ycoord_buffer[i] = ViewM[1]*x + ViewM[5]*y + ViewM[9]*z + ViewM[13];
    pnt_Zcoord_buffer[i] = ViewM[2]*x + ViewM[6]*y + ViewM[10]*z + ViewM[14];
  }
}

/*
   Global Data Used:NONE
   Expl:  This routine returns 1 if the polygon defined by normal vector
	  <normal> is facing the camera, and 0 if it is facing away.
*/
static int forward_facing(P_Renderer *self, Pnt_Vectortype *normal)
{
  /* All we have to do is calculate the z component of the normal in
   * the camera's frame.  If it is less than zero, the polygon is
   * facing backwards.
   */
  register float *ViewM;
  float zcomp;

  ViewM= VIEWMATRIX(self);
  zcomp= ViewM[2]*normal->x + ViewM[6]*normal->y + ViewM[10]*normal->z;

  return( (zcomp>=0.0) );
}

static void render_polymarker(P_Renderer *self, Pnt_Polytype *polymarker, 
			      int numcoords, Pnt_Colortype *color)
{
  int newcolor;
  float z_depth,red,green,blue;

  if (numcoords != 1) {
      ger_error("ERROR: Marker with more than one coordinate\n");
      return;
    }
  check_coordbuffsz(self,numcoords);
  /*  
   * translateWtoI places the translated coordinates of the marker in
   * the buffers pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer
   */
  translateWtoI( self,polymarker,numcoords );
  z_depth = pnt_Zcoord_buffer[0];
  
  if (!pnt_insideZbound(self,z_depth,HITHER_PLANE))
    return;
  if (!pnt_insideZbound(self,z_depth,YON_PLANE))
    return;
  trans3Dto2D(numcoords);
  red   = color->r;
  green = color->g;
  blue  = color->b;
  newcolor = pnt_makeDcolor_rec(self,red,green,blue,1.0);
  /*  Add marker to global DepthBuffer, increment DepthCount  */
  insert_depthBuff(self,numcoords,z_depth,POLYMARKER,newcolor);
}

static void render_polyline(P_Renderer *self, Pnt_Polytype *polyline, 
			    int numcoords, Pnt_Colortype *color)
{
  int new_numcoords=0,newcolor;
  float z_depth,red,green,blue;
  
  /*  
    translateWtoI places the translated coordinates of the polygon in
    the buffers pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer
    */
  check_coordbuffsz(self,numcoords);
  translateWtoI( self,polyline,numcoords );
  
  /*  Clip on Far and near Z */
  /*  Clipped coordinates may contain more points than before 
      clipping */
  z_depth=0.0;
  pnt_clip_Zline(self,HITHER_PLANE,numcoords,&new_numcoords,&z_depth);
  pnt_clip_Zline(self,YON_PLANE,new_numcoords,&numcoords,&z_depth);
  if (numcoords < 2)
    return;
  trans3Dto2D(numcoords);
  red   = color->r;
  green = color->g;
  blue  = color->b;
  newcolor = pnt_makeDcolor_rec(self,red,green,blue,1.0);
  /*  Add polyline to global DepthBuffer, increment DepthCount  */
  insert_depthBuff(self,numcoords,z_depth,POLYLINE,newcolor);
}

/*
   Global Data Used: pnt_Xcoord_buffer,pnt_Ycoord_buffer,pnt_Zcoord_buffer;
   Expl:  This routine transforms the polygon at index <polygon> from
	  world coordinates to (virtual)screen coordinates, clips it against
	  the hither and yon planes, calculates its color with respect to
	  its orientation to the light, and inserts it into the depth buffer 
	  for later sorting and displaying.  If backface culling is on and a 
	  polygon is facing away from the camera, it is not added to the depth 
	  buffer.  The original polygon record is unaltered.
*/
static void render_polygon(P_Renderer *self, Pnt_Polytype *polygon, 
			   float *trans, int numcoords, int backcull, 
			   Pnt_Colortype *color)
{
  int new_numcoords=0,newcolor;
  float length,z_depth;
  Pnt_Vectortype normal;
  
  /*  
    translateWtoI places the translated coordinates of the polygon in
    the buffers pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer
    */
  check_coordbuffsz(self,numcoords);
  translateWtoI( self,polygon,numcoords );
  
  /* calculate the polygon's normal */
  pnt_calc_normal(self,polygon,trans,&normal);
  length = pnt_vector_length(&normal);
  if (length == 0.0)
    length=1.0;
  
  /*  do backface culling */
  if ( !backcull || forward_facing(self,&normal)) {
    normal.x /= length;
    normal.y /= length;
    normal.z /= length;
    newcolor = calc_intensity(self,color,polygon,trans,&normal);
    /*  Clip on Far and near Z */
    /*  Clipped coordinates may contain more points than before clipping */
    /*  pnt_clip_Zpolygon assumes Hither plane is clipped before Yon */
    z_depth=0.0;
    pnt_clip_Zpolygon(self,HITHER_PLANE,numcoords,&new_numcoords,
		      &z_depth);
    pnt_clip_Zpolygon(self,YON_PLANE,new_numcoords,&numcoords,
		      &z_depth);
    /* Make sure we still have a polygon */
    if (numcoords < 3)
      return;
    trans3Dto2D(numcoords);
    /*  Add polygon to global DepthBuffer, increment DepthCount  */
    insert_depthBuff(self,numcoords,z_depth,POLYGON,newcolor);
  }
}

static void pnt_recache(P_Renderer *self)
/* This routine copies some renderer data into caches for faster access */
{
  ger_debug("xpainter: swap_in_vars");

  EI0= EYEMATRIX(self)[0];
  EI1= EYEMATRIX(self)[1];
  EI2= EYEMATRIX(self)[2];
  EI3= EYEMATRIX(self)[3];
  EI4= EYEMATRIX(self)[4];
  EI5= EYEMATRIX(self)[5];
  EI6= EYEMATRIX(self)[6];
  EI7= EYEMATRIX(self)[7];
  EI8= EYEMATRIX(self)[8];
  EI9= EYEMATRIX(self)[9];
  EI10= EYEMATRIX(self)[10];
  EI11= EYEMATRIX(self)[11];
  EI12= EYEMATRIX(self)[12];
  EI13= EYEMATRIX(self)[13];
  EI14= EYEMATRIX(self)[14];
  EI15= EYEMATRIX(self)[15];

  current_renderer= self;
}

/*
   Global Data Used: none
   Expl:  Transforms each polygon at index <object> from world coordinates to
	  (virtual)screen coordinates and stores them in DepthBuffer, which is 
	  later sorted and drawn in order.  <ViewMatrix> is altered to include
	  <object_trans> for drawing all polygons at index <object>, and
	  then returned to its original value.
*/
static void pnt_render_primitive( P_Renderer *self, Pnt_Objecttype *object,
				 float *object_trans, int back_cull,
				 Pnt_Colortype *color)
{
  Pnt_Polytype *poly;
  Pnt_Colortype *newcolor;
  register int poly_index;
  int numcoords,numpolys;
  float *oldM;
  
  ger_debug("xpainter: pnt_render_primitive");

  if (current_renderer != self) pnt_recache(self);

  oldM = VIEWMATRIX(self);
  /*  
    Add gob-related transformations into ViewMatrix.  These occur BEFORE
    the translation from World to Eye coordinates (the ViewMatrix)
    */
  VIEWMATRIX(self) = pnt_mult3dMatrices(object_trans, VIEWMATRIX(self));
  poly = object->polygons;
  numpolys = object->num_polygons;
  for (poly_index=0; poly_index<numpolys; poly_index++) {
    numcoords = poly->numcoords;
    /* if no color of its own, inherit one */
    if (poly->color) newcolor = poly->color;
    else newcolor= color; 
    switch(poly->type) {
    case POLYGON:	
      render_polygon(self,poly,object_trans,numcoords, back_cull,newcolor);
      break;
    case POLYLINE:
      render_polyline(self,poly,numcoords,newcolor);
      break;
    case POLYMARKER:
      render_polymarker(self,poly,numcoords,newcolor);
      break;
    default:
      ger_error("ERROR: Undefined poly-type:%d\n", poly->type);
      break;
    }
    poly++;
  }
  free(VIEWMATRIX(self));
  VIEWMATRIX(self) = oldM;
}

static P_Void_ptr def_cmap( char *name, double min, double max,
		  void (*mapfun)(float *, float *, float *, float *, float *) )
/* This function stores a color map definition */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Renderer_Cmap *thismap;
  METHOD_IN

  ger_debug("gen_painter: def_cmap");

  if (max == min) {
    ger_error("gen_painter: def_cmap: max equals min (val is %f)", max );
    METHOD_OUT;
    return( (P_Void_ptr)0 );    
  }

  if ( RENDATA(self)->open ) {
    if ( !(thismap= (P_Renderer_Cmap *)malloc(sizeof(P_Renderer_Cmap))) )
      ger_fatal("gen_painter: def_cmap: unable to allocate %d bytes!",
		sizeof(P_Renderer_Cmap) );

    strncpy(thismap->name, name, MAXSYMBOLLENGTH-1);
    (thismap->name)[MAXSYMBOLLENGTH-1]= '\0';
    thismap->min= min;
    thismap->max= max;
    thismap->mapfun= mapfun;
    METHOD_OUT
    return( (P_Void_ptr)thismap );
  }
  METHOD_OUT
  return( (P_Void_ptr)0 );
}

static void install_cmap( P_Void_ptr mapdata )
/* This method causes the given color map to become the 'current' map. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gen_painter: install_cmap");
  if ( RENDATA(self)->open ) {
    if (mapdata) CUR_MAP(self)= (P_Renderer_Cmap *)mapdata;
    else ger_error("gen_painter: install_cmap: got null color map data.");
  }
  METHOD_OUT
}

static void destroy_cmap( P_Void_ptr mapdata )
/* This method causes renderer data associated with the given color map
 * to be freed.  It must not refer to "self", because the renderer which
 * created the data may already have been destroyed.
 */
{
  ger_debug("gen_painter: destroy_cmap");
  if ( mapdata ) free( mapdata );
}

static void map_color( P_Renderer *self, float val, 
		      float *r, float *g, float *b, float *a )
/* This routine invokes the color map. */
{
  /* no debugging; called too often */

  /* Scale, clip, and map. */
  val= ( val-MAP_MIN(self) )/( MAP_MAX(self)-MAP_MIN(self) );
  if ( val > 1.0 ) val= 1.0;
  if ( val < 0.0 ) val= 0.0;
  (*MAP_FUN(self))( &val, r, g, b, a );
}

static P_Void_ptr def_camera( P_Camera *cam )
/* This method defines the given camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Camera *saved_cam;
  METHOD_IN

  ger_debug("gen_painter: def_camera");
  if ( RENDATA(self)->open ) {
    saved_cam= po_create_camera(cam->name, &(cam->lookfrom), &(cam->lookat),
				&(cam->up), cam->fovea, cam->hither, cam->yon);
    saved_cam->background.ctype= cam->background.ctype;
    saved_cam->background.r= cam->background.r;
    saved_cam->background.g= cam->background.g;
    saved_cam->background.b= cam->background.b;
    saved_cam->background.a= cam->background.a;
  }
  else saved_cam= (P_Camera *)0;

  METHOD_OUT
  return( (P_Void_ptr)saved_cam );
}

static void set_camera( P_Void_ptr primdata )
/* This makes the given camera the current camera */
{
  P_Renderer *self= (P_Renderer *)po_this;
  double fov;
  P_Camera *cam;
  int i;
  METHOD_IN

  ger_debug("gen_painter: set_camera");
  cam= (P_Camera *)primdata;
  if ( RENDATA(self)->open ) {
    float *trans_camera, *Rotx, *Roty, *Rotz;
    float a, b, c, d, a_prime, b_prime, d_prime,length;
    Pnt_Vectortype *up, *forward, *rotated_upvector;
    Pnt_Pointtype *origin,*camera;
    float deg_to_rad= 3.14159265/180;
    int i;
    
    if (cam->yon == 0.0)
      cam->yon = 0.00001;
    origin = pnt_makepoint_rec( cam->lookat.x, cam->lookat.y, 
			       cam->lookat.z );
    camera = pnt_makepoint_rec( cam->lookfrom.x, cam->lookfrom.y, 
			       cam->lookfrom.z );
    /* Background color record */
    BACKGROUND(self).r= cam->background.r;
    BACKGROUND(self).g= cam->background.g;
    BACKGROUND(self).b= cam->background.b;
    BACKGROUND(self).a= cam->background.a;
    
    /* what if Camera_Up is not a unit vector ? */
    /* and what if Camera_Up is not perpendicular to Forward */
    up = pnt_makevector_rec( cam->up.x, cam->up.y, 
			    cam->up.z );
    forward = pnt_make_directionvector(camera,origin);
    
    if (VIEWMATRIX(self)) free( VIEWMATRIX(self) );
    VIEWMATRIX(self) = pnt_make3dScale(1.0,1.0,1.0);
    trans_camera = pnt_make3dTrans(0.0 - camera->x, 
				   0.0 - camera->y, 
				   0.0 - camera->z);
    pnt_append3dMatrices(VIEWMATRIX(self),trans_camera);
    
    a = forward->x;
    b = forward->y;
    c = forward->z;
    d = (float) sqrt( (b*b + c*c) );
    
    Rotx = pnt_make3dScale(1.0,1.0,1.0);
    Roty = pnt_make3dScale(1.0,1.0,1.0);
    Rotz = pnt_make3dScale(1.0,1.0,1.0);
    
    /*  Need check for d=0 */
    if (d != 0.0) {
      Rotx[5] = c/d;
      Rotx[6] = b/d;
      Rotx[9] = 0.0-b/d;
      Rotx[10] = c/d;
    } else {
      Rotx[5] = 1.0;
      Rotx[6] = 0.0;
      Rotx[9] = 0.0;
      Rotx[10] = 1.0;
    }
    
    Roty[0] =  d;
    Roty[2] =  a;
    Roty[8] =  0.0 - a;
    Roty[10] = d;
    
    pnt_append3dMatrices(VIEWMATRIX(self),Rotx);
    pnt_append3dMatrices(VIEWMATRIX(self),Roty);
    
    rotated_upvector = pnt_vector_matrix_mult3d(up,VIEWMATRIX(self));
    a_prime = rotated_upvector->x;
    b_prime = rotated_upvector->y;
    d_prime = (float) sqrt(a_prime*a_prime + b_prime*b_prime);
    
    if (d_prime != 0.0) {
      Rotz[0] = b_prime/d_prime;
      Rotz[1] = a_prime/d_prime;
      Rotz[4] = (0.0 - a_prime)/d_prime;
      Rotz[5] = b_prime/d_prime;
    } else {
      Rotz[0] = 1.0;
      Rotz[1] = 0.0;
      Rotz[4] = 0.0;
      Rotz[5] = 1.0;
    }
    pnt_append3dMatrices(VIEWMATRIX(self),Rotz);
    
    fov = fabs(cam->fovea) * deg_to_rad;
    for (i=0; i<15; i++) EYEMATRIX(self)[i]= 0.0;
    EYEMATRIX(self)[0] = 1.0 / tan((double) fov * 0.5);   
    EYEMATRIX(self)[5] = 1.0 / tan((double) fov * 0.5);   
    
    EYEMATRIX(self)[8] = -1.0;   /*  Offset origin to middle of screen */
    EYEMATRIX(self)[9] = -1.0;   /*  Screen dimensions are 2.0  by 2.0 */
    EYEMATRIX(self)[10] = 1.0 / (1.0 - cam->hither/cam->yon);
    EYEMATRIX(self)[11] = -1.0;
    EYEMATRIX(self)[14] = (0.0 + cam->hither)/ 
      (1.0 - cam->hither/cam->yon);
    ZMAX(self) = cam->hither;
    ZMIN(self) = cam->yon;
    
    /* Make sure the renderer doesn't keep old camera info in its caches */
    pnt_recache(self);
    
    free(rotated_upvector);
    free(up);
    free(forward);
    free(origin);
    free(camera);
    free(trans_camera);
    free(Rotx);
    free(Roty);
    free(Rotz);
  }
  METHOD_OUT
}

static void destroy_camera( P_Void_ptr primdata )
{
  /* This method must not refer to "self", because the renderer for which
   * the camera was defined may already have been destroyed.
   */
  if ( primdata ) {
    free(primdata);
  }
}

static void ren_print( VOIDLIST )
/* This is the print method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gen_painter: ren_print");
  if ( RENDATA(self)->open ) 
    ind_write("RENDERER: Painter renderer '%s' open",
	      self->name, MANAGER(self),GEOMETRY(self)); 
  else 
    ind_write("RENDERER: Painter renderer '%s' closed",
	      self->name, MANAGER(self),GEOMETRY(self)); 
  ind_eol();
  METHOD_OUT
}

static void ren_open( VOIDLIST )
/* This is the open method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("gen_painter: ren_open");
  RENDATA(self)->open= 1;
  METHOD_OUT
}

static void ren_close( VOIDLIST )
/* This is the close method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("gen_painter: ren_close");
  RENDATA(self)->open= 0;
  METHOD_OUT
}

static void ren_destroy( VOIDLIST )
/* This is the destroy method for the renderer */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("gen_painter: ren_destroy");

  if (RENDATA(self)->open) ren_close();
#ifdef INCL_XPAINTER
#ifdef INCL_PAINTER
  if (XPDATA(self)) xpnt_shutdown_renderer(self);
  else pnt_shutdown_renderer();
#else
  if (XPDATA(self)) xpnt_shutdown_renderer(self);
#endif
#else /* ifdef INCL_XPAINTER */
#ifdef INCL_PAINTER
  pnt_shutdown_renderer();
#endif
#endif

  if (ASSIST(self)) {
    METHOD_RDY( ASSIST(self) );
    (*(ASSIST(self)->destroy_self))();
  }
  if (DPOLYBUFFER(self)) free((P_Void_ptr)DPOLYBUFFER(self));
  if (DEPTHBUFFER(self)) free((P_Void_ptr)DEPTHBUFFER(self));
  if (DCOORDBUFFER(self)) free((P_Void_ptr)DCOORDBUFFER(self));
  if (DCOLORBUFFER(self)) free((P_Void_ptr)DCOLORBUFFER(self));
  if (MANAGER(self)) free((P_Void_ptr)MANAGER(self));
  if (GEOMETRY(self)) free((P_Void_ptr)GEOMETRY(self));
  if (VIEWMATRIX(self)) free((P_Void_ptr)VIEWMATRIX(self));
  if (EYEMATRIX(self)) free((P_Void_ptr)EYEMATRIX(self));

  RENDATA(self)->initialized= 0;
  free( (P_Void_ptr)RENDATA(self) );
  free( (P_Void_ptr)self );
  METHOD_DESTROYED
}

void internal_render(P_Renderer *self, P_Gob *gob, float *thistrans)
{
  P_Attrib_List newattrlist;
  P_Transform *atrans;
  float *newtrans;
  P_Gob_List *kidlist;
  
  ger_debug(
    "gen_painter: internal_render: rendering object given by gob <%s>",
	    gob->name);

  if ( gob->has_transform ) {
    atrans = transpose_trans( duplicate_trans( &(gob->trans) ) );
    newtrans = pnt_mult3dMatrices( atrans->d, thistrans );
    destroy_trans(atrans);
  } else newtrans  = thistrans;

  if (gob->attr) {
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->push_attributes))( gob->attr );
  }
  /* There must be children to render, because if this was a primitive
   * this method would not be getting executed.
   */
  kidlist= gob->children;
  while (kidlist) {
    RECENTTRANS(self)= newtrans;
    METHOD_RDY(kidlist->gob);
    (*(kidlist->gob->render_to_ren))(self, (P_Transform *)0, 
				     (P_Attrib_List *)0);
    kidlist= kidlist->next;
  }
  if (newtrans != thistrans) free(newtrans);
  if (gob->attr) {
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->pop_attributes))( gob->attr );
  }
}

int compare(a,b)
depthtable_rec *a,*b;
{
  if (a->key > b->key)
    return(1);
  if (a->key < b->key)
    return(-1);
  return(0);
}

static void internal_traverse(P_Renderer *self, P_Gob *gob, float *thistrans)
{
   P_Attrib_List newattrlist;
   P_Transform *atrans;
   float *newtrans;
   P_Gob_List *kidlist;

   ger_debug(
      "gen_painter: internal_traverse: traversing object given by gob <%s>",
	     gob->name);

   if ( gob->has_transform )
	{
	atrans = transpose_trans( duplicate_trans( &(gob->trans) ) );
	newtrans = pnt_mult3dMatrices( atrans->d, thistrans );
	destroy_trans(atrans);
	}
   else
	newtrans  = thistrans;
   if (gob->attr) {
     METHOD_RDY(ASSIST(self));
     (*(ASSIST(self)->push_attributes))( gob->attr );
   }
   /* There must be children to render, because if this was a primitive
    * this method would not be getting executed.
    */
   kidlist= gob->children;
   while (kidlist) {
     RECENTTRANS(self)= newtrans;
     METHOD_RDY(kidlist->gob);
     (*(kidlist->gob->traverselights_to_ren))(self, (P_Transform *)0, 
				      (P_Attrib_List *)0);
     kidlist= kidlist->next;
   }
   if (newtrans != thistrans) free(newtrans);
   if (gob->attr) {
     METHOD_RDY(ASSIST(self));
     (*(ASSIST(self)->pop_attributes))( gob->attr );
   }
}

static void traverse_gob( P_Void_ptr primdata, P_Transform *thistrans, 
		    P_Attrib_List *thisattrlist )
/* This routine traverses a gob looking for lights. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  int top_level_call= 0;
  METHOD_IN

  if (RENDATA(self)->open) {
    if (primdata) {
      P_Gob *thisgob;
      P_Transform *newtrans= (P_Transform *)0;

      thisgob= (P_Gob *)primdata;
      
      ger_debug(
	 "gen_painter: traverse_gob: traversing object given by gob <%s>", 
		thisgob->name);

      if (thisattrlist) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->push_attributes))( thisattrlist );
      }

      /* If thistrans is non-null, this is a top-level call to
       * traverse_gob, as opposed to a recursive call.  
       */
      if (thistrans) {
	top_level_call= 1;
	newtrans = transpose_trans( duplicate_trans(thistrans) );
	/* Initialize light table and ambient color buffer */
	DLIGHTCOUNT(self)= 0;
	AMBIENTCOLOR(self).r = 0.0;
	AMBIENTCOLOR(self).g = 0.0;
	AMBIENTCOLOR(self).b = 0.0;
	AMBIENTCOLOR(self).a = 0.0;
	internal_traverse(self,thisgob,newtrans->d);
	destroy_trans(newtrans);
      }
      else {
	float *oldtrans= RECENTTRANS(self);
	internal_traverse(self,thisgob,RECENTTRANS(self));
	RECENTTRANS(self)= oldtrans;
      }

      if (thisattrlist) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->pop_attributes))( thisattrlist );
      }

    }
    else ger_error("gen_painter: traverse_gob: got a null data pointer.");
  }
  METHOD_OUT
}

static void destroy_gob( P_Void_ptr primdata )
/* This routine destroys the renderer's data for the gob. */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    P_Gob *gob;
    gob= (P_Gob *)primdata;
    ger_debug("gen_painter: destroy_gob: destroying gob <%s>",gob->name);
  }
  METHOD_OUT
}

static P_Void_ptr def_sphere(char *name)
/* This routine defines a sphere */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_sphere");

    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_sphere))();

    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_sphere( P_Void_ptr rendata, P_Transform *trans, 
			 P_Attrib_List *attr )
/* This routine renders a sphere */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: ren_sphere");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_sphere))(rendata,trans,attr);
  }  
  METHOD_OUT
}

static void destroy_sphere( P_Void_ptr rendata )
/* This routine renders a sphere */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: destroy_sphere");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_sphere))(rendata);
  }  
  METHOD_OUT
}

static P_Void_ptr def_cylinder(char *name)
/* This routine defines a cylinder */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_cylinder");

    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_cylinder))();

    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_cylinder( P_Void_ptr rendata, P_Transform *trans, 
			 P_Attrib_List *attr )
/* This routine renders a cylinder */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: ren_cylinder");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_cylinder))(rendata,trans,attr);
  }  
  METHOD_OUT
}

static void destroy_cylinder( P_Void_ptr rendata )
/* This routine renders a cylinder */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: destroy_cylinder");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_cylinder))(rendata);
  }  
  METHOD_OUT
}

static P_Void_ptr def_torus(char *name, double major, double minor)
/* This routine defines a torus */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_torus");

    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_torus))(major,minor);

    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_torus( P_Void_ptr rendata, P_Transform *trans, 
			 P_Attrib_List *attr )
/* This routine renders a torus */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: ren_torus");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_torus))(rendata,trans,attr);
  }  
  METHOD_OUT
}

static void destroy_torus( P_Void_ptr rendata )
/* This routine renders a torus */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: destroy_torus");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_torus))(rendata);
  }  
  METHOD_OUT
}

static void get_coords(Pnt_Polytype *record, P_Vlist *vlist)
{
  int length, i;
  float *x, *y, *z;

  METHOD_RDY(vlist);
  length= vlist->length;
  if ( !(record->xcoords= (float *)malloc(length*sizeof(float))) )
    ger_fatal("gen_painter: get_coords: unable to allocate %d floats for x!",
	      length);
  if ( !(record->ycoords= (float *)malloc(length*sizeof(float))) )
    ger_fatal("gen_painter: get_coords: unable to allocate %d floats for y!",
	      length);
  if ( !(record->zcoords= (float *)malloc(length*sizeof(float))) )
    ger_fatal("gen_painter: get_coords: unable to allocate %d floats for z!",
	      length);
  x= record->xcoords;
  y= record->ycoords;
  z= record->zcoords;
  METHOD_RDY(vlist);
  for ( i=0; i<length; i++ ) {
    *x++= (*(vlist->x))(i);
    *y++= (*(vlist->y))(i);
    *z++= (*(vlist->z))(i);
  }
}

static void get_ind_coords(Pnt_Polytype *record, P_Vlist *vlist, 
			   int *indices, int length)
{
  int i;
  float *x, *y, *z;

  if ( !(record->xcoords= (float *)malloc(length*sizeof(float))) )
    ger_fatal(
	"gen_painter: get_ind_coords: unable to allocate %d floats for x!",
	      length);
  if ( !(record->ycoords= (float *)malloc(length*sizeof(float))) )
    ger_fatal(
	"gen_painter: get_ind_coords: unable to allocate %d floats for y!",
	      length);
  if ( !(record->zcoords= (float *)malloc(length*sizeof(float))) )
    ger_fatal(
        "gen_painter: get_ind_coords: unable to allocate %d floats for z!",
	      length);
  x= record->xcoords;
  y= record->ycoords;
  z= record->zcoords;
  METHOD_RDY(vlist);
  for ( i=0; i<length; i++ ) {
    *x++= (*(vlist->x))(*indices);
    *y++= (*(vlist->y))(*indices);
    *z++= (*(vlist->z))(*indices++);
  }
}

static void get_one_coord(Pnt_Polytype *record, P_Vlist *vlist, int i)
{
  if ( !(record->xcoords= (float *)malloc(sizeof(float))) )
    ger_fatal(
       "gen_painter: get_one_coord: unable to allocate 1 float for x!");
  if ( !(record->ycoords= (float *)malloc(sizeof(float))) )
    ger_fatal(
       "gen_painter: get_one_coord: unable to allocate 1 float for y!");
  if ( !(record->zcoords= (float *)malloc(sizeof(float))) )
    ger_fatal(
       "gen_painter: get_one_coord: unable to allocate 1 float for z!");
  METHOD_RDY(vlist);
  *(record->xcoords)= (*(vlist->x))(i);
  *(record->ycoords)= (*(vlist->y))(i);
  *(record->zcoords)= (*(vlist->z))(i);
}

static Pnt_Colortype *get_ave_color(P_Vlist *vlist)
{
  int i,length;
  Pnt_Colortype *result;

  METHOD_RDY(vlist);
  length= vlist->length;
  if ( !(result= (Pnt_Colortype *)malloc(sizeof(Pnt_Colortype))) )
    ger_fatal(
       "gen_painter: get_ave_color: unable to allocate %d bytes!",
	      sizeof(Pnt_Colortype));
  result->r= result->g= result->b= 0.0;
  result->a= 1.0; /* opacity currently ignored */
  METHOD_RDY(vlist);
  for ( i=0; i<length; i++ ) {
    result->r += (*(vlist->r))(i);
    result->g += (*(vlist->g))(i);
    result->b += (*(vlist->b))(i);
  }
  result->r /= length;
  result->g /= length;
  result->b /= length;
  return(result);
}

static Pnt_Colortype *get_ind_ave_color(P_Vlist *vlist, int *indices,
					int length)
{
  int i;
  Pnt_Colortype *result;

  if ( !(result= (Pnt_Colortype *)malloc(sizeof(Pnt_Colortype))) )
    ger_fatal(
       "gen_painter: get_ind_ave_color: unable to allocate %d bytes!",
	      sizeof(Pnt_Colortype));
  result->r= result->g= result->b= 0.0;
  result->a= 1.0; /* opacity currently ignored */
  METHOD_RDY(vlist);
  for ( i=0; i<length; i++ ) {
    result->r += (*(vlist->r))(*indices);
    result->g += (*(vlist->g))(*indices);
    result->b += (*(vlist->b))(*indices++);
  }
  result->r /= length;
  result->g /= length;
  result->b /= length;
  return(result);
}

static Pnt_Colortype *get_one_color(P_Vlist *vlist, int i)
{
  Pnt_Colortype *result;

  METHOD_RDY(vlist);
  if ( !(result= (Pnt_Colortype *)malloc(sizeof(Pnt_Colortype))) )
    ger_fatal(
       "gen_painter: get_one_color: unable to allocate %d bytes!",
	      sizeof(Pnt_Colortype));
  result->a= 1.0; /* opacity currently ignored */
  METHOD_RDY(vlist);
  result->r= (*(vlist->r))(i);
  result->g= (*(vlist->g))(i);
  result->b= (*(vlist->b))(i);
  return(result);
}

static Pnt_Colortype *get_val_color(P_Renderer *self, P_Vlist *vlist)
{
  int length, i;
  Pnt_Colortype *result;
  float r, g, b, a;

  METHOD_RDY(vlist);
  length= vlist->length;
  if ( !(result= (Pnt_Colortype *)malloc(sizeof(Pnt_Colortype))) )
    ger_fatal(
       "gen_painter: get_val_color: unable to allocate %d bytes!",
	      sizeof(Pnt_Colortype));
  result->r= result->g= result->b= 0.0;
  result->a= 1.0; /* opacity currently ignored */
  METHOD_RDY(vlist);
  for ( i=0; i<length; i++ ) {
    map_color(self, (*(vlist->v))(i), &r, &g, &b, &a);
    result->r += r;
    result->g += g;
    result->b += b;
  }
  result->r /= length;
  result->g /= length;
  result->b /= length;
  return(result);
}

static Pnt_Colortype *get_ind_val_color(P_Renderer *self, P_Vlist *vlist,
					int *indices, int length)
{
  int i;
  Pnt_Colortype *result;
  float r, g, b, a;

  if ( !(result= (Pnt_Colortype *)malloc(sizeof(Pnt_Colortype))) )
    ger_fatal(
       "gen_painter: get_ind_val_color: unable to allocate %d bytes!",
	      sizeof(Pnt_Colortype));
  result->r= result->g= result->b= 0.0;
  result->a= 1.0; /* opacity currently ignored */
  METHOD_RDY(vlist);
  for ( i=0; i<length; i++ ) {
    map_color(self, (*(vlist->v))(*indices++), &r, &g, &b, &a);
    result->r += r;
    result->g += g;
    result->b += b;
  }
  result->r /= length;
  result->g /= length;
  result->b /= length;
  return(result);
}

static Pnt_Colortype *get_one_val_color(P_Renderer *self, P_Vlist *vlist, 
					int i)
{
  Pnt_Colortype *result;
  float r, g, b, a;

  if ( !(result= (Pnt_Colortype *)malloc(sizeof(Pnt_Colortype))) )
    ger_fatal(
       "gen_painter: get_one_val_color: unable to allocate %d bytes!",
	      sizeof(Pnt_Colortype));
  result->a= 1.0; /* opacity currently ignored */
  METHOD_RDY(vlist);
  map_color(self, (*(vlist->v))(i), &r, &g, &b, &a);
  result->r += r;
  result->g += g;
  result->b += b;
  return(result);
}

static Pnt_Polytype *get_polyrec( P_Renderer *self, P_Vlist *vlist, 
				 primtype polytype )
{
  Pnt_Polytype *result;

  if ( !(result= (Pnt_Polytype *)malloc( sizeof(Pnt_Polytype) ) ) )
    ger_fatal("gen_painter: get_polyrec: unable to allocate %d bytes!\n",
	      sizeof(Pnt_Polytype));

  METHOD_RDY(vlist);
  result->numcoords= vlist->length;
  result->type= polytype;
  result->free_me= 1;
  get_coords(result,vlist);
  METHOD_RDY(vlist);
  switch (vlist->type) {
  case P3D_CVTX:   
  case P3D_CNVTX:
    result->color= (Pnt_Colortype *)0;
    break;
  case P3D_CCVTX:  
  case P3D_CCNVTX: 
    result->color= get_ave_color(vlist);
    break;
  case P3D_CVVTX:
  case P3D_CVVVTX: /* ignore second value */
  case P3D_CVNVTX: 
    result->color= get_val_color(self,vlist);
    break;
  default: 
    ger_error("gen_painter: get_polyrec: unknown vlist type %d!",vlist->type);
  }

  return(result);
}
static Pnt_Polytype *get_mesh_polyrec( P_Renderer *self, P_Vlist *vlist,
				      int *indices, int *facet_lengths, 
				      int nfacets, primtype polytype )
{
  Pnt_Polytype *result, *runner;
  int i;

  if ( !(result= (Pnt_Polytype *)malloc( nfacets*sizeof(Pnt_Polytype) ) ) )
    ger_fatal("gen_painter: get_mesh_polyrec: unable to allocate %d bytes!\n",
	      nfacets*sizeof(Pnt_Polytype));

  runner= result;
  METHOD_RDY(vlist);
  for (i=0; i<nfacets; i++) {
    runner->numcoords= *facet_lengths;
    runner->type= polytype;
    runner->free_me= 0;
    get_ind_coords(runner, vlist, indices, *facet_lengths);
    switch (vlist->type) {
    case P3D_CVTX:   
    case P3D_CNVTX:
      runner->color= (Pnt_Colortype *)0;
      break;
    case P3D_CCVTX:  
    case P3D_CCNVTX: 
      runner->color= get_ind_ave_color(vlist, indices, *facet_lengths);
      break;
    case P3D_CVVTX:
    case P3D_CVVVTX: /* ignore second value */
    case P3D_CVNVTX: 
      runner->color= get_ind_val_color(self, vlist, indices, *facet_lengths);
      break;
    default: 
      ger_error("gen_painter: get_mesh_polyrec: unknown vlist type %d!",
		vlist->type);
    }
    /* Advance to the next polygon */
    indices += *facet_lengths;
    facet_lengths++;
    runner++;
  }
  result->free_me= 1; /* freeing the first will free all */

  return(result);
}


static Pnt_Polytype *get_tri_polyrec( P_Renderer *self, P_Vlist *vlist,
				     primtype type )
{
  Pnt_Polytype *result, *runner;
  int i,nfacets;
  int indices[3];

  nfacets= vlist->length - 2;

  if ( !(result= (Pnt_Polytype *)malloc( nfacets*sizeof(Pnt_Polytype) ) ) )
    ger_fatal("gen_painter: get_tri_polyrec: unable to allocate %d bytes!\n",
	      nfacets*sizeof(Pnt_Polytype));

  runner= result;
  METHOD_RDY(vlist);
  for (i=0; i<nfacets; i++) {
    if (i%2) { /* odd, so we flip vertex order */
      indices[0]= i+1;
      indices[1]= i;
      indices[2]= i+2;
    }
    else { /* even- normal vertex order */
      indices[0]= i;
      indices[1]= i+1;
      indices[2]= i+2;
    }
    runner->numcoords= 3;
    runner->type= type;
    runner->free_me= 0;
    get_ind_coords(runner, vlist, indices, 3);
    switch (vlist->type) {
    case P3D_CVTX:   
    case P3D_CNVTX:
      runner->color= (Pnt_Colortype *)0;
      break;
    case P3D_CCVTX:  
    case P3D_CCNVTX: 
      runner->color= get_ind_ave_color(vlist, indices, 3);
      break;
    case P3D_CVVTX:
    case P3D_CVVVTX: /* ignore second value */
    case P3D_CVNVTX: 
      runner->color= get_ind_val_color(self, vlist, indices, 3);
      break;
    default: 
      ger_error("gen_painter: get_tri_polyrec: unknown vlist type %d!",
		vlist->type);
    }
    /* Advance to the next polygon */
    runner++;
  }
  result->free_me= 1; /* freeing the first will free all */

  return(result);
}

static Pnt_Polytype *get_multiple_polyrecs( P_Renderer *self, P_Vlist *vlist, 
				 primtype polytype )
{
  Pnt_Polytype *result, *runner;
  int i;

  if ( !(result=(Pnt_Polytype *)malloc( vlist->length*sizeof(Pnt_Polytype) )) )
    ger_fatal(
       "gen_painter: get_multiple_polyrecs: unable to allocate %d bytes!\n",
	      vlist->length*sizeof(Pnt_Polytype));

  runner= result;
  for (i=0; i<vlist->length; i++) {
    runner->numcoords= 1;
    runner->type= polytype;
    runner->free_me= 0;
    get_one_coord(runner,vlist,i);
    switch (vlist->type) {
    case P3D_CVTX:   
    case P3D_CNVTX:
      runner->color= (Pnt_Colortype *)0;
      break;
    case P3D_CCVTX:  
    case P3D_CCNVTX: 
      runner->color= get_one_color(vlist,i);
      break;
    case P3D_CVVTX:
    case P3D_CVVVTX: /* ignore second value */
    case P3D_CVNVTX: 
      runner->color= get_one_val_color(self,vlist,i);
      break;
    default: 
      ger_error("gen_painter: get_multiple_polyrecs: unknown vlist type %d!",
		vlist->type);
    }
    runner++;
  }
  result->free_me= 1; /* freeing the first will free all */

  return(result);
}

static P_Void_ptr def_polymarker(char *name, P_Vlist *vlist)
/* This routine defines a polymarker */
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_polymarker");
    if (vlist->length < 1)
      ger_error(
 "gen_painter: def_polymarker: %d isn't enough vertices for a polymarker!",
		vlist->length);
    else {
      if ( !(result= (Pnt_Objecttype *)malloc(sizeof(Pnt_Objecttype))) )
	ger_fatal(
	  "gen_painter: def_polymarker: unable to allocate %d bytes!",
		  sizeof(Pnt_Objecttype));
      result->polygons= get_multiple_polyrecs( self, vlist, POLYMARKER ); 
      result->num_polygons= vlist->length;     
      METHOD_OUT
      return( (P_Void_ptr)result );
    }
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_polyline(char *name, P_Vlist *vlist)
/* This routine defines a polyline */
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_polyline");
    if (vlist->length < 2)
      ger_error(
  "gen_painter: def_polyline: %d isn't enough vertices for a polyline!",
		vlist->length);
    else {
      if ( !(result= (Pnt_Objecttype *)malloc(sizeof(Pnt_Objecttype))) )
	ger_fatal("gen_painter: def_polyline: unable to allocate %d bytes!",
		  sizeof(Pnt_Objecttype));
      result->polygons= get_polyrec( self, vlist, POLYLINE ); 
      result->num_polygons= 1;
      METHOD_OUT
      return( (P_Void_ptr)result );
    }
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_polygon(char *name, P_Vlist *vlist)
/* This routine defines a polyline */
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_polygon");
    if (vlist->length < 3)
      ger_error(
	"gen_painter: def_polygon: %d isn't enough vertices for a polygon!",
	vlist->length);
    else {
      if ( !(result= (Pnt_Objecttype *)malloc(sizeof(Pnt_Objecttype))) )
	ger_fatal("gen_painter: def_polygon: unable to allocate %d bytes!",
		  sizeof(Pnt_Objecttype));
      result->polygons= get_polyrec( self, vlist, POLYGON ); 
      result->num_polygons= 1;
      METHOD_OUT
      return( (P_Void_ptr)result );
    }
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_object(P_Void_ptr primdata, P_Transform *trans, 
			 P_Attrib_List *attr)
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *object;
  Pnt_Colortype color;
  P_Color *pcolor;
  int back_cull;
  P_Transform *newtrans;
  METHOD_IN

  /* The mechanism by which this gets called guarantees attr is null. */

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: ren_object");
    object= (Pnt_Objecttype *)primdata;
    if (!object) {
      ger_error("gen_painter: ren_object: null object data found.");
      METHOD_OUT
      return;
    }

    /* Inherit needed attributes */
    METHOD_RDY(ASSIST(self));
    pcolor= (*(ASSIST(self)->color_attribute))(COLORSYMBOL(self));
    back_cull= (*(ASSIST(self)->bool_attribute))(BACKCULLSYMBOL(self));
    rgbify_color(pcolor);
    color.r= pcolor->r;
    color.g= pcolor->g;
    color.b= pcolor->b;
    color.a= pcolor->a;

    /* The assist object can call this routine with non-null transforms
     * under some circumstances, so we need to handle that case.
     */
    if (trans) {
      P_Transform *newtrans;
      float *totaltrans;
      newtrans = transpose_trans( duplicate_trans(trans) );
      totaltrans = pnt_mult3dMatrices( newtrans->d, RECENTTRANS(self) );
      pnt_render_primitive(self, object, totaltrans, back_cull, &color);
      destroy_trans(newtrans);
      free( (P_Void_ptr)totaltrans );
    } else {
      pnt_render_primitive(self, object, RECENTTRANS(self), back_cull, &color);
    }
  }  

  METHOD_OUT
}

static void destroy_polymarker(P_Void_ptr primdata)
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *obj= (Pnt_Objecttype *)primdata;
  Pnt_Polytype *rec;
  int i;
  METHOD_IN

  /* This block was allocated with get_multiple_polyrecs */
  rec= obj->polygons;
  free( (P_Void_ptr)rec );
  free( (P_Void_ptr)obj );

  METHOD_OUT
}

static void destroy_polyrec(Pnt_Polytype *rec)
{
  if (rec->xcoords) free( (P_Void_ptr)(rec->xcoords) );
  if (rec->ycoords) free( (P_Void_ptr)(rec->ycoords) );
  if (rec->zcoords) free( (P_Void_ptr)(rec->zcoords) );
  if (rec->color) free( (P_Void_ptr)(rec->color) );
  if (rec->free_me) free( (P_Void_ptr)rec );
}

static void destroy_objecttype(P_Void_ptr primdata)
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *obj= (Pnt_Objecttype *)primdata;
  Pnt_Polytype *rec;
  int i;
  METHOD_IN

  rec= obj->polygons;
  for (i=0; i<obj->num_polygons; i++)
    destroy_polyrec( rec++ );
  free( (P_Void_ptr)obj );

  METHOD_OUT
}

static void destroy_mesh(P_Void_ptr primdata)
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *obj= (Pnt_Objecttype *)primdata;
  Pnt_Polytype *rec;
  int i;
  METHOD_IN

  rec= obj->polygons;
  for (i=0; i<obj->num_polygons; i++) {
    if (rec->xcoords) free( (P_Void_ptr)(rec->xcoords) );
    if (rec->ycoords) free( (P_Void_ptr)(rec->ycoords) );
    if (rec->zcoords) free( (P_Void_ptr)(rec->zcoords) );
    if (rec->color) free( (P_Void_ptr)(rec->color) );
    rec++;
  }
  free( (P_Void_ptr)(obj->polygons) );
  free( (P_Void_ptr)obj );

  METHOD_OUT
}

static P_Void_ptr def_tristrip(char *name, P_Vlist *vlist)
/* This routine defines a triangle strip */
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_tristrip");
    if (vlist->length < 3)
      ger_error(
    "gen_painter: def_tristrip: %d isn't enough vertices for a tristrip!",
		vlist->length);
    else {
      if ( !(result= (Pnt_Objecttype *)malloc(sizeof(Pnt_Objecttype))) )
	ger_fatal("gen_painter: def_tristrip: unable to allocate %d bytes!",
		  sizeof(Pnt_Objecttype));
      result->polygons= get_tri_polyrec( self, vlist, POLYGON );
      result->num_polygons= vlist->length - 2;
      
      METHOD_OUT
      return( (P_Void_ptr)result );
    }
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_bezier(char *name, P_Vlist *vlist)
/* This routine defines a 4-by-4 bicubic Bezier patch */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_bezier");

    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_bezier))(vlist);

    METHOD_OUT
    return( result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_bezier( P_Void_ptr rendata, P_Transform *trans, 
			 P_Attrib_List *attr )
/* This routine renders a 4-by-4 bicubic Bezier patch */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: ren_bezier");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_bezier))(rendata,trans,attr);

  }  
  METHOD_OUT
}

static void destroy_bezier( P_Void_ptr rendata )
/* This routine destroys a 4-by-4 bicubic Bezier patch */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: destroy_bezier");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_bezier))(rendata);

  }  
  METHOD_OUT
}

static P_Void_ptr def_mesh(char *name, P_Vlist *vlist, int *indices,
                           int *facet_lengths, int nfacets)
/* This routine defines a general mesh */
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Objecttype *result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_mesh");
    if (vlist->length < 3)
      ger_error(
		"gen_painter: def_mesh: %d isn't enough vertices for a mesh!",
		vlist->length);
    else if (nfacets<1)
      ger_error(
		"gen_painter: def_mesh: %d isn't enough facets for a mesh!",
		nfacets);
    else {
      if ( !(result= (Pnt_Objecttype *)malloc(sizeof(Pnt_Objecttype))) )
	ger_fatal("gen_painter: def_mesh: unable to allocate %d bytes!",
		  sizeof(Pnt_Objecttype));
      result->polygons= get_mesh_polyrec( self, vlist, indices,
					 facet_lengths, nfacets, POLYGON ); 
      result->num_polygons= nfacets;
      
      METHOD_OUT
      return( (P_Void_ptr)result );
    }
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_text( char *name, char *tstring, P_Point *location, 
                           P_Vector *u, P_Vector *v )
/* This method defines a text string */
{
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_text");
    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_text))( tstring, location, u, v );
    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_text( P_Void_ptr rendata, P_Transform *trans, 
			 P_Attrib_List *attr )
/* This method renders a text string */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: ren_text");
    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->ren_text))(rendata, trans, attr);
  }  
  METHOD_OUT
}

static void destroy_text( P_Void_ptr rendata )
/* This method destroys a text string */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: destroy_text");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_text))(rendata);

  }  
  METHOD_OUT
}

static P_Void_ptr def_light( char *name, P_Point *location, P_Color *color )
/* This method defines a positional light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Lighttype *result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_light");

    if ( !(result= (Pnt_Lighttype *)malloc(sizeof(Pnt_Lighttype))) )
      ger_fatal("gen_painter: def_ambient: unable to allocate %d bytes!",
		sizeof(Pnt_Lighttype));

    rgbify_color(color);
    result->color.r= color->r;
    result->color.g= color->g;
    result->color.b= color->b;
    result->color.a= color->a;
    result->loc.x= location->x;
    result->loc.y= location->y;
    result->loc.z= location->z;
    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void traverse_light( P_Void_ptr primdata, P_Transform *trans, 
			 P_Attrib_List *attr )
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Lighttype *light;
  register float *rtrans= RECENTTRANS(self);
  float lx,ly,lz;
  METHOD_IN

  if (RENDATA(self)->open) {
    int lightindex;

    ger_debug("gen_painter: traverse_light");

    light= (Pnt_Lighttype *)primdata;
    if (!light) {
      ger_error("gen_painter: traverse_light: null object data found.");
      METHOD_OUT
      return;
    }
    
    lightindex= pnt_new_DLightIndex(self);
    lx= light->loc.x;
    ly= light->loc.y;
    lz= light->loc.z;
    DLIGHTBUFFER(self)[lightindex].loc.x=
      rtrans[0]*lx + rtrans[4]*ly + rtrans[8]*lz + rtrans[12];
    DLIGHTBUFFER(self)[lightindex].loc.y=
      rtrans[1]*lx + rtrans[5]*ly + rtrans[9]*lz + rtrans[13];
    DLIGHTBUFFER(self)[lightindex].loc.z=
      rtrans[2]*lx + rtrans[6]*ly + rtrans[10]*lz + rtrans[14];
    DLIGHTBUFFER(self)[lightindex].color.r= light->color.r;
    DLIGHTBUFFER(self)[lightindex].color.g= light->color.g;
    DLIGHTBUFFER(self)[lightindex].color.b= light->color.b;
    DLIGHTBUFFER(self)[lightindex].color.a= light->color.a;
  }  
  METHOD_OUT
}

static void destroy_light( P_Void_ptr primdata )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("gen_painter: destroy_light");
  free( primdata );
  METHOD_OUT
}

static P_Void_ptr def_ambient( char *name, P_Color *color )
/* This method defines an ambient light source */
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Colortype *result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_ambient");

    if ( !(result= (Pnt_Colortype *)malloc(sizeof(Pnt_Colortype))) )
      ger_fatal("gen_painter: def_ambient: unable to allocate %d bytes!",
		sizeof(Pnt_Colortype));
    rgbify_color(color);
    result->r= color->r;
    result->g= color->g;
    result->b= color->b;
    result->a= color->a;
    METHOD_OUT
    return( (P_Void_ptr)result );
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void traverse_ambient( P_Void_ptr primdata, P_Transform *trans, 
			 P_Attrib_List *attr )
{
  P_Renderer *self= (P_Renderer *)po_this;
  Pnt_Colortype *color;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: traverse_ambient");

    color= (Pnt_Colortype *)primdata;
    if (!color) {
      ger_error("gen_painter: traverse_ambient: null object data found.");
      METHOD_OUT
      return;
    }
    AMBIENTCOLOR(self).r += color->r;
    AMBIENTCOLOR(self).g += color->g;
    AMBIENTCOLOR(self).b += color->b;
    AMBIENTCOLOR(self).a += color->a;
  }  
  METHOD_OUT
}

static void destroy_ambient( P_Void_ptr primdata )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("gen_painter: destroy_ambient");
  free( primdata );
  METHOD_OUT
}

static void ren_donothing( P_Void_ptr primdata, P_Transform *thistrans, 
		    P_Attrib_List *thisattrlist )
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN
  ger_debug("gen_painter: ren_donothing: doing nothing.");
  METHOD_OUT
}

static P_Void_ptr def_gob( char *name, P_Gob *gob )
/* This method defines a gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: def_gob");
    METHOD_OUT
    return( (P_Void_ptr)gob ); /* Stash the gob where we can get it later */
  }  
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_gob( P_Void_ptr primdata, P_Transform *thistrans, 
		    P_Attrib_List *thisattrlist )
/* This routine renders gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  int top_level_call= 0;
  METHOD_IN

  if (RENDATA(self)->open) {
    if (primdata) {
      P_Gob *thisgob;
      P_Transform *newtrans= (P_Transform *)0;

      thisgob= (P_Gob *)primdata;
      
      ger_debug("gen_painter: ren_gob: rendering object given by gob <%s>", 
		thisgob->name);

      if (thisattrlist) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->push_attributes))( thisattrlist );
      }

      /* If thistrans is non-null, this is a top-level call to
       * ren_gob, as opposed to a recursive call.  
       */
      if (thistrans) {
	top_level_call= 1;
	newtrans = transpose_trans( duplicate_trans(thistrans) );
	internal_render(self,thisgob,newtrans->d);
	destroy_trans(newtrans);
      } else {
	float *oldtrans= RECENTTRANS(self);
	internal_render(self,thisgob,RECENTTRANS(self));
	RECENTTRANS(self)= oldtrans;
      }

      if (thisattrlist) {
	METHOD_RDY(ASSIST(self));
	(*(ASSIST(self)->pop_attributes))( thisattrlist );
      }

      if (top_level_call) { /* Actually draw the model */
	/* This is the sort that implements Painter's Algorithm */
	qsort((char *)DEPTHBUFFER(self),(unsigned) DEPTHCOUNT(self),
	      sizeof(depthtable_rec),compare);

#ifdef INCL_XPAINTER
#ifdef INCL_PAINTER
	if (XPDATA(self)) xdraw_DepthBuffer(self);
	else draw_DepthBuffer(self);
#else
	if (XPDATA(self)) xdraw_DepthBuffer(self);
#endif
#else
#ifdef INCL_PAINTER
	draw_DepthBuffer(self);
#endif
#endif
	

	/*  Reset fill indices for disposable records  */
	DEPTHCOUNT(self) = 0;
	DEPTHPOLYCOUNT(self) = 0;
	DCOLORCOUNT(self) = 0;
	DCOORDINDEX(self) = 0;
      }
    }
    else ger_error("gen_painter: ren_gob: got a null data pointer.");
  }
  METHOD_OUT
}

static void hold_gob( P_Void_ptr primdata )
/* This routine holds gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: hold_gob: doing nothing");
  }
  METHOD_OUT
}

static void unhold_gob( P_Void_ptr primdata )
/* This routine unholds gob */
{
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gen_painter: unhold_gob: doing nothing");
  }
  METHOD_OUT
}

void setup_Buffers( P_Renderer *self )
{   
  DEPTHPOLYCOUNT(self) =0;
  DPOLYBUFFER(self) = (Pnt_DPolytype *)
    malloc( MAXDEPTHPOLY(self)*sizeof(Pnt_DPolytype));
  if (!DPOLYBUFFER(self))
    ger_fatal("ERROR: COULD NOT ALLOCATE %d POLYGONS\n",
	      MAXDEPTHPOLY(self));
  DEPTHCOUNT(self) = 0;
  DEPTHBUFFER(self) = (depthtable_rec *)
    malloc( MAXPOLYCOUNT(self)*sizeof(depthtable_rec) );
  if (!DEPTHBUFFER(self))
    ger_fatal("ERROR: COULD NOT ALLOCATE %d POLYGON RECORDS\n",
	      MAXPOLYCOUNT(self));
  DLIGHTCOUNT(self)= 0;
  DLIGHTBUFFER(self)= (Pnt_Lighttype *)
    malloc( MAXDLIGHTCOUNT(self)*sizeof(Pnt_Lighttype) );
  if (!DLIGHTBUFFER(self))
    ger_fatal(
	 "gen_painter: setup_Buffers: could not allocate %d light recors!",
	      MAXDLIGHTCOUNT(self));
}

P_Renderer *fill_methods(P_Renderer *self)
{
  /* Fill in all the methods */
  self->def_sphere= def_sphere;
  self->ren_sphere= ren_sphere;
  self->destroy_sphere= destroy_sphere;

  self->def_cylinder= def_cylinder;
  self->ren_cylinder= ren_cylinder;
  self->destroy_cylinder= destroy_cylinder;

  self->def_torus= def_torus;
  self->ren_torus= ren_torus;
  self->destroy_torus= destroy_torus;

  self->def_polymarker= def_polymarker;
  self->ren_polymarker= ren_object;
  self->destroy_polymarker= destroy_polymarker;

  self->def_polyline= def_polyline;
  self->ren_polyline= ren_object;
  self->destroy_polyline= destroy_objecttype;

  self->def_polygon= def_polygon;
  self->ren_polygon= ren_object;
  self->destroy_polygon= destroy_objecttype;

  self->def_tristrip= def_tristrip;
  self->ren_tristrip= ren_object;
  self->destroy_tristrip= destroy_objecttype;

  self->def_bezier= def_bezier;
  self->ren_bezier= ren_bezier;
  self->destroy_bezier= destroy_bezier;

  self->def_mesh= def_mesh;
  self->ren_mesh= ren_object;
  self->destroy_mesh= destroy_mesh;

  self->def_text= def_text;
  self->ren_text= ren_text;
  self->destroy_text= destroy_text;

  self->def_light= def_light;
  self->ren_light= ren_donothing;
  self->light_traverse_light= traverse_light;
  self->destroy_light= destroy_light;

  self->def_ambient= def_ambient;
  self->ren_ambient= ren_donothing;
  self->light_traverse_ambient= traverse_ambient;
  self->destroy_ambient= destroy_ambient;

  self->def_gob= def_gob;
  self->ren_gob= ren_gob;
  self->light_traverse_gob= traverse_gob;
  self->hold_gob= hold_gob;
  self->unhold_gob= unhold_gob;
  self->destroy_gob= destroy_gob;

  self->print= ren_print;
  self->open= ren_open;
  self->close= ren_close;
  self->destroy_self= ren_destroy;

  self->def_camera= def_camera;
  self->set_camera= set_camera;
  self->destroy_camera= destroy_camera;

  self->def_cmap= def_cmap;
  self->install_cmap= install_cmap;
  self->destroy_cmap= destroy_cmap;

  return( self );
}
