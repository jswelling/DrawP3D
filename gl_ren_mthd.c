/****************************************************************************
 * gl_ren_mthd.c
 * Author Robert Earhart
 * Copyright 1992, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides renderer methods for the IRIS gl renderer
*/

#include <stdio.h>
#include <math.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "ge_error.h"
#include "assist.h"
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include <gl/gl.h>
#include <gl/sphere.h>
#include "gl_strct.h"

#define GLPROF(x)
    
#define BLACKPATTERN 0
#define GREYPATTERN 1

#define N_MATERIALS 6
#define N_ENTRIES 15
static float materials[N_MATERIALS][N_ENTRIES] = {
    /*default_material*/
    {
	AMBIENT, .5, .5, .5,
	DIFFUSE, .5, .5, .5,
	SPECULAR, .5, .5, .5,
	SHININESS, 30,
	LMNULL
	},
    /*dull_material*/
    {
	AMBIENT, .9, .9, .9,
	DIFFUSE, .9, .9, .9,
	SPECULAR, .1, .1, .1,
	SHININESS, 5.0,
	LMNULL
	},
    /*shiny_material*/
    {
	AMBIENT, .5, .5, .5,
	DIFFUSE, .5, .5, .5,
	SPECULAR, .5, .5, .5,
	SHININESS, 50.0,
	LMNULL
	},
    /*metallic_material*/
    {
	AMBIENT, .1, .1, .1,
	DIFFUSE, .1, .1, .1,
	SPECULAR, .9, .9, .9,
	SHININESS, 100.0,
	LMNULL
	},
    /*matte_material*/
    {
	AMBIENT, 1.0, 1.0, 1.0,
	DIFFUSE, 1.0, 1.0, 1.0,
	SPECULAR, 0.0, 0.0, 0.0,
	SHININESS, 0.0,
	LMNULL
	},
    /*aluminum_material*/
    {
	AMBIENT, .25, .25, .25,
	DIFFUSE, .25, .25, .25,
	SPECULAR, .75, .75, .75,
	SHININESS, 6.0,
	LMNULL
	}    
};

static float initiallm[]={
    AMBIENT, .6, .6, .6,
    LOCALVIEWER, 0.0,
    TWOSIDE, 0.0,
    LMNULL};    

static unsigned short greyPattern[16] =
{
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA
    };

static short hastransparent;

static short light_no = 3;

static unsigned long gd_min; /*Value used to clear zbuffer*/

/*Definitions for cylinder nurbs*/

#define CYL_NUMKNOTSX	4
#define CYL_NUMKNOTSY	12
#define CYL_NUMCOORDS	3
#define CYL_ORDERX      2
#define CYL_ORDERY      6
#define CYL_NUMPOINTSX	(CYL_NUMKNOTSX - CYL_ORDERX)
#define CYL_NUMPOINTSY	(CYL_NUMKNOTSY - CYL_ORDERY)

/*So we don't have to realloc these EVERY TIME (primitive caching)*/
static gl_gob *my_cylinder = NULL, *my_sphere = NULL;

static void destroy_object(P_Void_ptr the_thing) {
    /*Destroys any object.*/
    
    gl_gob *it = (gl_gob *)the_thing;
    P_Renderer *self = (P_Renderer *)po_this;

    METHOD_IN

    ger_debug("gl_ren_mthd: destroy_object\n");

    if (it == my_cylinder) {
	METHOD_OUT
	return;
    }

    if (it == my_sphere) {
	METHOD_OUT
	return;
    }
    
    if (it) {
	if (isobj(it->obj))
	    delobj(it->obj);	
	free((void *)it);
    }
    METHOD_OUT
}

static void ren_transform(P_Transform trans) {
    register short lupe, loope, lup;
    Matrix theMatrix;
    
    for (lupe=lup=0;lupe<16;lupe+=4, lup++)
	for (loope=0;loope<4;loope++)
	    theMatrix[loope][lup]=trans.d[lupe+loope];
    multmatrix(theMatrix);
}

static void ren_object(P_Void_ptr the_thing, P_Transform *transform,
		P_Attrib_List *attrs) {
    
    /*P_Transform is an struct which contains float d[16];*/
    /*P_Attrib is a struct of
        P_Symbol attribute; where P_Symbol is just a P_Void_ptr
        int type; where type is te value type
        P_Void_ptr value;
        and two structs for the next and prev things in the list.
    */
    
    P_Renderer *self = (P_Renderer *)po_this;
    P_Color *pcolor;
    float color[4];
    P_Material *mat;
    METHOD_IN
	
    gl_gob *it = (gl_gob *)the_thing;
    
    if (RENDATA(self)->open) {
	ger_debug("gl_ren_mthd: ren_object");
	if (!it) {
	    ger_error("gl_ren_mthd: ren_object: null object data found.");
	    METHOD_OUT
	    return;
	}

	/*get the attributes*/
	METHOD_RDY(ASSIST(self));
	pcolor= (*(ASSIST(self)->color_attribute))(COLORSYMBOL(self));
	backface((*(ASSIST(self)->bool_attribute))(BACKCULLSYMBOL(self)));
	mat= (*(ASSIST(self)->material_attribute))(MATERIALSYMBOL(self));
	
	/*prepare attributes*/
	/*we've already defined all of our materials at open time... :)*/
	/*Note: there is a significant performance penalty for frequently
	  changing the current material via lmbind...*/

	if (CURMATERIAL(self) != (mat->type + 1))
	    if (mat->type < N_MATERIALS) {
		lmbind(MATERIAL, mat->type+1);
		CURMATERIAL(self) = mat->type + 1;
	    } else {
		lmbind(MATERIAL, 1);
		CURMATERIAL(self) = 1;
	    }

	/*Set color mode...*/
	lmcolor(it->color_mode); /*Some like it AD, some like it COLOR...*/

	/* What we're doing in the above call is distinguishing between the
	 * case in which we're updating the COLOR of an object and the case
	 * in which we're updating the color of the MATERIAL of the object.
	 * 
	 * This is a Really Important Distinction. Things with inherent color
	 * attributes are updating their COLOR at various spots, while things
	 * without color attributes, i.e. surfaces of vertices and normals,
	 * must update their materials.
	 *
	 * I guess this'd be easier if we didn't implement materials, but...
	 */
	
	/*Set color...*/
	rgbify_color(pcolor);

	color[0]= pcolor->r;
	color[1]= pcolor->g;
	color[2]= pcolor->b;
	color[3]= pcolor->a;
	c4f(color);

	if ((! hastransparent) && (color[3] < .5)) {
	    setpattern(GREYPATTERN);
	    zwritemask(0);
	}

	/*we should almost never need this segment, but then again,
	  we might... */
	if (transform) {
	    pushmatrix();
	    ren_transform(*transform);
	}
	
	/*and render*/
	callobj(it->obj);
	/*And restore*/
	if (transform)
	    popmatrix();

	if ((! hastransparent) && (color[3] < .5)) {
	    setpattern(BLACKPATTERN);
	    zwritemask(0xFFFFFFFF);

	lmcolor(LMC_COLOR);
	}

	/*Gee, wasn't that easy? :)  */
	METHOD_OUT
     }
}

