#include <stdio.h>
#include <stdlib.h>
#include <p3dgen.h>
#include <drawp3d.h>

#include "fl_gl_interface.h"
#include "Fl_DrawP3D_Window.h"

#define ERRCHK( string ) if (!(string)) fprintf(stderr,"Error!\n");

void rot_cb(Fl_Slider*, void*) {
  myGlWindow->redraw();
}

void redraw_cb(Fl_Button*, void*) {
  myGlWindow->redraw();
}

void exit_cb(Fl_Button*, void*) {
  exit(0);
}

static P_Point lookfrom= { 1.0, 2.0, 17.0 };
static P_Point lookat= { 1.0, 2.0, -3.0 };
static P_Vector up= { 0.0, 1.0, 0.0 };
static float fovea= 45.0;
static float hither= -5.0;
static float yon= -30.0;

static P_Point text_loc= { 0.0, 0.0, 3.0 };
static P_Vector text_u= { 1.0, 0.0, 0.0 };
static P_Vector text_v= { 0.0, 0.0, 1.0 };

static P_Point light_loc= {0.0, 1.0, 10.0};
static P_Color light_color= { P3D_RGB, 0.8, 0.8, 0.8, 1.0 };
static P_Color ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 1.0 };
static P_Color test_color= { P3D_RGB, 0.2, 0.3, 0.8, 0.9 };
static P_Color red_color= { P3D_RGB, 1.0, 0.0, 0.0, 1.0 };
static P_Color green_color= { P3D_RGB, 0.0, 1.0, 0.0, 1.0 };
static P_Color blue_color= { P3D_RGB, 0.0, 0.0, 1.0, 1.0 };
static P_Vector test_vector= { 1.414, 0.0, 1.414 };
static P_Vector x_vector= { 1.0, 0.0, 0.0 };
static P_Vector y_vector= { 0.0, 1.0, 0.0 };
static float b_cvtx_coords[3*16]= {
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

void buildModel() {
  ERRCHK( dp_open("rotbez") );
  ERRCHK( dp_rotate(&test_vector,270.0) );
  ERRCHK( dp_translate(-2.0,0.0,5.0) );
  ERRCHK( dp_gobcolor(&red_color) );
  ERRCHK( dp_gobmaterial(p3d_shiny_material) );
  ERRCHK( dp_bezier(P3D_CVTX, P3D_RGB, b_cvtx_coords) );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("shovecyl") );
  ERRCHK( dp_translate(0.0, 0.0, 2.0) );
  ERRCHK( dp_color_attr("color",&green_color) );
  ERRCHK( dp_gobmaterial(p3d_shiny_material) );
  ERRCHK( dp_cylinder() );
  ERRCHK( dp_close() );
  ERRCHK( dp_open("mygob") );
  ERRCHK( dp_gobcolor(&test_color) );
  ERRCHK( dp_sphere() );
  ERRCHK( dp_torus(3.0, 1.0) );
  ERRCHK( dp_text("Sample Text",&text_loc, &text_u, &text_v) );
  ERRCHK( dp_textheight(0.5) );
  ERRCHK( dp_child("shovecyl") );
  ERRCHK( dp_child("rotbez") );
  ERRCHK( dp_close() );
}

void buildLights() {
  ERRCHK( dp_open("mylights") );
  ERRCHK( dp_light(&light_loc, &light_color) );
  ERRCHK( dp_ambient(&ambient_color) );
  ERRCHK( dp_close() );
}

void buildCam() {
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );
}

int Fl_DrawP3D_Window::count= 0;


Fl_DrawP3D_Window::Fl_DrawP3D_Window(int X, int Y, int W, int H, const char *L)
  : Fl_Gl_Window(X, Y, W, H, L) {

  char name[128];
  sprintf(name,"myrenderer%d",count);
  initialized= 0;
  ERRCHK( dp_init_ren(name,"gl","nomanage","") );
  ERRCHK( dp_std_cmap( 5.0, 9.0, 0 ) );
  count++;
}

void Fl_DrawP3D_Window::draw() {
  ERRCHK( dp_open("parent"));
  ERRCHK( dp_rotate(&x_vector,rotate->value()) );
  ERRCHK( dp_rotate(&y_vector,rotate2->value()) );
  ERRCHK( dp_child("mygob") );
  ERRCHK( dp_close() );
  ERRCHK( dp_snap("parent","mylights","mycamera") );
}

int main( int argc, char* argv[] ) {
  Fl_Window* w= make_window();
  buildModel();
  buildLights();
  buildCam();
  w->show(argc,argv);
  return Fl::run();
}

