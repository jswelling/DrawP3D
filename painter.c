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


/* The following structures are used across all instances of the renderer.
 * They are buffers for storing coordinates in the process of being
 * transformed from world to screen space and clipped.
 */
extern int TempCoordBuffSz;

/* Space for default color map */
static P_Renderer_Cmap default_map;

/* default color map function */
static void default_mapfun(float *val, float *r, float *g, float *b, float *a)
/* This routine provides a simple map from values to colors within the
 * range 0.0 to 1.0.
 */
{
  /* No debugging; called too often */
  *r= *g= *b= *a;
}

/*
   Global Data Used: none
   Expl: initialized the cgmgen routines.  It tells gplot that
	 the device is DeviceName, to output to file FileName
	 (which, if it is '-', will cause output to go to the screen)
	 and that all coordinates are normalized to between xmin,ymin
	 and xmax ymax.  If xmax and ymax are not equal, distortion
	 in size results.  If they are made smaller, magnification of
	 the image will occur.  They are set to 1.0 so that a 1x1 square
	 located at the camera position will just barely cover the entire
	 screen.  They have been set to values that produce results close to
	 Dore's renderer
*/
static void init_cgmgen(char *outfile, char *devname)
{

  int error=0;
  float xmin=0.0,ymin=0.0,xmax=2.0,ymax=2.0;
  
  csetdev(devname,&error);
  wrcopn(outfile,&error);
  setwcd(&xmin,&ymin,&xmax,&ymax,&error);
  if (error) ger_error("ERROR: Cgmgen initialization error:%d\n",error);
}

/*
   Global Data Used: none
   Expl: shuts down the cgmgen routines by calling wrtend.
*/
static void shutdown_cgmgen()
{
  int error=0;
  
  wrtend(&error);
  if (error)
    ger_error("ERROR: Cgmgen shutdown error:%d\n",error);
}


/*
   Global Data Used: NONE (The wire-frame model switch will go here so that
			   the interior style will be hollow)
   Expl:  Draws a polygon,polyline, or polymarker with it's recorded color 
	  wristl is a cgmgen routine to set the interior style;
	  wpgndc is a cgmgen routine to set the polygon fill color;
	  wrtpgn is a cgmgen routine to draw a polygon;
*/
static void pnt_render_polyrec(P_Renderer *self, int depth_index)
{
  int i,error=0,poly_index,numcoords,x_index,y_index,color;
  float red,green,blue;

  poly_index = DEPTHBUFFER(self)[depth_index].poly;
  
  i=1;
  wristl(&i,&error);
  if (error) ger_error("ERROR: Bad interior style:%d\n",error);
  x_index = DPOLYBUFFER(self)[poly_index].x_index;
  y_index = DPOLYBUFFER(self)[poly_index].y_index;
  numcoords = DPOLYBUFFER(self)[poly_index].numcoords;
  
  color = DPOLYBUFFER(self)[poly_index].color;
  red   = DCOLORBUFFER(self)[color].r;
  green = DCOLORBUFFER(self)[color].g;
  blue  = DCOLORBUFFER(self)[color].b;
  
  switch(DPOLYBUFFER(self)[poly_index].type)
    {
    case POLYGON:
      wpgndc(&red,&green,&blue,&error);
      wrtpgn(DCOORDBUFFER(self)+x_index,
	     DCOORDBUFFER(self)+y_index, &numcoords,&error);
      break;
    case POLYLINE:
      wplndc(&red,&green,&blue,&error);
      wrplin(DCOORDBUFFER(self)+x_index,
	     DCOORDBUFFER(self)+y_index, &numcoords,&error);
      break;
    case POLYMARKER:
      wpmkdc(&red,&green,&blue,&error);
      wrtpmk(DCOORDBUFFER(self)+x_index,
	     DCOORDBUFFER(self)+y_index,&numcoords, &error);
      break;
    default:
      ger_error("ERROR: Unknown primitive:%d\n",error);
    }
}

/*
   Global Data Used: LightLocation,LightIntensity,AmbientIntensity,
		     ViewMatrix, Zmin, Zmax,
		     pnt_Xcoord_buffer, pnt_Ycoord_buffer, pnt_Zcoord_buffer
   Expl:  calls init_cgmgen, and initializes all global variables.
	   Zmin, Zmax, and EyeMatrix will all be set to appropriate values
	   in ren_camera (located in painter_ren.c).  Light characteristics are
	   set, but are not (currently) used.
*/
static void pnt_init_renderer(P_Renderer *self)
{
	int i;

	/*  
	    ViewMatrix is the matrix that translates  coordinates from
	    their position in the world to their position relative to the
	    Eye/Camera.  EyeMatrix hold projection information.
	*/
	VIEWMATRIX(self) = (float *) malloc(16*sizeof(float));
	if (!(VIEWMATRIX(self)))
	  ger_fatal(
		"painter: pnt_init_renderer: couldn't allocate 16 floats!");
	for (i=0;i<16;i++)
		VIEWMATRIX(self)[i]=0.0;
	VIEWMATRIX(self)[0] = 1.0;
	VIEWMATRIX(self)[5] = 1.0;
	VIEWMATRIX(self)[10] = 1.0;
	VIEWMATRIX(self)[15] = 1.0;

	EYEMATRIX(self) = (float *) malloc(16*sizeof(float));
	if (!(EYEMATRIX(self)))
	  ger_fatal(
		"painter: pnt_init_renderer: couldn't allocate 16 floats!");
	for (i=0;i<16;i++)
		EYEMATRIX(self)[i]=0.0;
	EYEMATRIX(self)[0] = 1.0;
	EYEMATRIX(self)[5] = 1.0;
	EYEMATRIX(self)[10] = 1.0;
	EYEMATRIX(self)[15] = 1.0;

	/* Set (default) Z clipping boundries,  Zmax is hither, Zmin is yon */
	ZMAX(self) = -0.01;
	ZMIN(self) = -900.0;

	/*
	   These buffers hold coordinates of polygons, polylines, markers, etc
	   as they are transformed from World coordinates to screen coordinates
	   The pnt_Xclip_buffer,pnt_Zclip_buffer,pnt_Yclip_buffer are used to
	   hold coordinates between clippings against the hither and
	   yon planes (Zmin and Zmax).
	*/
	pnt_Xcoord_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
	pnt_Ycoord_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
	pnt_Zcoord_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
	pnt_Xclip_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
	pnt_Yclip_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
	pnt_Zclip_buffer = (float *) malloc(TempCoordBuffSz*sizeof(float));
	if ( !pnt_Xcoord_buffer || !pnt_Ycoord_buffer || !pnt_Zcoord_buffer
	    || !pnt_Xclip_buffer || !pnt_Yclip_buffer || !pnt_Zclip_buffer )
	  ger_fatal("painter: pnt_init_renderer: memory allocation failed!\n");
	init_cgmgen(OUTFILE(self),DEVICENAME(self));

}

