/****************************************************************************
 * drawp3d_fi.c
 * Author Joel Welling
 * Copyright 1990, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides the Fortran interface to drawp3d.
*/

/*  This module recognizes what type of machine it's on by the presence
of the symbol VMS, unix, CRAY, or ardent.  The following makes the machine
assignment less ambiguous.
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
#ifdef __hpux
#undef USE_UNIX
#endif
#ifdef OSF1
#define USE_UNIX
#endif

#ifdef _IBMR2
#include <string.h>
#endif
#ifdef VMS
#include descrip
#endif
#ifdef CRAY
#include <string.h>
#include <fortran.h>
#endif
#ifdef USE_UNIX
#include <string.h>
#endif
#ifdef ardent
#include <string.h>
#endif
#ifdef __hpux
#include <string.h>
#endif

/* Interface to the p3dgen core routines */
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"

/* Include defs files that allow linkage to Fortran on various systems */
#ifdef USE_UNIX
#include "fnames_.h"
#endif
#ifdef CRAY
#include "FNAMES.h"
#endif
#ifdef ardent
#include "FNAMES.h"
#endif
#ifdef _IBMR2
/* include nothing */
#endif
#ifdef __hpux
/* include nothing */
#endif

/* Maximum supported string length */
#define MAXSTRING 128

/* Set up interface to Fortran character string descriptors */
#define STRINGLENGTH /* nothing */
#define DEFSTRINGLENGTH /* nothing */
#define STRINGLENGTH2 /* nothing */
#define DEFSTRINGLENGTH2 /* nothing */
#define STRINGLENGTH3 /* nothing */
#define DEFSTRINGLENGTH3 /* nothing */
#define STRINGLENGTH4 /* nothing */
#define DEFSTRINGLENGTH4 /* nothing */

#ifdef VMS
typedef struct dsc$descriptor_s *string_descriptor;
#endif
#ifdef CRAY
typedef _fcd string_descriptor;
#endif                           
#ifdef USE_UNIX
typedef char *string_descriptor;
#define STRINGLENGTH ,stringlength
#define DEFSTRINGLENGTH int stringlength;
#define STRINGLENGTH2 ,stringlength2
#define DEFSTRINGLENGTH2 int stringlength2;
#define STRINGLENGTH3 ,stringlength3
#define DEFSTRINGLENGTH3 int stringlength3;
#define STRINGLENGTH4 ,stringlength4
#define DEFSTRINGLENGTH4 int stringlength4;
#endif
#ifdef _IBMR2
typedef char *string_descriptor;
#define STRINGLENGTH ,stringlength
#define DEFSTRINGLENGTH int stringlength;
#define STRINGLENGTH2 ,stringlength2
#define DEFSTRINGLENGTH2 int stringlength2;
#define STRINGLENGTH3 ,stringlength3
#define DEFSTRINGLENGTH3 int stringlength3;
#define STRINGLENGTH4 ,stringlength4
#define DEFSTRINGLENGTH4 int stringlength4;
#endif
#ifdef ardent
typedef struct { char *addr; int length; } *string_descriptor;
#endif
#ifdef __hpux
typedef char *string_descriptor;
#define STRINGLENGTH ,stringlength
#define DEFSTRINGLENGTH int stringlength;
#define STRINGLENGTH2 ,stringlength2
#define DEFSTRINGLENGTH2 int stringlength2;
#define STRINGLENGTH3 ,stringlength3
#define DEFSTRINGLENGTH3 int stringlength3;
#define STRINGLENGTH4 ,stringlength4
#define DEFSTRINGLENGTH4 int stringlength4;
#endif