static P_Void_ptr def_sphere(char *name) {
    /*Define a unit sphere at the origin and return it.*/
    
  P_Renderer *self= (P_Renderer *)po_this;
  gl_gob *it;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: def_sphere");

#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_sphere))();
    METHOD_OUT
    return( (P_Void_ptr)result );
#else
    if (my_sphere) {
	METHOD_OUT
	return((P_Void_ptr)my_sphere);
    }

    if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	ger_fatal("def_sphere: unable to allocate %d bytes!", sizeof(gl_gob));
    
    sphobj( it->obj=genobj() );
    /*As of now, we've defined the object. Aren't libraries wonderfull?*/

    it->color_mode = LMC_AD;
    my_sphere = it;
    METHOD_OUT
    return((P_Void_ptr)it);

#endif
  }
  
  METHOD_OUT
  return((P_Void_ptr)0);
}

void mycircle(float radius, int up) {
    /*Necessary for def_cylinder to work
      properly: draws a circle of radius radius
      in the xy plane at the origin.*/
    
#define NSIDES 20.0
    
    register float lupe, inc;
    float ver[3], nor[3];

    ver[2] = 0;
    inc = ((2.0*PI)/NSIDES);

    nor[0] = nor[1] = 0;
    if (up)
	nor[2] = 1;
    else
	nor[2] = -1;
    
    bgntmesh();
    n3f(nor);
    for (lupe = 0.0; lupe <= PI; lupe += inc) {
	ver[0] = fsin(lupe)*radius;
	ver[1] = fcos(lupe)*radius;
	v3f(ver);
	ver[0] = fsin(-lupe)*radius;
	ver[1] = fcos(-lupe)*radius;
	v3f(ver);
    }
    
    endtmesh();
#undef NSIDES
}

