/****************************************************************************
 * assist_text.c
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
The 'assist'  module provides support for renderers which do not have the
capability to do certain operations.  The operations are simulated using
a facility which the renderer (hopefully) does have.  This module provides
text facilities using the renderer's polyline function and transformation
function.  Because the text depends on the current attributes, this
module can only be used to render text if the renderer uses the attribute
assistance module to maintain its attribute state.
*/

/* Notes-
   -We could avoid clearing the cache so often if transformations were
    used to change the height and the u and v of text.  
*/

#include <math.h>
#include <ctype.h>
#include <string.h>
#include "ge_error.h"
#include "p3dgen.h"
#include "pgen_objects.h"
#include "assist.h"
#include "hershey.h"  /* hershey.h provides a set of Hershey fonts */

/* Some defaults */
#define DEFAULT_FONT_ID 0
#define DEFAULT_HEIGHT 1.0
#define INITIAL_COORD_BUF_SIZE 30

/* The following struct holds the info in a text gob */
typedef struct P_Text_Gob_struct {
  char *string;
  P_Point loc;
  P_Vector u;
  P_Vector v;
} P_Text_Gob;

/* The following buffer and size are used for coordinate intermediate space
 * by all instances of the assist object.
 */
static int coord_buf_size= 0;
static float *coord_buf= (float *)0;

/* The following macros simplify access to the Hershey font data structures */
#define this_char( self, character ) \
	char_table[ font_table[TEXT_FONT_ID(self)].first_char + character ]
#define this_stroke( self, character, strokeid ) \
	stroke_table[ this_char( self, character ).first_stroke + strokeid ]
#define this_vertex( self, character, strokeid, vtxid ) \
	vertex_table[ this_stroke(self,character,strokeid).start + vtxid ] 
#define this_vertex_x( self, character, strokeid, vtxid ) \
      ( (int)this_vertex( self, character, strokeid, vtxid )[0] - COORD_BIAS )
#define this_vertex_y( self, character, strokeid, vtxid ) \
      ( (int)this_vertex( self, character, strokeid, vtxid )[1] - COORD_BIAS )

static void check_coord_buf( int size )
/* Grow the coordinate buffer to a needed size */
{
  if ( size > coord_buf_size ) {
    ger_debug("assist_text: check_coord_buf: growing to size %d",size);
    if (coord_buf) 
      coord_buf= (float *)realloc( coord_buf, 3*size*sizeof(float) );
    else coord_buf= (float *)malloc( 3*size*sizeof(float) );
    if (!coord_buf)
      ger_fatal("assist_text: check_coord_buf: unable to grow to %d floats!",
		3*size);
    coord_buf_size= size;
  }
}

/* This routine releases all cached characters */
static void release_cache(P_Assist *self)
{
  int ichar;
  
  ger_debug("assist_text: release_cache: resetting character cache");
  
  for (ichar=0; ichar<MAX_FONT_CHARS; ichar++)
    if (FONT_CACHE(self)[ichar].ready) {
      free( (P_Void_ptr)(FONT_CACHE(self)[ichar].data) );
      FONT_CACHE(self)[ichar].data= (P_Void_ptr *)0;
      FONT_CACHE(self)[ichar].ready= 0;
    };
}