/*
  Global Data Used: none
  Expl:  calls shutdown_cgmgen.
*/
void pnt_shutdown_renderer()
{
  shutdown_cgmgen();
}

/*
  Global Data Used: background, depthcount
  Expl:  does cgm stuff, renders everything in the depthbuffer, does cgm stuff
*/
void draw_DepthBuffer(P_Renderer *self)
{
  register int i;
  int error=0,color_mode=1; 

  ger_debug("pnt_ren_mthd: draw_DepthBuffer");
    
  wrbegp( &error );		/* begin picture  */
  wrtcsm( &color_mode,&error );	/* use direct color */
  /* background color */
  wrbgdc( &(BACKGROUND(self).r), &(BACKGROUND(self).g), 
	 &(BACKGROUND(self).b), &(BACKGROUND(self).a) );
  wrbgpb( &error );		/* begin picture body  */
  if (error) ger_fatal("ERROR: Bad Picture Initialization!");
  
  if (DEPTHCOUNT(self) >= MAXDEPTHPOLY(self))
    ger_fatal("ERROR, was about to overflow!");
  for (i=0;i<DEPTHCOUNT(self);i++)
    pnt_render_polyrec(self,i);
  DEPTHCOUNT(self)=0;			/* reset DepthBuffer */
  
  wrendp(&error);
  if (error) ger_fatal("ERROR: Bad Picture End");
}

/*
  Global Data Used: just about all of it
  Expl: initializes object data, and the cgm painter renderer 
*/
P_Renderer *po_create_painter_renderer( char *device, char *datastr )
/* This routine creates a ptr-generating renderer object */
{
  P_Renderer *self;
  P_Renderer_data *rdata;
  static int sequence_number= 0;
  int length,i;

  ger_debug("po_create_painter_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Check to make sure this is the only instance, since
   * DRAWCGM can only handle one open device.
   */
  if (sequence_number>0) {
    ger_error(
"Sorry, only one instance of the Painter renderer is supported (for now).");
    return((P_Renderer *)0);
  }

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_painter_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_painter_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;

  /* Fill out default color map */
  strcpy(default_map.name,"default-map");
  default_map.min= 0.0;
  default_map.max= 1.0;
  default_map.mapfun= default_mapfun;

  /* Fill out public and private object data */
  sprintf(self->name,"painter%d",sequence_number++);
  rdata->open= 0;  /* renderer created in closed state */
  CUR_MAP(self)= &default_map;

  ASSIST(self)= po_create_assist(self);

  BACKCULLSYMBOL(self)= create_symbol("backcull");
  TEXTHEIGHTSYMBOL(self)= create_symbol("text-height");
  COLORSYMBOL(self)= create_symbol("color");
  MATERIALSYMBOL(self)= create_symbol("material");
  
  length = 1 + strlen(device);
  DEVICENAME(self) = (char *)malloc(length);
  strcpy(DEVICENAME(self),device);

  XPDATA(self) = 0;

  length = 1 + strlen(datastr);
  OUTFILE(self) = (char *)malloc(length);
  strcpy(OUTFILE(self),datastr);
  
  DCOORDINDEX(self)=0;
  MAXDCOORDINDEX(self) = INITIAL_MAX_DCOORD;
  DCOORDBUFFER(self) = (float *) malloc(MAXDCOORDINDEX(self)*sizeof(float));
  
  DCOLORCOUNT(self) = 0;
  MAXDCOLORCOUNT(self) = INITIAL_MAX_DCOLOR;
  DCOLORBUFFER(self) = (Pnt_Colortype *)
    malloc( MAXDCOLORCOUNT(self)*sizeof(Pnt_Colortype) );

  if ( !DCOORDBUFFER(self) || !DCOLORBUFFER(self) )
    ger_fatal("po_create_painter_renderer: memory allocation failed!");

  MAXPOLYCOUNT(self) = INITIAL_MAX_DEPTHPOLY;
  MAXDEPTHPOLY(self) = MAXPOLYCOUNT(self);
  
  MAXDLIGHTCOUNT(self)= INITIAL_MAX_DLIGHTS;

  pnt_init_renderer(self);
  
  setup_Buffers(self);

  RENDATA(self)->initialized= 1;

  return fill_methods(self);
}