static char *getstring(strdesc STRINGLENGTH)
string_descriptor strdesc;
DEFSTRINGLENGTH
/*  This routine finds a string in the supplied descriptor */
{
	static char stringcopy[MAXSTRING]="\0";
	int ichar;

#ifdef VMS
/* Note that the VMS version of this routine truncates trailing blanks */
	if (strdesc->dsc$b_class != DSC$K_CLASS_S)
		{
		ger_error(
			" Wrong descriptor class in GETSTRING: %d",
	     		strdesc->dsc$b_class);
		stringcopy[0]= '\0';
		};
	if (strdesc->dsc$w_length >= MAXSTRING)
		{
		ger_error(" Long string truncated in GETSTRING.");
		(void) strncpy(stringcopy,
			strdesc->dsc$a_pointer,MAXSTRING-1);
		stringcopy[MAXSTRING-1]='\0';
		}                           
      	else 
		{
		(void) strncpy(stringcopy,
			strdesc->dsc$a_pointer,strdesc->dsc$w_length);
		stringcopy[strdesc->dsc$w_length]='\0';
		};
	for (ichar= strlen(stringcopy)-1;(stringcopy[ichar]==' ')&&(ichar>=0);
		ichar--) stringcopy[ichar]= '\0';                     
#endif
#ifdef CRAY
/* Note that the Cray version of this routine truncates trailing blanks */
	if ( _fcdlen(strdesc) >= MAXSTRING )
		{
		ger_error(" Long string truncated in GETSTRING.");
		(void) strncpy(stringcopy,_fcdtocp(strdesc),MAXSTRING-1);
		stringcopy[MAXSTRING-1]='\0';
		}
	else
		{
		strncpy(stringcopy,_fcdtocp(strdesc),_fcdlen(strdesc));
		stringcopy[_fcdlen(strdesc)]= '\0';
		};
	for (ichar= strlen(stringcopy)-1;(stringcopy[ichar]==' ')&&(ichar>=0);
		ichar--) stringcopy[ichar]= '\0';                     
#endif
#ifdef USE_UNIX
/*   For this version of getstring, the macro STRINGLENGTH translates to
 *   ",stringlength", which should contain the string length.  Note that
 *   this version truncates trailing blanks.
 */
	if (stringlength >= MAXSTRING)
		{
		ger_error(" Long string truncated in GETSTRING.");
		(void) strncpy(stringcopy,strdesc,MAXSTRING-1);
		stringcopy[MAXSTRING-1]='\0';
		}
	else
		{
		strncpy(stringcopy,strdesc,stringlength);
		stringcopy[stringlength]= '\0';
		};
	for (ichar= strlen(stringcopy)-1;(stringcopy[ichar]==' ')&&(ichar>=0);
		ichar--) stringcopy[ichar]= '\0';                     
#endif
#ifdef _IBMR2
/*   For this version of getstring, the macro STRINGLENGTH translates to
 *   ",stringlength", which should contain the string length.  Note that
 *   this version truncates trailing blanks.
 */
	if (stringlength >= MAXSTRING)
		{
		ger_error(" Long string truncated in GETSTRING.");
		(void) strncpy(stringcopy,strdesc,MAXSTRING-1);
		stringcopy[MAXSTRING-1]='\0';
		}
	else
		{
		strncpy(stringcopy,strdesc,stringlength);
		stringcopy[stringlength]= '\0';
		};
	for (ichar= strlen(stringcopy)-1;(stringcopy[ichar]==' ')&&(ichar>=0);
		ichar--) stringcopy[ichar]= '\0';                     
#endif
#ifdef ardent
	if ( strdesc->length >= MAXSTRING )
		{
		ger_error(" Long string truncated in GETSTRING.");
		(void) strncpy(stringcopy,strdesc->addr,MAXSTRING-1);
		stringcopy[MAXSTRING-1]= '\0';
		}
	else 
		{
		(void) strcpy(stringcopy,strdesc->addr);
		stringcopy[strdesc->length]= '\0';
		};
#endif
#ifdef __hpux
/*   For this version of getstring, the macro STRINGLENGTH translates to
 *   ",stringlength", which should contain the string length.  Note that
 *   this version truncates trailing blanks.
 */
	if (stringlength >= MAXSTRING)
		{
		ger_error(" Long string truncated in GETSTRING.");
		(void) strncpy(stringcopy,strdesc,MAXSTRING-1);
		stringcopy[MAXSTRING-1]='\0';
		}
	else
		{
		strncpy(stringcopy,strdesc,stringlength);
		stringcopy[stringlength]= '\0';
		};
	for (ichar= strlen(stringcopy)-1;(stringcopy[ichar]==' ')&&(ichar>=0);
		ichar--) stringcopy[ichar]= '\0';                     
#endif

	ger_debug("getstring: returning <%s>.",stringcopy);

	return(stringcopy);	
}

