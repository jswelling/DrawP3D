/****************************************************************************
 * gen_paintr_strct.h
 * Authors Chris Nuuja and Joel Welling
 * Modified by Joe Demers
 * Copyright 1989, 1991, Pittsburgh Supercomputing Center, 
 * Carnegie Mellon University
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
/* This file provides structures needed by the Painter renderers. */

/* Maximum length of file names and generated symbols
 * (actually includes the final \0).
 */
#define MAXFILENAME 128
#define MAXSYMBOLLENGTH P3D_NAMELENGTH

typedef struct pnt_vector
{
  float x;
  float y;
  float z;
} Pnt_Vectortype;

typedef struct pnt_point
{
  float x;
  float y;
  float z;
} Pnt_Pointtype;

typedef enum { POLYGON=0, POLYLINE, POLYMARKER } primtype;

typedef struct pnt_color 
{
  float r;
  float g;
  float b;
  float a;
} Pnt_Colortype;

typedef struct pnt_lightrecord
{
  Pnt_Pointtype loc;
  Pnt_Colortype color;
} Pnt_Lighttype;

typedef struct pnt_polyrecord
{
  float *xcoords;	/* arrays of x, y, and z coord info */
  float *ycoords;	
  float *zcoords;
  int numcoords;        /* number of coordinates in this record */   
  Pnt_Colortype *color;	/* pointer to color memory buffer  */
  primtype type;        /* is it a POLYGON, POLYLINE,or POLYMARKER */
} Pnt_Polytype;

typedef struct pnt_dpolyrecord
{
  int x_index;	  /* offset into the coordinate memory buffer  */
  int y_index;	  /* where these coordinates begin */
  int z_index;
  int numcoords;  /* number of coordinates in this record */   
  int color;	  /* offset to color memory buffer  */
  primtype type;  /* is it a POLYGON, POLYLINE,or POLYMARKER */
} Pnt_DPolytype;

typedef struct pnt_object
{
  int num_polygons;
  Pnt_Polytype *polygons;
} Pnt_Objecttype;

typedef struct table_rec 
{
  int poly;
  float key;
} depthtable_rec;

/* Explainations for the buffer structures: 
 *
 * DepthBuffer: Buffer of depth records, and fill index.  Each record
 *  holds a polyrecord index and that polyrecord's Z depth.  This
 *  buffer is sorted by Z depth, and then the polyrecords drawn in
 *  (sorted) order.  Thats why its called painter
 *
 * DPolyBuffer: These records hold the 2d transformed and clipped
 *  versions of all primitive instances.  This buffer holds primitives
 *  that are sorted by zdepth.  Its indices are stored in depthBuffer.
 *
 * DColorBuffer: Buffer of disposable color records.  One record per
 *  primitive instance.  Each poly record has a color at this point,
 *  either its own or an inherited color.
 *
 * DCoordBuffer: Buffer holding single floating point coordintes.  It
 *  is this storage which is actually used to hold coordinates for drawing.
 *
 * DLightBuffer: Holds instances of light sources in the world coordinate
 *  system;  these records are used in the lighting calculation.
 *
 *
 */

/* Struct to hold info for a color map */
typedef struct renderer_cmap_struct {
  char name[MAXSYMBOLLENGTH];
  double min;
  double max;
  void (*mapfun)( float *, float *, float *, float *, float * );
} P_Renderer_Cmap;

/* struct for data specific to xpainter renderer */
typedef struct xpainter_data_struct {
  int xmauto;                   /* true if using xmautodrawimagehandler */
  int automanage;               /* true if automanaging (doing timer stuff) */
  int width;                    /* window width */
  int height;                   /* window height */
  int xoffset;
  int yoffset;
  void *ihandler;               /* ptr to xdraw- or xmautodraw-imagehandler */
  void *widget;                 /* widget to do XSynch()'s on */
} XP_data;

/* Struct for object data, and access functions for it */
typedef struct renderer_data_struct {
  int open;
  int initialized;
  P_Renderer_Cmap *current_cmap;
  char *string1;
  char *string2;
  Pnt_Colortype Background;     /* color record for background */
  float *RecentTrans;
  depthtable_rec *DepthBuffer;  /* buffer of depth table records */
  int DepthCount;               /* fill index */
  int MaxPolyCount;             /* size of the buffer */
  Pnt_DPolytype *DPolyBuffer;   /* buffer of disposable polyrecords */
  int DepthPolyCount;           /* fill index for the buffer */
  int MaxDepthPoly;             /* size of the buffer */
  Pnt_Colortype *DColorBuffer;  /* buffer of disposable color records */
  int DColorCount;              /* fill index for the buffer */
  int MaxDColorCount;           /* size of the buffer */
  float *DCoordBuffer;          /* buffer of disposable coordinate data */
  int DCoordIndex;              /* fill index for the buffer */
  int MaxDCoordIndex;           /* size of the buffer */
  Pnt_Lighttype *DLightBuffer;  /* buffer of disposable light sources */
  int DLightCount;              /* fill index for the buffer */
  int MaxDLightCount;           /* size of the buffer */
  Pnt_Colortype AmbientColor;   /* ambient light color */
  int TempCoordBuffSz;          /* size of transform and clipping buffers */
  float *ViewMatrix;            /* defines world-to-camera view transform */
  float *EyeMatrix;             /* Eye-to-image transformation */
  float Zmax;                   /* hither clipping distance */
  float Zmin;                   /* yon clipping distance */
  P_Symbol backcull_symbol;     /* backcull symbol */
  P_Symbol text_height_symbol;  /* text height symbol */
  P_Symbol color_symbol;        /* color symbol */
  P_Symbol material_symbol;     /* material symbol */
  P_Assist *assist;             /* renderer assist object */
  XP_data *xp_data;             /* data specific to the xpainter renderer */
} P_Renderer_data;

