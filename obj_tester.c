/****************************************************************************
 * obj_tester.c
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

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"

#define ERRCHK( string ) if (!(string)) ger_error("ERROR!")

static int test_flag= 1;
static int test_int= 2;
static float test_float= 3.0;
static P_Color test_color= { P3D_RGB, 0.2, 0.3, 0.8, 0.9 };
static P_Color red_color= { P3D_RGB, 1.0, 0.0, 0.0, 1.0 };
static P_Color green_color= { P3D_RGB, 0.0, 1.0, 0.0, 1.0 };
static P_Color white_color= { P3D_RGB, 1.0, 1.0, 1.0, 1.0 };
static P_Point test_point= { 0.3, 0.4, 0.5 };
static P_Vector test_vector= { 1.414, 0.0, 1.414 };
static char test_string[]= "This is my test string";

static P_Transform test_trans= {{
  1.0, 0.0, 0.0, 0.0,
  0.0, 2.0, 0.0, 0.0,
  0.0, 0.0, 3.0, 3.0,
  0.0, 0.0, 0.0, 1.0
  }};

static P_Point text_loc= { 0.0, 0.0, 3.0 };
static P_Vector text_u= { 1.0, 0.0, 0.0 };
static P_Vector text_v= { 0.0, 0.0, 1.0 };

static P_Point light_loc= {0.0, 1.0, 10.0};
static P_Color light_color= { P3D_RGB, 0.8, 0.8, 0.8, 0.3 };

static P_Color ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 0.8 };

#define MESH_LENGTH 9
#define MESH_FACETS 3
static int meshind[MESH_LENGTH]={
  0, 2, 1,
  1, 2, 3,
  0, 3, 2
  };
static int meshlengths[MESH_FACETS]= { 3, 3, 3 };
static int meshfacets= MESH_FACETS;

static float bez_vtx_coords[3*16]= {
  0.0, 0.0, 0.0, 
  1.0, 0.0, 1.0, 
  2.0, 0.0, 1.0, 
  3.0, 0.0, 0.0,
  0.0, 1.0, 1.0, 
  1.0, 1.0, 2.0, 
  2.0, 1.0, 2.0, 
  3.0, 1.0, 1.0,
  0.0, 2.0, 1.0, 
  1.0, 2.0, 2.0, 
  2.0, 2.0, 2.0, 
  3.0, 2.0, 1.0,
  0.0, 3.0, 0.0, 
  1.0, 3.0, 1.0, 
  2.0, 3.0, 1.0, 
  3.0, 3.0, 0.0
  };

#define CVTX_LENGTH 4
static float cvtx_coords[3*CVTX_LENGTH]={ 
  0.0, 0.0, 0.0, 
  2.0, 0.0, 0.0, 
  2.0, 2.0, 1.0, 
  0.0, 2.0, 1.0
  };

#define FLIST_LENGTH 4
static float f_x[FLIST_LENGTH]= { 0.0, 2.0, 0.0, 0.0 };
static float f_y[FLIST_LENGTH]= { 0.0, 0.0, 2.0, 0.0 };
static float f_z[FLIST_LENGTH]= { 0.0, 0.0, 0.0, 2.0 };
static float f_r[FLIST_LENGTH]= { 1.0, 1.0, 0.0, 0.0 };
static float f_g[FLIST_LENGTH]= { 1.0, 0.0, 1.0, 0.0 };
static float f_b[FLIST_LENGTH]= { 1.0, 0.0, 0.0, 1.0 };
static float f_a[FLIST_LENGTH]= { 0.2, 0.3, 0.4, 0.5 };
static float f_nx[FLIST_LENGTH]= { 0.0, 1.0, 0.0, 0.0 };
static float f_ny[FLIST_LENGTH]= { 0.0, 0.0, 1.0, 0.0 };
static float f_nz[FLIST_LENGTH]= { 0.0, 0.0, 0.0, 1.0 };
static float f_v[FLIST_LENGTH]= { 5.3, 6.4, 7.5, 8.6 };

static void f_tests( VOIDLIST )
{
  P_Vlist *cvtx_vlist, *ccvtx_vlist, *cnvtx_vlist, *ccnvtx_vlist;
  P_Vlist *cvvtx_vlist, *cvnvtx_vlist;
  
  cvtx_vlist= po_create_fvlist( P3D_CVTX, FLIST_LENGTH, f_x, f_y, f_z,
                               (float *)0, (float *)0, (float *)0,
                               (float *)0, (float *)0, (float *)0, (float *)0);

  ccvtx_vlist= po_create_fvlist( P3D_CCVTX, FLIST_LENGTH, f_x, f_y, f_z,
                               (float *)0, (float *)0, (float *)0,
                               f_r, f_g, f_b, f_a );

  cnvtx_vlist= po_create_fvlist( P3D_CNVTX, FLIST_LENGTH, f_x, f_y, f_z,
                               f_nx, f_ny, f_nz,
                               (float *)0, (float *)0, (float *)0, (float *)0);

  ccnvtx_vlist= po_create_fvlist( P3D_CCNVTX, FLIST_LENGTH, f_x, f_y, f_z,
                               f_nx, f_ny, f_nz,
                               f_r, f_g, f_b, f_a );

  cvvtx_vlist= po_create_fvlist( P3D_CVVTX, FLIST_LENGTH, f_x, f_y, f_z,
                               (float *)0, (float *)0, (float *)0,
                               f_v, (float *)0, (float *)0, (float *)0);

  cvnvtx_vlist= po_create_fvlist( P3D_CVNVTX, FLIST_LENGTH, f_x, f_y, f_z,
                               f_nx, f_ny, f_nz,
                               f_v, (float *)0, (float *)0, (float *)0);
  METHOD_RDY( cvtx_vlist )
  (*(cvtx_vlist->print))();

  METHOD_RDY( ccvtx_vlist )
  (*(ccvtx_vlist->print))();

  METHOD_RDY( cnvtx_vlist )
  (*(cnvtx_vlist->print))();

  METHOD_RDY( ccnvtx_vlist )
  (*(ccnvtx_vlist->print))();

  METHOD_RDY( cvvtx_vlist )
  (*(cvvtx_vlist->print))();

  METHOD_RDY( cvnvtx_vlist )
  (*(cvnvtx_vlist->print))();

  METHOD_RDY( cvtx_vlist )
  (*(cvtx_vlist->destroy_self))();

  METHOD_RDY( ccvtx_vlist )
  (*(ccvtx_vlist->destroy_self))();

  METHOD_RDY( cnvtx_vlist )
  (*(cvvtx_vlist->destroy_self))();

  METHOD_RDY( ccnvtx_vlist )
  (*(ccnvtx_vlist->destroy_self))();

  METHOD_RDY( cvvtx_vlist )
  (*(cvvtx_vlist->destroy_self))();

  METHOD_RDY( cvnvtx_vlist )
  (*(cvnvtx_vlist->destroy_self))();
}

static void my_map_fun( float *val, float *r, float *g, float *b, float *a )
/* This routine gets installed as a color map. */
{
  *r= *g= *b= *val;
  *a= 1.0;
}