int pintrn( name, renderer, device, datastr 
	   STRINGLENGTH STRINGLENGTH2 STRINGLENGTH3 STRINGLENGTH4)
string_descriptor name, renderer, device, datastr;
DEFSTRINGLENGTH
DEFSTRINGLENGTH2
DEFSTRINGLENGTH3
DEFSTRINGLENGTH4
{
  char nmstr[MAXSTRING], renstr[MAXSTRING], devstr[MAXSTRING];
  char dtstr[MAXSTRING];

  strcpy( nmstr, getstring(name STRINGLENGTH) );
  strcpy( renstr, getstring(renderer STRINGLENGTH2) );
  strcpy( devstr, getstring(device STRINGLENGTH3) );
  strcpy( dtstr, getstring(datastr STRINGLENGTH4) );
  return( pg_init_ren( nmstr, renstr, devstr, dtstr ) );
}

int popnrn( renderer STRINGLENGTH )
string_descriptor renderer;
DEFSTRINGLENGTH
{
  return( pg_open_ren( getstring(renderer STRINGLENGTH) ) );
}

int pclsrn( renderer STRINGLENGTH )
string_descriptor renderer;
DEFSTRINGLENGTH
{
  return( pg_close_ren( getstring(renderer STRINGLENGTH) ) );
}

int pshtrn( renderer STRINGLENGTH )
string_descriptor renderer;
DEFSTRINGLENGTH
{
  return( pg_shutdown_ren( getstring(renderer STRINGLENGTH) ) );
}

int pshtdn()
{
  return( pg_shutdown() );
}

int pprtrn( renderer STRINGLENGTH )
string_descriptor renderer;
DEFSTRINGLENGTH
{
  return( pg_print_ren( getstring(renderer STRINGLENGTH) ) );
}

int popen( renderer STRINGLENGTH )
string_descriptor renderer;
DEFSTRINGLENGTH
{
  return( pg_open( getstring(renderer STRINGLENGTH) ) );
}

int pclose()
{
  return( pg_close() );
}

int pfree( renderer STRINGLENGTH )
string_descriptor renderer;
DEFSTRINGLENGTH
{
  return( pg_free( getstring(renderer STRINGLENGTH) ) );
}

int piatt( attribute, value STRINGLENGTH )
string_descriptor attribute;
int *value;
DEFSTRINGLENGTH
{
  return( pg_int_attr( getstring( attribute STRINGLENGTH ), *value ) );
}

int pblatt( attribute, value STRINGLENGTH )
string_descriptor attribute;
int *value;
DEFSTRINGLENGTH
{
  return( pg_bool_attr( getstring( attribute STRINGLENGTH ), *value ) );
}

int pfatt( attribute, value STRINGLENGTH )
string_descriptor attribute;
float *value;
DEFSTRINGLENGTH
{
  return( pg_float_attr( getstring( attribute STRINGLENGTH ), *value ) );
}

int pstatt( attribute, value STRINGLENGTH STRINGLENGTH2)
string_descriptor attribute;
string_descriptor value;
DEFSTRINGLENGTH
DEFSTRINGLENGTH2
{
  char attrstr[MAXSTRING];
  strcpy( attrstr, getstring( attribute STRINGLENGTH ) );
  return( pg_string_attr( attrstr,
			 getstring( value STRINGLENGTH2 ) ) );
}

int pclatt( attribute, ctype, r, g, b, a STRINGLENGTH )
string_descriptor attribute;
int *ctype;
float *r, *g, *b, *a;
DEFSTRINGLENGTH
{
  P_Color clr;
  clr.ctype= *ctype;
  clr.r= *r;
  clr.g= *g;
  clr.b= *b;
  clr.a= *a;
  return( pg_color_attr( getstring( attribute STRINGLENGTH ), &clr ) );
}