/* 
This routine checks for the presence of a character in the cache, and
if it is not present adds it.
*/
static void setup_char( P_Assist *self, char txtchar, 
		       float ux, float uy, float uz, 
		       float vx, float vy, float vz )
{
  int vcount, vloop, nstrokes, nrealstrokes, strokeloop;
  float x, y, z;
  float *coord;
  P_Void_ptr *data;
  P_Vlist *vlist;
  
  /* If the character is not cached, cache it */
  if (!FONT_CACHE(self)[txtchar].ready) {
    ger_debug("assist_text: setup_char: caching '%c'",txtchar);
    
    nstrokes= this_char( self,txtchar ).stroke_count;
    if ( !(data= (P_Void_ptr *)malloc(nstrokes*sizeof(P_Void_ptr *))) )
      ger_fatal("assist_text: setup_char: unable to allocate %d bytes!",
		nstrokes*sizeof(P_Void_ptr *));

    FONT_CACHE(self)[txtchar].data= data;
    nrealstrokes= 0;
    for (strokeloop=0; strokeloop<nstrokes; strokeloop++) {

      vcount= this_stroke( self, txtchar, strokeloop ).count;

      if (vcount>1) { /* watch out for zero length strokes */
	nrealstrokes++;
	check_coord_buf(vcount); /* make sure there's space in the buffer */
	coord= coord_buf;

	for (vloop=0; vloop<vcount; vloop++ ) {
	  *coord++= FONT_SCALE * TEXT_HEIGHT(self) * 
	    (this_vertex_x( self, txtchar, strokeloop, vloop ) * ux +
	     this_vertex_y( self, txtchar, strokeloop, vloop ) * vx );
	  *coord++= FONT_SCALE * TEXT_HEIGHT(self) * 
	    (this_vertex_x( self, txtchar, strokeloop, vloop ) * uy +
	     this_vertex_y( self, txtchar, strokeloop, vloop ) * vy );
	  *coord++= FONT_SCALE * TEXT_HEIGHT(self) * 
	    (this_vertex_x( self, txtchar, strokeloop, vloop ) * uz +
	     this_vertex_y( self, txtchar, strokeloop, vloop ) * vz );
	};

	vlist= po_create_cvlist(P3D_CVTX, vcount, coord_buf);
	METHOD_RDY(RENDERER(self));
	*data++= (*(RENDERER(self)->def_polyline))("",vlist);
	METHOD_RDY(vlist);
	(*(vlist->destroy_self))();
      }
    };
    FONT_CACHE(self)[txtchar].length= nrealstrokes;
    FONT_CACHE(self)[txtchar].ready= 1;
  }
  else ger_debug("assist_text: setup_char: '%c' ready",txtchar);
  
}

/* This function actually emits the character */
static void do_char( P_Assist *self, char txtchar, 
		    float ux, float uy, float uz, 
		    float vx, float vy, float vz, 
		    P_Transform *trans, P_Attrib_List *attr )
{
  P_Void_ptr *data;
  int strokeloop;
  
  ger_debug("assist_text: do_char: '%c'", txtchar);
  
  /* If the character is not in the font table, substitute for it */
  if ( this_char(self,txtchar).first_stroke == -1 ) txtchar= '?';
  if ( this_char(self,txtchar).first_stroke == -1 ) return;
  
  /* Make sure the character is in the cache */
  setup_char( self, txtchar, ux, uy, uz, vx, vy, vz );

  /* Walk the array of renderer data associated with the character,
   * drawing each polyline.
   */
  data= FONT_CACHE(self)[txtchar].data;
  for (strokeloop=0; strokeloop<FONT_CACHE(self)[txtchar].length; 
       strokeloop++) {
    METHOD_RDY(RENDERER(self));
    (*(RENDERER(self)->ren_polyline))(*data++, trans, attr);
  };
}

/* 
This routine looks up the current font name in the font table, returning
the appropriate font id.  If the name is not found, an error message
is emitted and id 0 is returned.
*/
static int font_lookup( font )
char *font;
{
  int iloop;
  char *thischar,*fontp;
  int found;

  ger_debug("assist_text: font_lookup: looking for font <%s>",font);

  for (iloop=0; iloop<NO_FONTS; iloop++) {
    thischar= font_table[iloop].name;
    fontp= font;
    found= 1;
    while (*thischar != ':') thischar++; /* pass up "HERSHEY" */
    thischar++; /* pass up ":" */
    while ( *thischar 
	   && ((thischar - font_table[iloop].name) < FONT_NM_LENGTH) )
      if (*thischar++ != toupper(*fontp++)) {
	found= 0;
	break;
      }
    if (found) return(iloop);
  }
  ger_error("font_lookup: font <%s> not found; using <%s>",
	    font, font_table[0].name);
  return(0);
}