static P_Void_ptr def_cylinder(char *name) {
	  
P_Renderer *self= (P_Renderer *)po_this;
P_Void_ptr result;
gl_gob *it;
METHOD_IN

  if (RENDATA(self)->open) {
      double surfknotsx[CYL_NUMKNOTSX] = {
	  -1., -1., 1., 1. 
	  };
      
      double surfknotsy[CYL_NUMKNOTSY] = {
	  -1., -1., -1., -1., -1., -1., 1., 1., 1., 1., 1., 1.
	  };

#define CON1 1.775
#define CON2 2.2
      
      double ctlpoints[CYL_NUMPOINTSY][CYL_NUMPOINTSX * CYL_NUMCOORDS] = {
	  -1.0,  0.0,  0.0,
	  -1.0,  0.0,  1.0,
	  
	  -1.0,  CON1,  0.0,
	  -1.0,  CON1,  1.0,
	  
	  CON2,  CON1,  0.0,
	  CON2,  CON1,  1.0,
	  
	  CON2, -CON1,  0.0,
	  CON2, -CON1,  1.0,
	  
	  -1.0, -CON1,  0.0,
	  -1.0, -CON1,  1.0,
	  
	  -1.0,  0.0,  0.0,
	  -1.0,  0.0,  1.0};

      ger_debug("gl_ren_mthd: def_cylinder");

#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_cylinder))();
    METHOD_OUT
    return( (P_Void_ptr)result );
#else
      /*DON'T redefine things!*/
      if (my_cylinder) {
	  METHOD_OUT
	  return((P_Void_ptr)my_cylinder);
      }
      
      if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	  ger_fatal("def_cylinder: unable to allocate %d bytes!", sizeof(gl_gob));
      
      makeobj( it->obj=genobj() );
      /*As of now, we're defining the object*/
      
      it->color_mode = LMC_AD;
      
      translate(0, 0, 1);
      mycircle(1.0, TRUE);
      translate(0, 0, -1);
      mycircle(1.0, FALSE);

      bgnsurface();
      nurbssurface( 
		   sizeof( surfknotsx) / sizeof( double ), surfknotsx, 
		   sizeof( surfknotsy) / sizeof( double ), surfknotsy, 
		   sizeof(double) * CYL_NUMCOORDS, 
		   sizeof(double) * CYL_NUMPOINTSX * CYL_NUMCOORDS, 
		   (double *)ctlpoints, 
		   CYL_ORDERX, CYL_ORDERY, 
		   N_V3D
		   );
      endsurface();

      closeobj();

      METHOD_OUT
      my_cylinder = it;
      return( (P_Void_ptr)it );
#endif
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static P_Void_ptr def_torus(char *name, double major, double minor) {
#define rad .7071067811865 /*  square root of 2 all divided by 2  */
    
  typedef struct  {
      double x, y, z, w;
  }   Vector4d;

  P_Renderer *self= (P_Renderer *)po_this;
  gl_gob *it;
  P_Void_ptr result;
  METHOD_IN
  
  double knots[12] = {0.,0.,0.,1.,1.,2.,2.,3.,3.,4.,4.,4.};
  double bx[9] = {1., rad, 0.,-rad, -1., -rad, 0., rad, 1.};
  double by[9] = {0., rad, 1.,rad, 0., -rad, -1., -rad, 0.};
  double  w[9] = {1., rad, 1., rad,  1.,  rad, 1., rad, 1.};
  Vector4d cpts[81];    /* control points*/
  int i, j, size_s;
  
  if (RENDATA(self)->open) {
      ger_debug("gl_ren_mthd: def_torus");
#ifdef AVOID_NURBS
      METHOD_RDY(ASSIST(self));
      result= (*(ASSIST(self)->def_torus))(major,minor);
      
      METHOD_OUT
      return( (P_Void_ptr)result );
#else
      if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	  ger_fatal("def_cylinder: unable to allocate %d bytes!", sizeof(gl_gob));
      
      makeobj( it->obj=genobj() );
      /*As of now, we're defining the object*/
      
      it->color_mode = LMC_AD;

      size_s = 9;    
      
      for(j=0;j<size_s;j++)  {
	  for(i=0;i<size_s;i++)  {
	      cpts[i+j*size_s].x  =  (minor * bx[j] + major * w[j]) * bx[i];
	      cpts[i+j*size_s].y  =  (minor * bx[j] + major * w[j]) * by[i];
	      cpts[i+j*size_s].z  =   minor * w[i] * by[j];
	      cpts[i+j*size_s].w  =   w[i] * w[j];
	  }
      }
      
      bgnsurface();
      nurbssurface(12,knots,12,knots,
		   sizeof(Vector4d), sizeof(Vector4d) * size_s,
		   (double *)cpts, 3, 3, N_V3DR);
      endsurface();      

      closeobj();

      METHOD_OUT
      return((P_Void_ptr)it);
#endif
  }
  METHOD_OUT
  return((P_Void_ptr)0);
}

static void ren_gob(P_Void_ptr primdata, P_Transform *thistrans,
		P_Attrib_List *thisattrlist) {
    
    /*This routine exists to clear the screen and zbuffer before rendering,
      and to swapbuffers and gflush afterwords.*/

    /*It's output just gets handed straight to ren_object.*/
    
    P_Renderer *self= (P_Renderer *)po_this;
    METHOD_IN
	
    ger_debug("gl_ren_mthd: ren_gob\n");
    
    if (RENDATA(self)->open) {
	winset(WINDOW(self));
	if (primdata) {
	    P_Gob *thisgob;
	    int error=0,color_mode=1;
	    int top_level_call=0;
	    P_Gob_List *kidlist;
	    float *back;

	    thisgob= (P_Gob *)primdata;

	    pushattributes();

	    if (thisattrlist) {
		METHOD_RDY(ASSIST(self));
		(*(ASSIST(self)->push_attributes))( thisattrlist );
	    }

	    if (thisgob->attr) {
		METHOD_RDY(ASSIST(self));
		(*(ASSIST(self)->push_attributes))( thisgob->attr );
	    }
	    
	    /* If thistrans is non-null, this is a top-level call to
	     * ren_gob, as opposed to a recursive call.
	     */
	    if (thistrans) {
		top_level_call = 1;
		lmcolor(LMC_COLOR);
		back = BACKGROUND(self);
		GLPROF("Clearing");
		czclear((back[0]*256)+((back[1]*255)*256)+((back[2]*255)*65536), gd_min);
		pushmatrix();
		lookat(LOOKFROM(self).x, LOOKFROM(self).y, LOOKFROM(self).z,
		       LOOKAT(self).x, LOOKAT(self).y, LOOKAT(self).z,
		       LOOKANGLE(self));
	    }
	    GLPROF(thisgob->name);
	    if ( thisgob->has_transform )
		ren_transform(thisgob->trans);
	    
	    /* There must be children to render, because if this was a primitive
	     * this method would not be getting executed.
	     */
	    kidlist= thisgob->children;
	    while (kidlist) {
		pushattributes();
		pushmatrix();
		METHOD_RDY(kidlist->gob);
		(*(kidlist->gob->render_to_ren))(self, (P_Transform *)0,
						 (P_Attrib_List *)0);
		kidlist= kidlist->next;
		popmatrix();
		popattributes();
	    }

	    if (thisgob->attr) {
		METHOD_RDY(ASSIST(self));
		(*(ASSIST(self)->pop_attributes))( thisgob->attr );
	    }

	    if (thisattrlist) {
		METHOD_RDY(ASSIST(self));
		(*(ASSIST(self)->pop_attributes))( thisattrlist );
	    }
	    
	    if (top_level_call) {
		popmatrix();
		swapbuffers();		
		gflush();
	    }

	    popattributes();
	    
	}
    }
    METHOD_OUT
}

static void traverse_gob( P_Void_ptr primdata, P_Transform *thistrans,
                    P_Attrib_List *thisattrlist )
/* This routine traverses a gob looking for lights. */
{
    P_Renderer *self= (P_Renderer *)po_this;
    P_Gob *thisgob = (P_Gob *)primdata;
    register short lupe;
    METHOD_IN
	
	if (RENDATA(self)->open) {
	    winset(WINDOW(self));
	    if (primdata) {

		ger_debug("gl_ren_mthd: traverse_gob\n");
		
		/*First, check to see if this lighting gob has already been
		  defined...*/
		/*If we're using the same lights as before don't bother to
		  re-render them.*/
		
		if (thisgob == DLIGHTBUFFER(self)) {
		    METHOD_OUT
		    return;
		}
		/*Okay, *now* we need to render them. :)*/
		DLIGHTBUFFER(self) = thisgob;
		
		/*First, clear all lights.*/
		for (lupe = LIGHT0; lupe <= DLIGHTCOUNT(self); lupe++)
		    lmbind(lupe, 0);
		DLIGHTCOUNT(self) = LIGHT0;

		/*Clear the ambient lighting...*/
		AMBIENTCOLOR(self)[0] = 0;
		AMBIENTCOLOR(self)[1] = 0;
		AMBIENTCOLOR(self)[2] = 0;

		/*And render it... ren methods for lights and ambients
		  will fill in everything*/
		pushmatrix();

		if ( thisgob->has_transform )
		    ren_transform(thisgob->trans);

		ren_gob((P_Void_ptr)thisgob, (P_Transform *)0, (P_Attrib_List *)0);
		popmatrix();
		
		/*Now the ambient lighting should be set...*/
		LM(self)[1] = AMBIENTCOLOR(self)[0];
		LM(self)[2] = AMBIENTCOLOR(self)[1];
		LM(self)[3] = AMBIENTCOLOR(self)[2];
		
		lmdef(DEFLMODEL, 3, 4, LM(self));
		lmbind(LMODEL, 3);
		
		/*That ought to do it... on to pillage!*/
	    }
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
    ger_debug("gl_ren_mthd: destroy_gob: destroying gob <%s>",gob->name);
  }
  METHOD_OUT
}

static void get_rgb_color(P_Renderer *self, float col[4], float val) {
    /*Return the RGB value from the colormap.*/

    /* Scale, clip, and map. */
    val= ( val-MAP_MIN(self) )/( MAP_MAX(self)-MAP_MIN(self) );
    if ( val > 1.0 ) val= 1.0;
    if ( val < 0.0 ) val= 0.0;
    (*MAP_FUN(self))( &val, &col[0], &col[1], &col[2], &col[3]);
}

static void def_polything(P_Renderer *self, P_Vlist *vertices) {
    int lupe;
    float vtx[3];
    float col4[4], nor[3];
    color_mode_type color_mode;

    ger_debug("gl_ren_mthd: def_polything\n");

    METHOD_RDY(vertices);

    switch(vertices->type) {
    case P3D_CVTX:
	/*Coordinate vertex list: feed and draw.*/
	for (lupe=0;lupe<vertices->length;lupe++) {
	    vtx[0] = (*vertices->x)(lupe);
	    vtx[1] = (*vertices->y)(lupe);
	    vtx[2] = (*vertices->z)(lupe);
	    v3f(vtx);
	}
	break;
    case P3D_CCVTX:
	/*Coordinate/color vertex list*/
	for (lupe=0;lupe<vertices->length;lupe++) {
	    col4[0] = (*vertices->r)(lupe);
	    col4[1] = (*vertices->g)(lupe);
	    col4[2] = (*vertices->b)(lupe);
	    col4[3] = (*vertices->a)(lupe);
	    c4f(col4);
	    vtx[0] = (*vertices->x)(lupe);
	    vtx[1] = (*vertices->y)(lupe);
	    vtx[2] = (*vertices->z)(lupe);
	    v3f(vtx);
	}
	break;
    case P3D_CCNVTX:
	/*Coordinate/color/normal vertex list*/
	for (lupe=0;lupe<vertices->length;lupe++) {
	    nor[0] = (*vertices->nx)(lupe);
	    nor[1] = (*vertices->ny)(lupe);
	    nor[2] = (*vertices->nz)(lupe);
	    n3f(nor);
	    col4[0] = (*vertices->r)(lupe);
	    col4[1] = (*vertices->g)(lupe);
	    col4[2] = (*vertices->b)(lupe);
	    col4[3] = (*vertices->a)(lupe);
	    c4f(col4);
	    vtx[0] = (*vertices->x)(lupe);
	    vtx[1] = (*vertices->y)(lupe);
	    vtx[2] = (*vertices->z)(lupe);
	    v3f(vtx);
	}
	break;
    case P3D_CNVTX:
	/*Coordinate/normal vertex list*/
	for (lupe=0;lupe<vertices->length;lupe++) {
	    nor[0] = (*vertices->nx)(lupe);
	    nor[1] = (*vertices->ny)(lupe);
	    nor[2] = (*vertices->nz)(lupe);
	    n3f(nor);
	    vtx[0] = (*vertices->x)(lupe);
	    vtx[1] = (*vertices->y)(lupe);
	    vtx[2] = (*vertices->z)(lupe);
	    v3f(vtx);
	}
	break;
    case P3D_CVVTX:
	/*Coordinate/value vertex list*/
	for (lupe=0;lupe<vertices->length;lupe++) {
	    get_rgb_color(self, col4,(*vertices->v)(lupe));
	    c4f(col4);
	    vtx[0] = (*vertices->x)(lupe);
	    vtx[1] = (*vertices->y)(lupe);
	    vtx[2] = (*vertices->z)(lupe);
	    v3f(vtx);
	}
	break;
    case P3D_CVNVTX:
	/*Coordinate/normal/value vertex list*/
	for (lupe=0;lupe<vertices->length;lupe++) {
	    nor[0] = (*vertices->nx)(lupe);
	    nor[1] = (*vertices->ny)(lupe);
	    nor[2] = (*vertices->nz)(lupe);
	    n3f(nor);
	    get_rgb_color(self, col4,(*vertices->v)(lupe));
	    c4f(col4);
	    vtx[0] = (*vertices->x)(lupe);
	    vtx[1] = (*vertices->y)(lupe);
	    vtx[2] = (*vertices->z)(lupe);
	    v3f(vtx);
	}
	break;
    case P3D_CVVVTX:
	/*Coordinate/value/value vertex list*/
	for (lupe=0;lupe<vertices->length;lupe++) {
	    get_rgb_color(self, col4,(*vertices->v)(lupe));
	    c4f(col4);
	    vtx[0] = (*vertices->x)(lupe);
	    vtx[1] = (*vertices->y)(lupe);
	    vtx[2] = (*vertices->z)(lupe);
	    v3f(vtx);
	}
	break;
    default:
	printf("gl_ren_mthd: def_polything: null vertex type.\n");
	break;
    } /*switch(vertices->type)*/
    
}

static P_Void_ptr def_polyline(char *name, P_Vlist *vertices) {
    /*Use def_polything to get the image*/
    
    gl_gob *it;
    P_Renderer *self= (P_Renderer *)po_this;
    METHOD_IN
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return((P_Void_ptr)0);
    }

    ger_debug("gl_ren_mthd: def_polyline\n");
    
    if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	ger_fatal("def_polyline: unable to allocate %d bytes!", sizeof(gl_gob));
    
    makeobj( it->obj=genobj() );
    /*As of now, we're defining the object*/
    
    it->color_mode = LMC_COLOR;
    bgnline();
    def_polything(self, vertices);
    endline();
    closeobj();
    METHOD_OUT
    return((P_Void_ptr)it);
}

static P_Void_ptr def_polygon(char *name, P_Vlist *vertices) {
    /*Use def_polything to get the image*/
    
    gl_gob *it;
    P_Renderer *self= (P_Renderer *)po_this;
    METHOD_IN
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return((P_Void_ptr)0);
    }
    
    ger_debug("gl_ren_mthd: def_polygon\n");

    if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	ger_fatal("def_polygon: unable to allocate %d bytes!", sizeof(gl_gob));
    
    makeobj( it->obj=genobj() );
    /*As of now, we're defining the object*/
    
    it->color_mode = LMC_COLOR;
    bgnpolygon();
    def_polything(self, vertices);
    endpolygon();
    closeobj();
    METHOD_OUT
    return((P_Void_ptr)it);
}