int pptatt( attribute, x, y, z STRINGLENGTH )
string_descriptor attribute;
float *x, *y, *z;
DEFSTRINGLENGTH
{
  P_Point pt;
  pt.x= *x;
  pt.y= *y;
  pt.z= *z;
  return( pg_point_attr( getstring( attribute STRINGLENGTH ), &pt ) );
}

int pvcatt( attribute, x, y, z STRINGLENGTH )
string_descriptor attribute;
float *x, *y, *z;
DEFSTRINGLENGTH
{
  P_Vector vec;
  vec.x= *x;
  vec.y= *y;
  vec.z= *z;
  return( pg_vector_attr( getstring( attribute STRINGLENGTH ), &vec ) );
}

int ptratt( attribute, val STRINGLENGTH )
string_descriptor attribute;
float *val;
DEFSTRINGLENGTH
{
  P_Transform trans;
  trans.d[0]= *val++;
  trans.d[4]= *val++;
  trans.d[8]= *val++;
  trans.d[12]= *val++;
  trans.d[1]= *val++;
  trans.d[5]= *val++;
  trans.d[9]= *val++;
  trans.d[13]= *val++;
  trans.d[2]= *val++;
  trans.d[6]= *val++;
  trans.d[10]= *val++;
  trans.d[14]= *val++;
  trans.d[3]= *val++;
  trans.d[7]= *val++;
  trans.d[11]= *val++;
  trans.d[15]= *val++;
  return( pg_trans_attr( getstring( attribute STRINGLENGTH ), &trans ) );
}

int pmtatt( attribute, val STRINGLENGTH )
string_descriptor attribute;
int *val;
DEFSTRINGLENGTH
{
  P_Material *material;
  switch (*val) {
  case 0: material= p3d_default_material; break;
  case 1: material= p3d_dull_material; break;
  case 2: material= p3d_shiny_material; break;
  case 3: material= p3d_metallic_material; break;
  case 4: material= p3d_matte_material; break;
  case 5: material= p3d_aluminum_material; break;
  default: 
    ger_error("psmatt: unknown standard material ID %d, call ignored.\n",
	      *val);
    return P3D_FAILURE;
  }
  return( pg_material_attr( getstring( attribute STRINGLENGTH ), material ) );
}

int pgbclr( ctype, r, g, b, a )
int *ctype;
float *r, *g, *b, *a;
{
  P_Color clr;
  clr.ctype= *ctype;
  clr.r= *r;
  clr.g= *g;
  clr.b= *b;
  clr.a= *a;
  return( pg_gobcolor( &clr ) );
}

int ptxtht( value )
float *value;
{
  return( pg_textheight( *value ) );
}

int pbkcul( value )
int *value;
{
  return( pg_backcull( *value ) );
}

int pgbmat( val )
int *val;
{
  P_Material *material;
  switch (*val) {
  case 0: material= p3d_default_material; break;
  case 1: material= p3d_dull_material; break;
  case 2: material= p3d_shiny_material; break;
  case 3: material= p3d_metallic_material; break;
  case 4: material= p3d_matte_material; break;
  case 5: material= p3d_aluminum_material; break;
  default: 
    ger_error("pgbsmt: unknown standard material ID %d, call ignored.\n",
	      *val);
    return P3D_FAILURE;
  }
  return( pg_gobmaterial( material ) );
}

int ptnsfm( val )
float *val;
{
  P_Transform trans,*tmp;
  trans.d[0]= *val++;
  trans.d[4]= *val++;
  trans.d[8]= *val++;
  trans.d[12]= *val++;
  trans.d[1]= *val++;
  trans.d[5]= *val++;
  trans.d[9]= *val++;
  trans.d[13]= *val++;
  trans.d[2]= *val++;
  trans.d[6]= *val++;
  trans.d[10]= *val++;
  trans.d[14]= *val++;
  trans.d[3]= *val++;
  trans.d[7]= *val++;
  trans.d[11]= *val++;
  trans.d[15]= *val++;
  
  trans.type_front = allocate_trans_type();
  trans.type_front->type = P3D_TRANSFORMATION;
  tmp = duplicate_trans(&trans);
  trans.type_front->trans = (P_Void_ptr)tmp;
  trans.type_front->generators[0] = 0;
  trans.type_front->generators[1] = 0;
  trans.type_front->generators[2] = 0;
  trans.type_front->generators[3] = 0;
  trans.type_front->next = NULL;

  return( pg_transform( &trans ) );
}