/* 
This routine extracts and implements text attributes from the given
attribute list.
*/
static void check_textattr( P_Assist *self )
{
  int clear_now= 0;
  float new_height;
  char *new_font;
  
  ger_debug("assist_text: check_textattr");
  
  if (!TEXT_SYMBOLS_READY(self)) {
    TEXT_HEIGHT_SYMBOL(self)= create_symbol("text-height");
    TEXT_FONT_SYMBOL(self)= create_symbol("text-font");
    TEXT_SYMBOLS_READY(self)= 1;
  };
  
  METHOD_RDY(self);
  new_height= (*(self->float_attribute))( TEXT_HEIGHT_SYMBOL(self) );
  ger_debug("           Text height= %f", new_height);
  if ( new_height != TEXT_HEIGHT(self) ) {
    TEXT_HEIGHT(self)= new_height;
    clear_now= 1;
  };
  
  new_font= (*(self->string_attribute))( TEXT_FONT_SYMBOL(self) );
  ger_debug("           Text font= <%f>", new_font);
  if ( strncmp(new_font,TEXT_FONT(self),FONT_NM_LENGTH) ) {
    strncpy(TEXT_FONT(self),new_font,FONT_NM_LENGTH);
    TEXT_FONT_ID(self)= font_lookup(TEXT_FONT(self));
    clear_now= 1;
  };
  
  if (clear_now) release_cache(self);
}

static void ren_text( P_Void_ptr rendata, P_Transform *trans, 
		     P_Attrib_List *attr )
{
  P_Assist *self= (P_Assist *)po_this;
  P_Text_Gob *text= (P_Text_Gob *)rendata;
  char *txtstring;
  float x0, y0, z0, ux, uy, uz, vx, vy, vz, unorm, vnorm;
  P_Transform *shifttrans,*nettrans;
  METHOD_IN
  
  ger_debug("assist_text: ren_text: rendering <%s>",text->string);
  
  /* Handle text attributes, if they are present. */
  check_textattr(self);
  
  /* 
    Extract u and v vector components, and reset cache if they've
    changed.
    */
  ux= text->u.x; uy= text->u.y; uz= text->u.z; 
  vx= text->v.x; vy= text->v.y; vz= text->v.z; 
  /* Checked at definition time to make sure they weren't null */
  unorm= sqrt( ux*ux + uy*uy + uz*uz );
  vnorm= sqrt( vx*vx + vy*vy + vz*vz );
  ux= ux/unorm; uy= uy/unorm; uz= uz/unorm;
  vx= vx/vnorm; vy= vy/vnorm; vz= vz/vnorm;
  
  if ( (ux != OLDU(self)[0]) || (uy != OLDU(self)[1]) 
      || (uz != OLDU(self)[2])
      || (vx != OLDV(self)[0]) || (vy != OLDV(self)[1]) 
      || (vz != OLDV(self)[2]) ) {
    release_cache(self);
    OLDU(self)[0]= ux;
    OLDU(self)[1]= uy;
    OLDU(self)[2]= uz;
    OLDV(self)[0]= vx;
    OLDV(self)[1]= vy;
    OLDV(self)[2]= vz;
  };
  
  /* Extract text coordinates */
  x0= text->loc.x;
  y0= text->loc.y;
  z0= text->loc.z;
  
  txtstring= text->string;
  for (; *txtstring; txtstring++) {
    /* Translate to the starting point */
    shifttrans= 
      translate_trans(x0 + FONT_SCALE * TEXT_HEIGHT(self) * 
		      (float)( ux*this_char(self,*txtstring).xcenter ),
		      y0 + FONT_SCALE * TEXT_HEIGHT(self) * 
		      (float)( uy*this_char(self,*txtstring).xcenter ),
		      z0 + FONT_SCALE * TEXT_HEIGHT(self) * 
		      (float)( uz*this_char(self,*txtstring).xcenter )
		      );
    if (trans) nettrans= premult_trans(shifttrans,trans);
    else nettrans= shifttrans;
    
    /* Draw the character */
    do_char( self, *txtstring, ux, uy, uz, vx, vy, vz, nettrans, attr );

    /* clean up */
    destroy_trans( shifttrans );
    if (trans) destroy_trans( nettrans );
    
    /* advance point to next character starting position */
    x0 += FONT_SCALE * TEXT_HEIGHT(self) *
      (float)(ux * this_char(self,*txtstring).xshift);
    y0 += FONT_SCALE * TEXT_HEIGHT(self) *
      (float)(uy * this_char(self,*txtstring).xshift);
    z0 += FONT_SCALE * TEXT_HEIGHT(self) *
      (float)(uz * this_char(self,*txtstring).xshift);
  };
  METHOD_OUT
}