static P_Void_ptr def_polymarker(char *name, P_Vlist *vertices) {
    /*Use def_polything to get the image*/
    
    gl_gob *it;
    P_Renderer *self= (P_Renderer *)po_this;
    METHOD_IN
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return((P_Void_ptr)0);
    }
    
    ger_debug("gl_ren_mthd: def_polymarker\n");

    if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	ger_fatal("def_polymarker: unable to allocate %d bytes!", sizeof(gl_gob));
    
    makeobj( it->obj=genobj() );
    /*As of now, we're defining the object*/
    
    it->color_mode = LMC_COLOR;

    bgnline();
    def_polything(self, vertices);
    endline();
    closeobj();
    METHOD_OUT
    return((P_Void_ptr)it);
}

static P_Void_ptr def_tristrip(char *name, P_Vlist *vertices) {
    /*Use def_polything to get the image*/
    
    gl_gob *it;
    P_Renderer *self= (P_Renderer *)po_this;
    METHOD_IN
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return((P_Void_ptr)0);
    }
    
    ger_debug("gl_ren_mthd: def_tristrip\n");

    if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
	ger_fatal("def_tristrip: unable to allocate %d bytes!", sizeof(gl_gob));
    
    makeobj( it->obj=genobj() );
    /*As of now, we're defining the object*/

    it->color_mode = LMC_COLOR;

    bgntmesh();
    def_polything(self, vertices);
    endtmesh();
    closeobj();
    METHOD_OUT
    return((P_Void_ptr)it);
}