int ptrans( x, y, z )
float *x, *y, *z;
{
  return( pg_translate( *x, *y, *z ) );
}

int protat( x, y, z, angle )
float *x, *y, *z, *angle;
{
  P_Vector vec;
  vec.x= *x;
  vec.y= *y;
  vec.z= *z;
  return( pg_rotate( &vec, *angle ) );
}

int pscale( factor )
float *factor;
{
  return( pg_scale(*factor) );
}

int pascal( x, y, z )
float *x, *y, *z;
{
  return( pg_ascale( *x, *y, *z ) );
}

int pchild( name STRINGLENGTH )
string_descriptor name;
DEFSTRINGLENGTH
{
  return( pg_child( getstring( name STRINGLENGTH ) ) );
}

int pprtgb( name STRINGLENGTH )
string_descriptor name;
DEFSTRINGLENGTH
{
  return( pg_print_gob( getstring( name STRINGLENGTH ) ) );
}

int paxis( startf, endf, upf, startval, endval, numtics, label, 
           textht, prcsn  STRINGLENGTH )
float *startf, *endf, *upf, *startval, *endval;
int *numtics;
string_descriptor label;
float *textht;
int *prcsn;
DEFSTRINGLENGTH
{
  P_Point start, end;
  P_Vector up;
  
  start.x = *startf++;
  start.y = *startf++;
  start.z = *startf;
  end.x = *endf++;
  end.y = *endf++;
  end.z = *endf;
  up.x = *upf++;
  up.y = *upf++;
  up.z = *upf;
  return( pg_axis( &start, &end, &up, (double) *startval, (double) *endval, 
          *numtics, getstring( label STRINGLENGTH ), 
          ( double) *textht, *prcsn ) );
}

int pbndbx( cornera, cornerb)
float *cornera;
float *cornerb;
{
  P_Point corner1, corner2;

  corner1.x = *cornera++;
  corner1.y = *cornera++;
  corner1.z = *cornera;
  corner2.x = *cornerb++;
  corner2.y = *cornerb++;
  corner2.z = *cornerb;
  return( pg_boundbox( &corner1, &corner2 ) );
}

int pisosf( type, data, valdata, nx, ny, nz, value, corner1f, corner2f,
	   show_inside )
int *type; 
float *data, *valdata;
int *nx, *ny, *nz; 
float *value;
float *corner1f, *corner2f;
int *show_inside;
{
  P_Point corner1, corner2;
  double dblval;
  dblval= *value;
  corner1.x= *corner1f++;
  corner1.y= *corner1f++;
  corner1.z= *corner1f;
  corner2.x= *corner2f++;
  corner2.y= *corner2f++;
  corner2.z= *corner2f;
  return( pg_isosurface( *type, data, valdata, *nx, *ny, *nz, dblval,
			&corner1, &corner2, *show_inside, 1 ) );
}

int pzsurf( vtxtype, zdata, valdata, nx, ny, corner1f, corner2f, 
           null_tfun, testfun )
int *vtxtype;
float *zdata, *valdata;
int *nx, *ny;
float *corner1f, *corner2f;
int *null_tfun;
void (*testfun)();
{
  P_Point corner1, corner2;
 
  corner1.x = *corner1f++;
  corner1.y = *corner1f++;
  corner1.z = *corner1f;
  corner2.x = *corner2f++;
  corner2.y = *corner2f++;
  corner2.z = *corner2f;
  if (*null_tfun)
    testfun = 0;
  return( pg_zsurface( *vtxtype, zdata, valdata, *nx, *ny, &corner1, 
		      &corner2, testfun, 1 ) );
}

int prnzsf( vtxtype, ctype, npts, coords, colors, norms, null_tfun, testfun )
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors, *norms;
int *null_tfun;
void (*testfun)();
{
  if (*null_tfun)
    testfun= 0;
  return( pg_rand_zsurf( po_create_mvlist( *vtxtype, *npts, 
					  coords, colors, norms ),
			testfun ) );
}