static void object_tests( VOIDLIST )
/* This routine tests all the objects except hash tables. */
{
  P_Vlist *cvtx_vlist, *bez_vtx_vlist;
  P_Gob *gob, *sphere1, *cylinder1, *torus1, *text1, *pmark1, *pline1;
  P_Gob *pgon1, *tri1, *bezier1, *mesh1;
  P_Gob *light1, *ambient1, *lightgob;
  
  cvtx_vlist= po_create_cvlist( P3D_CVTX, CVTX_LENGTH, cvtx_coords );
  bez_vtx_vlist= po_create_cvlist( P3D_CVTX, 16, bez_vtx_coords );

  gob= po_create_gob("");
  lightgob= po_create_gob("");
  sphere1= po_create_sphere("sph1");
  cylinder1= po_create_cylinder("cyl1");
  torus1= po_create_torus("tor1",3.0, 0.8);
  text1= po_create_text("txt1",test_string,&text_loc,&text_u,&text_v);
  pmark1= po_create_polymarker("pmk1",cvtx_vlist);
  pline1= po_create_polyline("pln1",cvtx_vlist);
  pgon1= po_create_polygon("pgn1",cvtx_vlist);
  tri1= po_create_polygon("tri1",cvtx_vlist);
  bezier1= po_create_bezier("bezier1",bez_vtx_vlist);
  mesh1= po_create_mesh("msh1",cvtx_vlist,meshind,meshlengths,meshfacets);
  light1= po_create_light("lt1",&light_loc,&light_color);
  ambient1= po_create_ambient("amb1",&ambient_color);

  METHOD_RDY(lightgob);
  (*(lightgob->add_child))(light1);
  (*(lightgob->add_child))(ambient1);

  METHOD_RDY(gob)
  (*(gob->add_transform))( &test_trans );
  (*(gob->add_attribute))("color",P3D_COLOR,(P_Void_ptr)&test_color);
  (*(gob->add_attribute))("backcull",P3D_BOOLEAN,(P_Void_ptr)&test_flag);
  (*(gob->add_attribute))("color",P3D_COLOR,(P_Void_ptr)&test_color);
  (*(gob->add_attribute))("some-int",P3D_INT,(P_Void_ptr)&test_int);
  (*(gob->add_attribute))("some-float",P3D_FLOAT,(P_Void_ptr)&test_float);
  (*(gob->add_attribute))("some-point",P3D_POINT,(P_Void_ptr)&test_point);
  (*(gob->add_attribute))("some-vector",P3D_VECTOR,(P_Void_ptr)&test_vector);
  (*(gob->add_attribute))("some-string",P3D_STRING,(P_Void_ptr)test_string);
  (*(gob->add_attribute))("some-transform",P3D_TRANSFORM,
			  (P_Void_ptr)&test_trans);
  (*(gob->add_attribute))("some-other-thing",P3D_OTHER,(P_Void_ptr)0);

  (*(gob->add_child))(sphere1);
  (*(gob->add_child))(cylinder1);
  (*(gob->add_child))(torus1);
  (*(gob->add_child))(text1);
  (*(gob->add_child))(pmark1);
  (*(gob->add_child))(pline1);
  (*(gob->add_child))(pgon1);
  (*(gob->add_child))(tri1);
  (*(gob->add_child))(bezier1);
  (*(gob->add_child))(light1);
  (*(gob->add_child))(mesh1);

  METHOD_RDY(lightgob);
  (*(lightgob->print))();
  METHOD_RDY(gob);
  (*(gob->print))();

  METHOD_RDY(gob);
  (*(gob->destroy_self))(1);
  METHOD_RDY(lightgob);
  (*(lightgob->destroy_self))(1);
}