static P_Void_ptr def_bezier(char *name, P_Vlist *vertices) {
  static double bez_knots[8]= {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
  double coords[3*16];
  double colors[4*16];
  double *coordptr, *clrptr;
  float col4[4];
  int i;
  int color_flag= 0;
  int value_flag= 0;
  int value_flag_2= 0;
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  gl_gob *it;
    
  METHOD_IN

  if (! (RENDATA(self)->open)) {
      METHOD_OUT
      return((P_Void_ptr)0);
  }
  
  ger_debug("gl_ren_mthd: def_bezier");

#ifdef AVOID_NURBS
    METHOD_RDY(ASSIST(self));
    result= (*(ASSIST(self)->def_bezier))(vertices);
    METHOD_OUT
    return( (P_Void_ptr)result );
#else

  if (! (it = (gl_gob *)malloc(sizeof(gl_gob))))
      ger_fatal("def_bezier: unable to allocate %d bytes!", sizeof(gl_gob));
  
  switch (vertices->type) {
  case P3D_CVVVTX:
      it->color_mode = LMC_COLOR;
      value_flag_2 = 0;
      value_flag = 1;
      color_flag = 0;
      break;
  case P3D_CVNVTX:
      it->color_mode = LMC_AD;
      value_flag_2 = 0;
      value_flag = 1;
      color_flag = 0;
      break;
  case P3D_CVVTX:
      it->color_mode = LMC_COLOR;
      value_flag_2 = 0;
      value_flag = 1;
      color_flag = 0;
      break;
  case P3D_CCNVTX:
  case P3D_CCVTX:
      it->color_mode = LMC_AD;
      value_flag_2 = 0;
      value_flag = 0;
      color_flag = 1;
      break;
  case P3D_CVTX:
      it->color_mode = LMC_AD;
      value_flag_2 = 0;
      color_flag = 0;
      value_flag = 0;
      break;
  case P3D_CNVTX:
      it->color_mode = LMC_COLOR;
      value_flag_2 = 0;
      color_flag = 0;
      value_flag = 0;
      break;
  }
  
  coordptr= coords;
  clrptr= colors;
  METHOD_RDY(vertices);
  for (i=0; i<16; i++) {
      *coordptr++= (*vertices->x)(i);
      *coordptr++= (*vertices->y)(i);
      *coordptr++= (*vertices->z)(i);
      if (color_flag) {
	  *clrptr++ = (*vertices->r)(i);
	  *clrptr++ = (*vertices->g)(i);
	  *clrptr++ = (*vertices->b)(i);
	  *clrptr++ = (*vertices->a)(i);
      } else if (value_flag) {
	  get_rgb_color(self, col4, (*vertices->v)(i));
	  *clrptr++ = (col4[0]);
	  *clrptr++ = (col4[1]);
	  *clrptr++ = (col4[2]);
	  *clrptr++ = (col4[3]);
      } else if (value_flag_2) {
	  get_rgb_color(self, col4, (*vertices->v2)(i));
	  *clrptr++ = (col4[0]);
	  *clrptr++ = (col4[1]);
	  *clrptr++ = (col4[2]);
	  *clrptr++ = (col4[3]);
      }
  }
  
  makeobj( it->obj=genobj() );
  /*As of now, we're defining the object*/
  
  bgnsurface();
  
  if (color_flag || value_flag || value_flag_2)
      nurbssurface( 8, bez_knots, 8, bez_knots,
		   4*sizeof(double), 4*4*sizeof(double), colors,
		   4, 4, N_C4D );
  
  nurbssurface( 8, bez_knots, 8, bez_knots,
	       3*sizeof(double), 4*3*sizeof(double), coords,
	       4, 4, N_V3D );
  
  endsurface();
  closeobj();
  
  METHOD_OUT
	
  return((P_Void_ptr)it);
#endif
}

static P_Void_ptr def_mesh(char *name, P_Vlist *vertices, int *indices,
			   int *facet_lengths, int nfacets) {
    P_Renderer *self= (P_Renderer *)po_this;
    gl_gob *it;
    int lupe, loope;
    float vtx[3];
    float col4[4], nor[3];
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return((P_Void_ptr)0);
    }

    ger_debug("gl_ren_mthd: def_mesh");
    if (vertices->length < 3) {
	ger_error("gl_ren_mthd: def_mesh: %d isn't enough vertices for a mesh!", vertices->length);
	METHOD_OUT
	return((P_Void_ptr)0);
    }

    if (nfacets<1) {
	ger_error("gl_ren_mthd: def_mesh: %d isn't enough facets for a mesh!", nfacets);
	METHOD_OUT
	return((P_Void_ptr)0);
    }

    if ( !(it= (gl_gob *)malloc(sizeof(gl_gob))))
	ger_fatal("gl_ren_mthd: def_mesh: unable to allocate %d bytes!"
		  , sizeof(gl_gob));
    
    makeobj( it->obj=genobj() );
    /*As of now, we're defining the object*/
    
    switch (vertices->type) {
    case P3D_CNVTX:  /*Fixed for std. gobs*/
    case P3D_CCNVTX:
    case P3D_CVNVTX: /*Fixed*/
    case P3D_CVTX:
	it->color_mode = LMC_AD;
	break;
    case P3D_CCVTX:
    case P3D_CVVTX:  /*Fixed*/
    case P3D_CVVVTX:
	it->color_mode = LMC_COLOR;
	break;
    }

    METHOD_RDY(vertices);
    for (loope=0; loope < nfacets; loope++) {
	if (*facet_lengths == 3)
	    bgntmesh();
	else
	    bgnpolygon();
	switch(vertices->type) {
	case P3D_CVTX:
	    /*Coordinate vertex list: feed and draw.*/
	    for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
		vtx[0] = (*vertices->x)(*indices);
		vtx[1] = (*vertices->y)(*indices);
		vtx[2] = (*vertices->z)(*indices);
		v3f(vtx);
	    }
	    break;
	case P3D_CCVTX:
	    /*Coordinate/color vertex list*/
	    for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
		col4[0] = (*vertices->r)(*indices);
		col4[1] = (*vertices->g)(*indices);
		col4[2] = (*vertices->b)(*indices);
		col4[3] = (*vertices->a)(*indices);
		c4f(col4);
		vtx[0] = (*vertices->x)(*indices);
		vtx[1] = (*vertices->y)(*indices);
		vtx[2] = (*vertices->z)(*indices);
		v3f(vtx);
	    }
	    break;
	case P3D_CCNVTX:
	    /*Coordinate/color/normal vertex list*/
	    for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
		nor[0] = (*vertices->nx)(*indices);
		nor[1] = (*vertices->ny)(*indices);
		nor[2] = (*vertices->nz)(*indices);
		n3f(nor);
		col4[0] = (*vertices->r)(*indices);
		col4[1] = (*vertices->g)(*indices);
		col4[2] = (*vertices->b)(*indices);
		col4[3] = (*vertices->a)(*indices);
		c4f(col4);
		vtx[0] = (*vertices->x)(*indices);
		vtx[1] = (*vertices->y)(*indices);
		vtx[2] = (*vertices->z)(*indices);
		v3f(vtx);
	    }
	    break;
	case P3D_CNVTX:
	    /*Coordinate/normal vertex list*/
	    for (lupe=0;lupe < *facet_lengths; lupe++, indices++) {
		nor[0] = (*vertices->nx)(*indices);
		nor[1] = (*vertices->ny)(*indices);
		nor[2] = (*vertices->nz)(*indices);
		n3f(nor);
		vtx[0] = (*vertices->x)(*indices);
		vtx[1] = (*vertices->y)(*indices);
		vtx[2] = (*vertices->z)(*indices);
		v3f(vtx);
	    }
	    break;
	case P3D_CVVTX:
	    /*Coordinate/value vertex list*/
	    for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
		get_rgb_color(self, col4,(*vertices->v)(*indices));
		c4f(col4);
		vtx[0] = (*vertices->x)(*indices);
		vtx[1] = (*vertices->y)(*indices);
		vtx[2] = (*vertices->z)(*indices);
		v3f(vtx);
	    }
	    break;
	case P3D_CVNVTX:
	    /*Coordinate/normal/value vertex list*/
	    for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
		nor[0] = (*vertices->nx)(*indices);
		nor[1] = (*vertices->ny)(*indices);
		nor[2] = (*vertices->nz)(*indices);
		n3f(nor);
		get_rgb_color(self, col4,(*vertices->v)(*indices));
		c4f(col4);
		vtx[0] = (*vertices->x)(*indices);
		vtx[1] = (*vertices->y)(*indices);
		vtx[2] = (*vertices->z)(*indices);
		v3f(vtx);
	    }
	    break;
	case P3D_CVVVTX:
	    /*Coordinate/value/value vertex list*/
	    for (lupe=0;lupe < *facet_lengths;lupe++, indices++) {
		get_rgb_color(self, col4,(*vertices->v)(*indices));
		c4f(col4);
		vtx[0] = (*vertices->x)(*indices);
		vtx[1] = (*vertices->y)(*indices);
		vtx[2] = (*vertices->z)(*indices);
		v3f(vtx);
	    }
	    break;
	default:
	    printf("gl_ren_mthd: def_mesh: null vertex type.\n");
	    break;
	} /*switch(vertices->type)*/
	
	if (*facet_lengths == 3)
	    endtmesh();
	else
	    endpolygon();
	facet_lengths++;
    }
    
    closeobj();    
    
    METHOD_OUT
    return( (P_Void_ptr)it );
}