int prniso( vtxtype, ctype, npts, coords, colors, norms, ival, show_inside )
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors, *norms;
float *ival;
int *show_inside;
{
  double dblval;
  int retval;
  P_Vlist* vlist= po_create_mvlist( *vtxtype, *npts, coords, colors, norms );
  dblval= *ival;
  retval= pg_rand_isosurf( vlist, dblval, *show_inside );
  METHOD_RDY(vlist);
  (*(vlist->destroy_self))();
  return( retval );    
}

int pirzsf( vtxtype, zdata, valdata, nx, ny, xyfun, null_tfun, tstfun )
int *vtxtype;
float *zdata, *valdata;
int *nx, *ny;
void (*xyfun)();
int *null_tfun;
void (*tstfun)();
{
  if (*null_tfun) tstfun = 0;
  return( pg_irreg_zsurf( *vtxtype, zdata, valdata, *nx, *ny, 
			 xyfun, tstfun, 1 ) );
}

int piriso( type, data, valdata, nx, ny, nz, value, coordfun, show_inside )
int *type;
float *data;
float *valdata;
int *nx;
int *ny;
int *nz;
float *value;
void (*coordfun)();
int *show_inside;
{
  double dblval;
  dblval= *value;
  return( pg_irreg_isosurf( *type, data, valdata, *nx, *ny, *nz, dblval,
			   coordfun, *show_inside, 1 ) );
}

int ptbmol( vtxtype, ctype, npts, coords, colors, cross, bres, cres )
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors;
int *cross;
int *bres;
int *cres;
{
  int retval;
  P_Vlist* vlist;
  int mytype= *vtxtype;
  if (mytype==P3D_CNVTX) mytype= P3D_CVTX;
  if (mytype==P3D_CCNVTX) mytype= P3D_CCVTX;
  if (mytype==P3D_CVNVTX) mytype= P3D_CVVTX;
  vlist= po_create_mvlist( mytype, *npts, coords, colors, (float*)0 );
  retval= pg_spline_tube( vlist, cross, *bres, *cres );
  METHOD_RDY(vlist);
  (*(vlist->destroy_self))();
  return( retval );    
}

int pcyl()
{
  return( pg_cylinder() );
}

int psphr()
{
  return( pg_sphere() );
}

int ptorus( major, minor )
float *major, *minor;
{
  return( pg_torus( *major, *minor ) );
}

int pplymk( vtxtype, ctype, npts, coords, colors )
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors;
{
  int mytype;

  mytype= *vtxtype;
  if (mytype==P3D_CNVTX) mytype= P3D_CVTX;
  if (mytype==P3D_CCNVTX) mytype= P3D_CCVTX;
  if (mytype==P3D_CVNVTX) mytype= P3D_CVVTX;
  return( pg_polymarker( 
	     po_create_mvlist(mytype, *npts, coords, colors, (float *)0) ) );
}

int pplyln( vtxtype, ctype, npts, coords, colors )
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors;
{
  int mytype;

  mytype= *vtxtype;
  if (mytype==P3D_CNVTX) mytype= P3D_CVTX;
  if (mytype==P3D_CCNVTX) mytype= P3D_CCVTX;
  if (mytype==P3D_CVNVTX) mytype= P3D_CVVTX;
  return( pg_polyline( 
	     po_create_mvlist(mytype, *npts, coords, colors, (float *)0) ) );
}

int pplygn( vtxtype, ctype, npts, coords, colors, normals )
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors, *normals;
{
  return( pg_polygon( 
	     po_create_mvlist(*vtxtype, *npts, coords, colors, normals) ) );
}

int ptrist( vtxtype, ctype, npts, coords, colors, normals )
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors, *normals;
{
  return( pg_tristrip( 
	     po_create_mvlist(*vtxtype, *npts, coords, colors, normals) ) );
}

int pmesh( vtxtype, ctype, npts, coords, colors, normals, nfacets,
	  facet_lengths, vertices)
int *vtxtype;
int *ctype;
int *npts;
float *coords, *colors, *normals;
int *nfacets;
int *facet_lengths;
int *vertices;
{
  return( pg_mesh( po_create_mvlist(*vtxtype, *npts, coords, colors, normals),
		  vertices, facet_lengths, *nfacets) );
}