static P_Void_ptr def_text( char *tstring, P_Point *location, 
			   P_Vector *u, P_Vector *v )
/* Define a text string */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Text_Gob *result;
  METHOD_IN

  ger_debug("assist_text: def_text: defining <%s>",tstring);

  if ( !(result= (P_Text_Gob *)malloc(sizeof(P_Text_Gob))) )
    ger_fatal("assist_text: def_text: unable to allocate %d bytes!",
	      sizeof(P_Text_Gob));

  if ( !(result->string= (char *)malloc( strlen(tstring)+1 )) )
    ger_fatal("assist_text: def_text: unable to allocate %d bytes!",
	      strlen(tstring)+1);
  (void)strcpy(result->string,tstring);

  result->loc.x= location->x;
  result->loc.y= location->y;
  result->loc.z= location->z;
  (void)vec_copy( &(result->u), u );
  (void)vec_copy( &(result->v), v );
  if ( (result->u.x==0.0) && (result->u.y==0.0) && (result->u.z==0.0) ) {
    ger_error("assist_text: def_text: null u vector; using x-vec");
    result->u.x= 1.0; 
    result->u.y= 0.0; 
    result->u.z= 0.0;
  }
  if ( (result->v.x==0.0) && (result->v.y==0.0) && (result->v.z==0.0) ) {
    ger_error("assist_text: def_text: null v vector; using y-vec");
    result->v.x= 0.0; 
    result->v.y= 1.0; 
    result->v.z= 0.0;
  }

  METHOD_OUT
  return((P_Void_ptr)result);
}

static void destroy_text( P_Void_ptr rendata )
/* Destroy a text string */
{
  P_Assist *self= (P_Assist *)po_this;
  P_Text_Gob *text= (P_Text_Gob *)rendata;
  METHOD_IN

  ger_debug("assist_text: destroy_text: destroying <%s>",text->string);

  free( (P_Void_ptr)(text->string));
  free( (P_Void_ptr)text );

  METHOD_OUT
}

void ast_text_reset( int hard )
/* This routine resets the attribute part of the assist module, in the 
 * event its symbol environment is reset.
 */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_text: ast_text_reset: hard= %d",hard);

  if (hard) {
    TEXT_SYMBOLS_READY(self)= 0;
  }

  METHOD_OUT
}

void ast_text_destroy( void )
/* This destroys the text part of an assist object */
{
  P_Assist *self= (P_Assist *)po_this;
  METHOD_IN

  ger_debug("assist_text: ast_text_destroy");
  if (coord_buf) {
    free( (P_Void_ptr)coord_buf );
    coord_buf= (float *)0;
    coord_buf_size= 0;
  }

  METHOD_OUT
}

void ast_text_init( P_Assist *self )
/* This initializes the attribute part of an assist object */
{
  int ichar= 0;

  ger_debug("assist_text: ast_text_init");

  /* If P3D_NAMELENGTH from p3dgen.h is set to a value smaller than
   * FONT_NM_LENGTH in hershey.h, logic errors can result.  So, we
   * check that here and quit if it is so.
   */
  if (P3D_NAMELENGTH < FONT_NM_LENGTH)
    ger_fatal("assist_text: ast_text_init: \
compiled with P3D_NAMELENGTH less than FONT_NM_LENGTH!");

  /* Initialize coordinate buffer */
  check_coord_buf( INITIAL_COORD_BUF_SIZE );

  /* Initialize object data */
  TEXT_SYMBOLS_READY(self)= 0;
  OLDU(self)[0]= OLDU(self)[1]= OLDU(self)[2]= 0.0;
  OLDV(self)[0]= OLDV(self)[1]= OLDV(self)[2]= 0.0;
  TEXT_HEIGHT(self)= DEFAULT_HEIGHT;
  TEXT_FONT_ID(self)= DEFAULT_FONT_ID;
  TEXT_FONT(self)[0]= '\0';

  /* Mark the font cache empty */
  for (ichar=0; ichar<MAX_FONT_CHARS; ichar++)
    FONT_CACHE(self)[ichar].ready= 0;

  /* Fill out the methods */
  self->def_text= def_text;
  self->ren_text= ren_text;
  self->destroy_text= destroy_text;

}