static P_Void_ptr def_light(char *name, P_Point *point, P_Color *color) {
    
    P_Renderer *self = (P_Renderer *)po_this;

    float the_light[] = {
	LCOLOR, 1, 1, 1,
	POSITION, 1, 1, 1, 0,
	LMNULL };
    
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
    
    ger_debug("gl_ren_mthd: def_light\n");
    
    the_light[1] = color->r;
    the_light[2] = color->g;
    the_light[3] = color->b;

    the_light[5] = point->x;
    the_light[6] = point->y;
    the_light[7] = point->z;

    /*WARNING: DOING IT THIS WAY LIMITS ONE TO USING ONLY A SHORT OF LIGHTS
      IF THIS MESSES UP IT'LL NEED TO BE REWRITTEN.*/

    lmdef(DEFLIGHT, light_no, 0, the_light);

    METHOD_OUT
    return ((P_Void_ptr)light_no++);
}

void ren_light(P_Void_ptr it, P_Transform *foo,
			      P_Transform *bar) {

    P_Renderer *self = (P_Renderer *)po_this;
    short the_light = (short)it;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }

    ger_debug("gl_ren_mthd: ren_light");
    
    /*Okay... let's figure out how many lights are in use...
     and quit if we have too many...*/
    if ((LIGHT0 + MAXLIGHTS) == DLIGHTCOUNT(self))
	return;

    lmbind(DLIGHTCOUNT(self)++, the_light);
    
    METHOD_OUT
}

void destroy_light(P_Void_ptr it) {
    
    P_Renderer *self = (P_Renderer *)po_this;
    short the_light = (short)it;
    METHOD_IN
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
	
    ger_debug("gl_ren_mthd: destroy_light\n");

    /*Hmm... Maybe if we do resource allocation one of these days there'll
      actually be something to do here...*/
    
    METHOD_OUT
}

static P_Void_ptr def_ambient(char *name, P_Color *color) {
    
    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return((P_Void_ptr)0);
    }
	
    ger_debug("gl_ren_mthd: def_ambient\n");
    
    /*Let's just put this somewhere we can use it later.*/

    METHOD_OUT
    return ((P_Void_ptr)color);
}

void ren_ambient(P_Void_ptr it, P_Transform *foo,
			      P_Transform *bar) {

    P_Color *color = (P_Color *)it;
    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }

    /*Ambient lighting sums... at least, the painter renderer seems
      to think it does... so let's just add in the ambient lighting
      here...*/
    
    rgbify_color(color);
    
    AMBIENTCOLOR(self)[0] += color->r;
    AMBIENTCOLOR(self)[1] += color->g;
    AMBIENTCOLOR(self)[2] += color->b;
        
    ger_debug("gl_ren_mthd: ren_ambient");
    
    METHOD_OUT
}

void destroy_ambient(P_Void_ptr it) {
    
    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
	
    ger_debug("gl_ren_mthd: destroy_ambient\n");

    METHOD_OUT
}

static void hold_gob(P_Void_ptr *gob) {
    /*This doesn't do anything*/

    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }

    
    ger_debug("gl_ren_mthd: hold_gob\n");

    METHOD_OUT
}

static void unhold_gob(P_Void_ptr *gob) {
    /*This doesn't do anything*/

    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
	
    ger_debug("gl_ren_mthd: unhold_gob\n");
    METHOD_OUT
}