int pbezp( vtxtype, ctype, coords, colors, normals )
int *vtxtype;
int *ctype;
float *coords, *colors, *normals;
{
  return( pg_bezier( 
	     po_create_mvlist(*vtxtype, 16, coords, colors, normals) ) );
}

int ptext( string, location, u, v STRINGLENGTH )
string_descriptor string;
float *location;
float *u;
float *v;
DEFSTRINGLENGTH
{
  P_Point locpt;
  P_Vector uvec, vvec;

  locpt.x= *location;
  locpt.y= *(location+1);
  locpt.z= *(location+2);
  uvec.x= *u;
  uvec.y= *(u+1);
  uvec.z= *(u+2);
  vvec.x= *v;
  vvec.y= *(v+1);
  vvec.z= *(v+2);
  return( pg_text( getstring(string STRINGLENGTH), &locpt, &uvec, &vvec ) );
}

int plight( location, ctype, color )
float *location;
int *ctype;
float *color;
{
  P_Point locpt;
  P_Color colval;

  locpt.x= *location++;
  locpt.y= *location++;
  locpt.z= *location;
  colval.ctype= *ctype;
  colval.r= *color++;
  colval.g= *color++;
  colval.b= *color++;
  colval.a= *color;
  return( pg_light( &locpt, &colval ) );
}

int pamblt( ctype, color )
int *ctype;
float *color;
{
  P_Color colval;

  colval.ctype= *ctype;
  colval.r= *color++;
  colval.g= *color++;
  colval.b= *color++;
  colval.a= *color;
  return( pg_ambient( &colval ) );
}

int pcamra( name, lookfrom, lookat, up, fovea, hither, yon STRINGLENGTH )
string_descriptor name;
float *lookfrom, *lookat, *up, *fovea, *hither, *yon;
DEFSTRINGLENGTH
{
  P_Point frompt, atpt;
  P_Vector upvec;

  frompt.x= *lookfrom++;
  frompt.y= *lookfrom++;
  frompt.z= *lookfrom;
  atpt.x= *lookat++;
  atpt.y= *lookat++;
  atpt.z= *lookat;
  upvec.x= *up++;
  upvec.y= *up++;
  upvec.z= *up;
  return( pg_camera( getstring(name STRINGLENGTH),
		    &frompt, &atpt, &upvec, *fovea, *hither, *yon ) );
}

int pprtcm( name STRINGLENGTH )
string_descriptor name;
DEFSTRINGLENGTH
{
  return( pg_print_camera( getstring(name STRINGLENGTH) ) );
}

int pcmbkg( name, ctype, color STRINGLENGTH )
string_descriptor name;
int *ctype;
float *color;
DEFSTRINGLENGTH
{
  P_Color colval;

  colval.ctype= *ctype;
  colval.r= *color++;
  colval.g= *color++;
  colval.b= *color++;
  colval.a= *color;
  return( pg_camera_background( getstring(name STRINGLENGTH), &colval ) );
}

int psnap( model, lights, camera STRINGLENGTH STRINGLENGTH2 STRINGLENGTH3 )
string_descriptor model;
string_descriptor lights;
string_descriptor camera;
DEFSTRINGLENGTH
DEFSTRINGLENGTH2
DEFSTRINGLENGTH3
{
  char modelstr[MAXSTRING], lightstr[MAXSTRING], camstr[MAXSTRING];

  strcpy( modelstr, getstring(model STRINGLENGTH) );
  strcpy( lightstr, getstring(lights STRINGLENGTH2) );
  strcpy( camstr, getstring(camera STRINGLENGTH3) );
  return( pg_snap( modelstr, lightstr, camstr ) );
}

int pstcmp(min, max, fun)
float *min;
float *max;
void (*fun)();
{
  return( pg_set_cmap( *min, *max, fun ) );
}

int psdcmp(min, max, which)
float *min;
float *max;
int *which;
{
  return( pg_std_cmap( *min, *max, *which ) );
}

int pdebug()
{
  ger_toggledebug();
  return( P3D_SUCCESS );
}