#define RENDATA( self ) ((P_Renderer_data *)(self->object_data))

#define XMAUTO( self ) (RENDATA(self)->xp_data->xmauto)
#define AUTOMANAGE( self ) (RENDATA(self)->xp_data->automanage)
#define MANAGER( self ) (RENDATA(self)->string1)
#define GEOMETRY( self ) (RENDATA(self)->string2)
#define WIDTH( self ) (RENDATA(self)->xp_data->width)
#define HEIGHT( self ) (RENDATA(self)->xp_data->height)
#define XOFFSET( self ) (RENDATA(self)->xp_data->xoffset)
#define YOFFSET( self ) (RENDATA(self)->xp_data->yoffset)
#define IHANDLER( self ) (RENDATA(self)->xp_data->ihandler)
#define WIDGET( self ) (RENDATA(self)->xp_data->widget)

#define XPDATA( self ) (RENDATA(self)->xp_data)
#define OUTFILE( self ) (RENDATA(self)->string1)
#define DEVICENAME( self ) (RENDATA(self)->string2)
#define CUR_MAP( self ) (RENDATA(self)->current_cmap)
#define MAP_NAME( self ) (CUR_MAP(self)->name)
#define MAP_MIN( self ) (CUR_MAP(self)->min)
#define MAP_MAX( self ) (CUR_MAP(self)->max)
#define MAP_FUN( self ) (CUR_MAP(self)->mapfun)
#define BACKGROUND( self ) (RENDATA(self)->Background)
#define RECENTTRANS( self ) (RENDATA(self)->RecentTrans)
#define DEPTHBUFFER( self ) (RENDATA(self)->DepthBuffer)
#define DEPTHCOUNT( self ) (RENDATA(self)->DepthCount)
#define MAXPOLYCOUNT( self ) (RENDATA(self)->MaxPolyCount)
#define DPOLYBUFFER( self ) (RENDATA(self)->DPolyBuffer)
#define MAXDEPTHPOLY( self ) (RENDATA(self)->MaxDepthPoly)
#define DEPTHPOLYCOUNT( self ) (RENDATA(self)->DepthPolyCount)
#define DCOLORBUFFER( self ) (RENDATA(self)->DColorBuffer)
#define DCOLORCOUNT( self ) (RENDATA(self)->DColorCount)
#define MAXDCOLORCOUNT( self ) (RENDATA(self)->MaxDColorCount)
#define DCOORDBUFFER( self ) (RENDATA(self)->DCoordBuffer)
#define DCOORDINDEX( self ) (RENDATA(self)->DCoordIndex)
#define MAXDCOORDINDEX( self ) (RENDATA(self)->MaxDCoordIndex)
#define DLIGHTBUFFER( self ) (RENDATA(self)->DLightBuffer)
#define DLIGHTCOUNT( self ) (RENDATA(self)->DLightCount)
#define MAXDLIGHTCOUNT( self ) (RENDATA(self)->MaxDLightCount)
#define AMBIENTCOLOR( self ) (RENDATA(self)->AmbientColor)
#define TEMPCOORDBUFFSZ( self ) (RENDATA(self)->TempCoordBuffSz)
#define VIEWMATRIX( self ) (RENDATA(self)->ViewMatrix)
#define EYEMATRIX( self ) (RENDATA(self)->EyeMatrix)
#define ZMAX( self ) (RENDATA(self)->Zmax)
#define ZMIN( self ) (RENDATA(self)->Zmin)
#define BACKCULLSYMBOL(self) (RENDATA(self)->backcull_symbol)
#define TEXTHEIGHTSYMBOL(self) (RENDATA(self)->text_height_symbol)
#define COLORSYMBOL(self) (RENDATA(self)->color_symbol)
#define MATERIALSYMBOL(self) (RENDATA(self)->material_symbol)
#define ASSIST(self) (RENDATA(self)->assist)

/*   clipping defs     */
#define HITHER_PLANE 0
#define YON_PLANE 1