static P_Void_ptr def_camera(struct P_Camera_struct *thecamera) {
    
    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return((P_Void_ptr)0);
    }
	

    ger_debug("gl_ren_mthd: def_camera\n");
    /*I think all I want to do for this method is to
      stash the camera, and deal with it in the set_camera
      routine.*/
    
    METHOD_OUT
    return ((P_Void_ptr)thecamera);
}

static void set_camera(P_Void_ptr thecamera) {

    P_Renderer *self = (P_Renderer *)po_this;
    P_Camera *camera = (P_Camera *)thecamera;
    P_Vector looking, theCross;
    float norm;
    float result;
    short short_result;

    METHOD_IN	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
	

    ger_debug("gl_ren_mthd: set_camera\n");

    /*Well, it was stashed, and now we get to use it.
      Happy Happy Joy Joy.*/
    
    winset(WINDOW(self));

    perspective(10*(camera->fovea), ASPECT(self), (0-(camera->hither)), (0-(camera->yon)));

    LOOKFROM(self).x = camera->lookfrom.x;
    LOOKFROM(self).y = camera->lookfrom.y;
    LOOKFROM(self).z = camera->lookfrom.z;
    LOOKAT(self).x = camera->lookat.x;
    LOOKAT(self).y = camera->lookat.y;
    LOOKAT(self).z = camera->lookat.z;

    theCross.x = ((looking.y = (camera->lookat.y - camera->lookfrom.y))*camera->up.z) - ((looking.z = (camera->lookat.z - camera->lookfrom.z))*camera->up.y);
    theCross.y = (looking.z*camera->up.x) - ((looking.x = (camera->lookat.x - camera->lookfrom.x))*camera->up.z);
    theCross.z = (looking.x*camera->up.y) - (looking.y*camera->up.x);
    
    norm = sqrtf((theCross.x*theCross.x) + (theCross.y*theCross.y) + (theCross.z*theCross.z));
    
    if (norm) {
	if (theCross.x <= 0)
	    result = 1800-10.0*RadtoDeg*asinf(theCross.y/norm);
	else
	    result = 10.0*RadtoDeg*asinf(theCross.y/norm);	
	short_result = result;
	LOOKANGLE(self) = short_result;
	
	/*Don't ask me how I came up with the angle.*/
	/*It's quite messy.*/
    }
    else
	ger_error("gl_ren_mthd: set_camera: up vector is parallel to \
                   viewing vector; setting up angle to %d.", LOOKANGLE(self));
    BACKGROUND(self)[0] = camera->background.r;
    BACKGROUND(self)[1] = camera->background.g;
    BACKGROUND(self)[2] = camera->background.b;

    METHOD_OUT
}    
    
static void destroy_camera(P_Void_ptr *thecamera) {

    P_Renderer *self = (P_Renderer *)po_this;
    METHOD_IN
	
    if (! (RENDATA(self)->open)) {
	METHOD_OUT
	return;
    }
	
    ger_debug("gl_ren_mthd: destroy_camera\n");

    /*I don't have anything to deallocate... :)*/
    
    METHOD_OUT
}    
    
static P_Void_ptr def_text(char *name, char *tstring, P_Point *location,
		    P_Vector *u, P_Vector *v) {
  P_Renderer *self= (P_Renderer *)po_this;
  P_Void_ptr result;
  METHOD_IN

  if (RENDATA(self)->open) {
    ger_debug("gl_ren_mthd: def_text");
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
    ger_debug("gl_ren_mthd: ren_text");
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
    ger_debug("gl_ren_mthd: destroy_text");

    METHOD_RDY(ASSIST(self));
    (*(ASSIST(self)->destroy_text))(rendata);

  }
  METHOD_OUT
}

static void ren_print() {
    ger_debug("gl_ren_mthd: ren_print");
}

static void ren_open() {
    /*opening the renderer.*/
    
    P_Renderer *self=(P_Renderer *)po_this;
    
    METHOD_IN

    ger_debug("gl_ren_mthd: ren_open\n");
    RENDATA(self)->open=1;
    
    METHOD_OUT
}

static void ren_close() {
    P_Renderer *self= (P_Renderer *)po_this;
    METHOD_IN
    ger_debug("gl_ren_mthd: ren_close");
    RENDATA(self)->open= 0;
    METHOD_OUT
}

static void ren_destroy() {
    /*Kill everything. Everything.*/
    P_Renderer *self=(P_Renderer *)po_this;
    METHOD_IN

    winclose(WINDOW(self));

    ger_debug("gl_ren_mthd: ren_destroy\n");

    free ((void *)self);
    METHOD_DESTROYED
}