void hash_tests( VOIDLIST )
{
  P_String_Hash *hash1, *hash2;

  hash1= po_create_chash(257);
  hash2= po_create_chash(307);

  METHOD_RDY(hash1);

  (*(hash1->add))("firstkey","This is the first test string");
  (*(hash1->add))("secondkey","This is the second test string");
  
  METHOD_RDY(hash2);
  (*(hash2->add))("firstkey","This is the third test string");
  (*(hash2->add))("secondkey","This is the fourth test string");

  METHOD_RDY(hash1);
  fprintf(stderr,"hash1 firstkey: <%s>\n",
	  (char *)(*(hash1->lookup))("firstkey"));
  fprintf(stderr,"hash1 secondkey: <%s>\n",
	  (char *)(*(hash1->lookup))("secondkey"));

  METHOD_RDY(hash2);
  fprintf(stderr,"hash2 firstkey: <%s>\n",
	  (char *)(*(hash2->lookup))("firstkey"));
  fprintf(stderr,"hash2 secondkey: <%s>\n",
	  (char *)(*(hash2->lookup))("secondkey"));

  METHOD_RDY(hash1);
  (*(hash1->free))("firstkey");
  (*(hash1->free))("secondkey");
  if ( !(*(hash1->lookup))("firstkey") )
    fprintf(stderr,"hash1 free check: firstkey not found\n");
  if ( !(*(hash1->lookup))("secondkey") )
    fprintf(stderr,"hash1 free check: secondkey not found\n");

  METHOD_RDY(hash2);
  (*(hash2->free))("firstkey");
  (*(hash2->free))("secondkey");
  if ( !(*(hash2->lookup))("firstkey") )
    fprintf(stderr,"hash2 free check: firstkey not found\n");

  METHOD_RDY(hash1);
  (*(hash1->destroy_self))();

  METHOD_RDY(hash2);
  (*(hash2->destroy_self))();
}

main()
{
#ifdef never
  ger_toggledebug();
#endif

  ger_init("obj_tester");
  ind_setup();

  fprintf(stderr,"F-type vertex list tests\n");
  f_tests();

  fprintf(stderr,"Object tests\n");
  object_tests();

  fprintf(stderr,"Character hash table tests\n");
  hash_tests();

  fprintf(stderr,"Tests complete\n");

  return 0;
}