static P_Void_ptr def_cmap(char *name, double min, double max, void(*mapfun)(float *, float *, float *, float *, float *)) {
  P_Renderer *self= (P_Renderer *)po_this;
  P_Renderer_Cmap *thismap;
  METHOD_IN
  
  if (! (RENDATA(self)->open)) {
      METHOD_OUT
      return((P_Void_ptr)0);
  }

  ger_debug("gl_ren_mthd: def_cmap");

  if (max == min) {
    ger_error("gl_ren_mthd: def_cmap: max equals min (val is %f)", max );
    METHOD_OUT;
    return( (P_Void_ptr)0 );
  }

  if ( RENDATA(self)->open ) {
    if ( !(thismap= (P_Renderer_Cmap *)malloc(sizeof(P_Renderer_Cmap))) )
      ger_fatal( "gl_ren_mthd: def_cmap: unable to allocate %d bytes!",
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

static void install_cmap(P_Void_ptr mapdata) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gl_ren_mthd: install_cmap");
  if ( RENDATA(self)->open ) {
    if (mapdata) CUR_MAP(self)= (P_Renderer_Cmap *)mapdata;
    else ger_error("gl_ren_mthd: install_cmap: got null color map data.");
  }
  METHOD_OUT
}

static void destroy_cmap(P_Void_ptr mapdata) {
  P_Renderer *self= (P_Renderer *)po_this;
  METHOD_IN

  ger_debug("gl_ren_mthd: destroy_cmap");
  if ( RENDATA(self)->open ) free( mapdata );
  METHOD_OUT
}

static P_Void_ptr def_gob(char *name, struct P_Gob_struct *gob) {
    
    P_Renderer *self = (P_Renderer *)po_this;

    METHOD_IN
	
    if (RENDATA(self)->open) {
	ger_debug("gl_ren_mthd: def_gob");
	METHOD_OUT
	return( (P_Void_ptr)gob ); /* Stash the gob where we can get it later */
    }
    METHOD_OUT
    return((P_Void_ptr)0);
}


P_Renderer *po_create_gl_renderer( char *device, char *datastr )
/* This routine creates a ptr-generating renderer object */
{
  P_Renderer *self;
  int length,i;
  P_Renderer_data *rdata;
  long xsize, ysize;
  register short lupe;
  register char *where;
  int x,y,result; /*For prefposition*/
  unsigned int width, height; /*For prefsize*/
  char *name, *size;
  
  ger_debug("po_create_gl_renderer: device= <%s>, datastr= <%s>",
	    device, datastr);

  /* Create memory for the renderer */
  if ( !(self= (P_Renderer *)malloc(sizeof(P_Renderer))) )
    ger_fatal("po_create_gl_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer) );

  /* Create memory for object data */
  if ( !(rdata= (P_Renderer_data *)malloc(sizeof(P_Renderer_data))) )
    ger_fatal("po_create_gl_renderer: unable to allocate %d bytes!",
              sizeof(P_Renderer_data) );
  self->object_data= (P_Void_ptr)rdata;
  
  /*parse the datastr string*/
  /*At present, because we only take "name" and "size" arguments,
    we're just looking forward for a "n" or a "s", searching forward for the 
    ", getting the data up to the next ", and repeating.
    We'll also support "p" for "position" and "g" for "geometry"
   in place of "s", and "t" for "title" in place of "n".
   */

  name = "p3d-gl";
  size = NULL;
  
  if (datastr) {
      where = (char *)malloc(strlen(datastr)+1);
      strcpy(where, datastr);
      for (;*where != '\0';where++)
	  switch (*where) {
	  case 's':
	  case 'S':
	  case 'g':
	  case 'G':
	  case 'p':
	  case 'P':
	      for (;(*where != '"') && (*where != '\0');where++);
	      if (*where != '\0') {
		  size = ++where;
		  for (;(*where != '"') && (*where != '\0');where++);
		  if (*where == '"')
		      *where='\0';
		  else
		      where--;
	      }
	      break;
	  case 'n':
	  case 'N':
	  case 't':
	  case 'T':
	      for (;(*where != '"') && (*where != '\0');where++);
	      if (*where != '\0') {
		  name = ++where;
		  for (;(*where != '"') && (*where != '\0');where++);
		  if (*where == '"')
		      *where='\0';
		  else
		      where--;
	      }
	      break;
	  default:
	      break;
	  }
  }
  
  NAME(self) = name;
  /*Define window, set up zbuffer, set paging, etc etc etc...*/
  
  /*start by parsing the size into a geometry spec...*/
  if (size)
      result = XParseGeometry(size, &x, &y, &width, &height);
  else
      result = 0;
  
  if (result & (XValue | YValue)) {
      if (result & XNegative)
	  x = getgdesc(GD_XPMAX) - x;
      if (! (result & YNegative))
	  y = getgdesc(GD_YPMAX) - y;
      prefposition(x, x+width, y, y+height);
      };

  if (result & (WidthValue | HeightValue))
      prefsize(width, height);
  
  foreground();
  WINDOW(self) = winopen(NAME(self));
  winconstraints();
  mmode(MVIEWING);
  doublebuffer();
  RGBmode();
  subpixel(TRUE);
  gconfig();
  cpack(0);
  clear();
  swapbuffers();
  zbuffer(TRUE);
  lmcolor(LMC_COLOR);
  glcompat(GLC_ZRANGEMAP, 0);
  glcompat(GLC_OLDPOLYGON, 1);
  lsetdepth(getgdesc(GD_ZMAX), getgdesc(GD_ZMIN));
  zfunction(ZF_GREATER);
  shademodel(GOURAUD);
  gd_min = getgdesc(GD_ZMIN);
  if (hastransparent = getgdesc(GD_BLEND))
      blendfunction(BF_SA, BF_MSA);
  else
      defpattern(GREYPATTERN, 16, greyPattern);	  

  sphmode(SPH_TESS, SPH_BILIN);
  sphmode(SPH_DEPTH, 4);
  
  /*Predefine all of our materials...*/
  
  for (lupe=0; lupe < N_MATERIALS; lupe++)
      lmdef(DEFMATERIAL, lupe+1, 5, materials[lupe]);

  getsize(&xsize, &ysize);
  ASPECT(self)= (float)xsize / (float)ysize;
  
  BACKGROUND(self) = (float *)(malloc(3*sizeof(float)));
  AMBIENTCOLOR(self) = (float *)(malloc(3*sizeof(float)));
  LM(self) = (float *)(malloc(9*sizeof(float)));

  for (lupe = 0; lupe < 9; lupe++)
      LM(self)[lupe] = initiallm[lupe];
    
  ASSIST(self)= po_create_assist(self);

  BACKCULLSYMBOL(self)= create_symbol("backcull");
  TEXTHEIGHTSYMBOL(self)= create_symbol("text-height");
  COLORSYMBOL(self)= create_symbol("color");
  MATERIALSYMBOL(self)= create_symbol("material");
  CURMATERIAL(self) = 0;
  MAXDLIGHTCOUNT(self) = MAXLIGHTS;
  DLIGHTCOUNT(self) = LIGHT0;
  DLIGHTBUFFER(self) = NULL;

  /* Fill in all the methods */
  self->def_sphere= def_sphere;
  self->ren_sphere= ren_object;
  self->destroy_sphere= destroy_object;

  self->def_cylinder= def_cylinder;
  self->ren_cylinder= ren_object;
  self->destroy_cylinder= destroy_object;

  self->def_torus= def_torus;
  self->ren_torus= ren_object;
  self->destroy_torus= destroy_object;

  self->def_polymarker= def_polymarker;
  self->ren_polymarker= ren_object;
  self->destroy_polymarker= destroy_object;

  self->def_polyline= def_polyline;
  self->ren_polyline= ren_object;
  self->destroy_polyline= destroy_object;

  self->def_polygon= def_polygon;
  self->ren_polygon= ren_object;
  self->destroy_polygon= destroy_object;

  self->def_tristrip= def_tristrip;
  self->ren_tristrip= ren_object;
  self->destroy_tristrip= destroy_object;

  self->def_bezier= def_bezier;
  self->ren_bezier= ren_object;
  self->destroy_bezier= destroy_object;

  self->def_mesh= def_mesh;
  self->ren_mesh= ren_object;
  self->destroy_mesh= destroy_object;

  self->def_text= def_text;
  self->ren_text= ren_text;
  self->destroy_text= destroy_text;

  self->def_light= def_light;
  self->ren_light= ren_light;
  self->light_traverse_light= ren_light;
  self->destroy_light= destroy_object;

  self->def_ambient= def_ambient;
  self->ren_ambient= ren_ambient;
  self->light_traverse_ambient= ren_ambient;
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
