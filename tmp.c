/*
 * cyl.c
 * Starter code for 15-462, Computer Graphics 1,
 * Assignment 2: Generalized Cylinders
 *
 * This is a skeleton for an OpenGL program that demonstrates
 * z-buffer rendering, double buffering, spinning an object
 * around, interactive redisplay, and lighting.
 * It uses Xforms for the user interface (sliders & buttons).
 *
 * The stuff I've put in redisplay() is just one way to compute
 * transformation matrices to use as reference frames; there are
 * many other ways.  See the assignment handout on the web.
 *
 * When compiling on Suns with 24 bits per pixel (i.e. "Sparc Ultra Creator")
 * and the Mesa OpenGL library, you'll probably get the warning messages:
 *   alloc_back_buffer: Invalid argument
 *   X/Mesa error: alloc_back_buffer: Shared memory error (shmget), disabling.
 * but the program seems to work fine anyway.  Let me know if you figure
 * out why this is happening.
 *
 * Paul Heckbert
 *
 * 26 September 1997
 * 
 * Added save function to cyl.c.
 *
 * Chris Rodriguez
 *
 * 4 Feb 1999
 *
 * Turned this into a whole program, Joel Welling, Sept 1999.
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <GL/glu.h>    /* some OpenGL extensions */
#include <math.h>

#include "glxf.h"      /* code to glue OpenGL and Xforms together */
#include "form.h"   /* UI framework built by XForms */
#include "pic.h"    /*for saving tiff pictures.  */
#include "spline.h" /*header file to add spline functionality.*/

typedef struct texpair_struct {
  float u;
  float v;
} TexPair;

static int DEBUG = 0;

#define LENSQR_EPSILON 0.0001
#define RADTODEG (180.0/M_PI)
#define DEGTORAD (M_PI/180.0)

#define VIEW_X 0.0
#define VIEW_Y 0.0
#define VIEW_Z 5.0

#define VFOV 45.0
#define ZNEAR 0.1
#define ZFAR  50.0

float view_range= VIEW_Z;

static Glxf_pane pane;	/* OpenGL-Xforms integration */
static Glxf_pane loop_canvas; /* OpenGL-Xforms integration */
FD_cyl *form;  	/* global form */

ctlpts* loop_ctl= NULL;
ctlpts* skel_ctl= NULL;

point3d* loop_buf= NULL;
int loop_buf_size= 0;
int loop_length= 0;
float loop_scale= 1.0;

int skel_length= 0;

int cball_roll_flag= 0;
int cball_shift_flag= 0;
double cball_matrix[4][4];
float cball_ballbase[3];
float cball_shiftbase[3];

double skel_matrix[4][4];
float skel_offset[3];

point3d* mesh_buf= NULL;
vec3d* norm_buf= NULL;
TexPair* texcoord_buf= NULL;
int mesh_buf_size= 0;
GLuint texname;

static int norms = 1, triad = 1, skin = 1, texture = 0;

/* Forward defs */
void redisplay_all();
static void redisplay();
static void regen_mesh();
void save(char* filename); /* so that you can save proceduraly, if you want to.*/

extern void timing_cb(FL_OBJECT *obj, long val) {
  int i;
  struct rusage start_rusage;
  struct rusage end_rusage;
  long s_usec,s_sec,u_usec,u_sec;
  int n= 100;

  getrusage(RUSAGE_SELF,&start_rusage);

  for (i=0; i<n; i++) redisplay();

  getrusage(RUSAGE_SELF,&end_rusage);
  s_usec= end_rusage.ru_stime.tv_usec - start_rusage.ru_stime.tv_usec;
  s_sec= end_rusage.ru_stime.tv_sec - start_rusage.ru_stime.tv_sec;
  u_usec= end_rusage.ru_utime.tv_usec - start_rusage.ru_utime.tv_usec;
  u_sec= end_rusage.ru_utime.tv_sec - start_rusage.ru_utime.tv_sec;

  if (s_usec<0) {
    s_sec -= 1;
    s_usec += 1000000;
  }
  if (u_usec<0) {
    u_sec -= 1;
    u_usec += 1000000;
  }

  fprintf(stderr,"times for %d redraws: %d.%06du   %d.%06ds\n",
          n,u_sec,u_usec,s_sec,s_usec);
}

void regen_loop() {
  int i;
  if (!loop_ctl) return;
  loop_length= (int)fl_get_slider_value(form->loop_segs_slider)+1;
  loop_scale= fl_get_slider_value(form->loop_scale_slider);

  if (DEBUG) fprintf(stderr,"loop length %d, scale %f\n",
		     loop_length, loop_scale);
		     
  if (loop_length>loop_buf_size) {
    free(loop_buf);
    if (!(loop_buf=(point3d*)malloc(loop_length*sizeof(point3d)))) {
      fprintf(stderr,"Unable to allocate %d bytes!\n",
	      loop_length*sizeof(point3d));
      exit(-1);
    }
    loop_buf_size= loop_length;
  }
  for (i=0; i<loop_length; i++) {
    point3d tmp;
    spline_calc(&tmp,loop_ctl,
		(1.0/(float)(loop_length-1))*(float)i,
		fl_get_slider_value(form->loop_alpha_slider));
    loop_buf[i].x= loop_scale*tmp.x;
    loop_buf[i].y= loop_scale*tmp.y;
    loop_buf[i].z= loop_scale*tmp.z;
  }
  regen_mesh();
  redisplay_all();
}

void regen_loop_cb(FL_OBJECT *obj, long val) {
  regen_loop();
}

void range_cb(FL_OBJECT *obj, long val) {
  GLenum old_mode;

  /* Save the new value */
  view_range= fl_get_slider_value(form->range_slider);

  /* Update top (view) transformation matrix */
  glGetIntegerv(GL_MATRIX_MODE,(int*)&old_mode);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(VIEW_X, VIEW_Y, -view_range);
  glMatrixMode(old_mode);

  /* redraw */
  redisplay();
}

static void draw_triad() {
    glBegin(GL_LINES);

    /* draw x axis in red, y axis in green, z axis in blue */
    glColor3f(1., .2, .2);
    glVertex3f(0., 0., 0.);
    glVertex3f(loop_scale, 0., 0.);

    glColor3f(.2, 1., .2);
    glVertex3f(0., 0., 0.);
    glVertex3f(0., loop_scale, 0.);

    glColor3f(.2, .2, 1.);
    glVertex3f(0., 0., 0.);
    glVertex3f(0., 0., loop_scale);

    glEnd();
}

static void draw_circle(int n) {
    int i;
    float ang;

    /* draw a regular n-gon of radius 1 in the xy plane */
    glColor3f(1., 1., 1.);	/* set current color to white */
    glBegin(GL_LINE_LOOP);
    for (i=0; i<n; i++) {
	ang = 2.*M_PI*i/n;
	glVertex2f(cos(ang), sin(ang));
    }
    glEnd();
}

static void draw_loop() {
    int i;
    if (loop_length>0) {
      glBegin(GL_LINE_STRIP);
      for (i=0; i<loop_length; i++) glVertex3fv((float*)(loop_buf+i));
      glEnd();
    }
}

static void draw_norms() {
  int i, j;
  if (loop_length>0 && skel_length>0) {
    point3d* v= mesh_buf;
    vec3d* n= norm_buf;
    glColor3f(0.7,0.7,0.7);
    glBegin(GL_LINES);
    for (i=0; i<skel_length; i++) {
      for (j=0; j<loop_length; j++) {
	glVertex3fv((float*)v);
	glVertex3f(v->x+0.25*n->x, v->y+0.25*n->y, v->z+0.25*n->z);
	n++;
	v++;
      }
    }
    glEnd();
  }
}

static void draw_mesh() {
  int i, j;
  if (loop_length>0 && skel_length>0) {
    glColor3f(0.8,0.8,0.3);
    for (i=0; i<skel_length; i++) {
      glBegin(GL_LINE_STRIP);
      for (j=0; j<loop_length; j++) 
	glVertex3fv((float*)(mesh_buf+(i*loop_length+j)));
      glEnd();
    }
    /* Connect the dots */
    for (i=0; i<skel_length-1; i++) { /* one less tri strip than row */
      glBegin(GL_LINE_STRIP);
      for (j=0; j<loop_length; j++) {
	glVertex3fv((float*)(mesh_buf+((i+1)*loop_length+j)));
	glVertex3fv((float*)(mesh_buf+(i*loop_length+j)));
      }
      glEnd();
    }
  }
}

void tex_init(char *filename) {
  Pic* in;

  /*
   * Read in the file; tiff_read will allocate the space for it.
   */
  in = tiff_read(filename, NULL);
  if( !in ) {
    printf("Unable to open input file '%s'\n", filename);
    return;
  }

  /* Dimensions of image should be 2**n by 2**m; this assumes RGB only */

  glGenTextures(1, &texname);
  glBindTexture(GL_TEXTURE_2D, texname);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, in->nx, in->ny, 0, GL_RGB, 
	       GL_UNSIGNED_BYTE, in->pix);

}


static void draw_skin() {
  int i, j;
  
  if (texture) {
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, texname);
  }

  if (loop_length>0 && skel_length>0) {
    glEnable(GL_LIGHTING);	/* turn on lighting */
    glColor3f(0.8,0.8,0.3);
    for (i=0; i<skel_length-1; i++) { /* one less tri strip than row */
      glBegin(GL_TRIANGLE_STRIP);
      for (j=0; j<loop_length; j++) {
	glNormal3fv((float*)(norm_buf+((i+1)*loop_length+j)));
	if (texture)
	  glTexCoord2fv((float*)(texcoord_buf+((i+1)*loop_length+j)));
	glVertex3fv((float*)(mesh_buf+((i+1)*loop_length+j)));
	glNormal3fv((float*)(norm_buf+(i*loop_length+j)));
	if (texture)
	  glTexCoord2fv((float*)(texcoord_buf+(i*loop_length+j)));
	glVertex3fv((float*)(mesh_buf+(i*loop_length+j)));
      }
      glEnd();
    }
    glDisable(GL_LIGHTING);
  }

  if (texture) glDisable(GL_TEXTURE_2D);
}

static void draw_skel() {
    int i;
    if (skel_length>0) {
      glBegin(GL_LINE_STRIP);
      for (i=0; i<skel_length; i++) {
	point3d tmp;
	float slider_val= fl_get_slider_value(form->skel_alpha_slider);
	spline_calc(&tmp,skel_ctl,
		    (1.0/(float)(skel_length-1))*(float)i,slider_val);
	glVertex3fv((float*)&tmp);
      }
      glEnd();
    }
}

static void matrix_transpose(double a[4][4], double b[4][4]) {
    int i, j;

    for (i=0; i<4; i++)
	for (j=0; j<4; j++)
	    b[i][j] = a[j][i];
}

static void matrix_print(char *str, double a[4][4]) {
    int i;

    printf("matrix %s:\n", str);
    for (i=0; i<4; i++)
	printf(" %8.3f %8.3f %8.3f %8.3f\n",
	    a[i][0], a[i][1], a[i][2], a[i][3]);
}

static void matrix_transform_affine(double m[4][4],
double x, double y, double z, float pt[3]) {
    /* transform the point (x,y,z) by the matrix m, which is
     * assumed to be affine (last row 0 0 0 1) */
    /* this is just a matrix-vector multiply */
    pt[0] = m[0][0]*x + m[0][1]*y + m[0][2]*z + m[0][3];
    pt[1] = m[1][0]*x + m[1][1]*y + m[1][2]*z + m[1][3];
    pt[2] = m[2][0]*x + m[2][1]*y + m[2][2]*z + m[2][3];
}

static void v3_sub(float a[3], float b[3], float c[3]) {
    c[0] = a[0]-b[0];
    c[1] = a[1]-b[1];
    c[2] = a[2]-b[2];
}

static void v3_add(float a[3], float b[3], float c[3]) {
    c[0] = a[0]+b[0];
    c[1] = a[1]+b[1];
    c[2] = a[2]+b[2];
}

static float v3_dot(float a[3], float b[3]) {
  return(a[0]*b[0]+a[1]*b[1]+a[2]*b[2]);
}

static void v3_cross(float a[3], float b[3], float c[3]) {
    /* cross product of two vectors: c = a x b */
    c[0] = a[1]*b[2]-a[2]*b[1];
    c[1] = a[2]*b[0]-a[0]*b[2];
    c[2] = a[0]*b[1]-a[1]*b[0];
}

static void v3_copy( float to[3], float from[3] ) {
  to[0]= from[0];
  to[1]= from[1];
  to[2]= from[2];
}

static void v3_normalize( float a[3] ) {
  float len;
  len= sqrt(a[0]*a[0] +a[1]*a[1] +a[2]*a[2]);
  if (len==0.0) {
    fprintf(stderr,"Tried to normalize zero-length vector!\n");
    exit(-1);
  }
  else {
    a[0] /= len;
    a[1] /= len;
    a[2] /= len;
  }
}
  
static float v3_lensqr( float a[3] ) {
  return( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
}

static void mult_aligning_rotation(float from[3], float to[3]) {
  /* How much of my life have I spent re-implementing this routine? */
  float cross[3];
  float fm_norm[3];
  float to_norm[3];

  v3_copy(fm_norm, from);
  v3_normalize(fm_norm);
  v3_copy(to_norm, to);
  v3_normalize(to_norm);
  v3_cross(fm_norm,to_norm,cross);

  if (v3_lensqr(cross)<=LENSQR_EPSILON) {
    /* parallel or antiparallel */
    if (v3_dot(fm_norm,to_norm) > 0.0) {
      /* parallel- this routine is a no-op */
    }
    else {
      /* anti-parallel.  Pick a perpendicular axis and flip. */
      static float x_axis[3]= {1.0,0.0,0.0};
      static float y_axis[3]= {0.0,1.0,0.0};
      float rot_axis[3];
      if (v3_dot(fm_norm,x_axis)>LENSQR_EPSILON) {
	v3_cross(fm_norm,x_axis,rot_axis);
      }
      else {
	v3_cross(fm_norm,y_axis,rot_axis);
      }
      glRotatef(180.0,rot_axis[0],rot_axis[1],rot_axis[2]);
    }
  }
  else {
    /* All's well */
    float theta_rad= acos(v3_dot(fm_norm,to_norm));
    glRotatef(RADTODEG*theta_rad,cross[0],cross[1],cross[2]);
  }
}

void glGetModelViewUntransposed(double m[4][4]) {
    double mt[4][4];

    /* get the current transformation matrix (the Modelview matrix, to be
     * precise) and write it into m.
     * Transpose it since OpenGL uses transposed matrices, to reduce confusion.
     */
    glGetDoublev(GL_MODELVIEW_MATRIX, &mt[0][0]);
    matrix_transpose(mt, m);
}

void glLoadMatrixUntransposed(double m[4][4]) {
    double mt[4][4];

    /* load the current transformation matrix with m (after transposing).
     * We transpose it since OpenGL uses transposed matrices,
     * to reduce confusion. */
    matrix_transpose(m, mt);
    glLoadMatrixd(&mt[0][0]);
}

void glMultMatrixUntransposed(double m[4][4]) {
    double mt[4][4];

    /* load the current transformation matrix with m (after transposing).
     * We transpose it since OpenGL uses transposed matrices,
     * to reduce confusion. */
    matrix_transpose(m, mt);
    glMultMatrixd(&mt[0][0]);
}

static void cball_init() {
  /* Just want identity in cball matrix */
  int i,j;
  for (i=0;i<4;i++)
    for (j=0;j<4;j++)
      cball_matrix[i][j]= 0.0;
  for (i=0;i<4;i++) cball_matrix[i][i]= 1.0;
}

static void cball_roll( int x, int y ) {
  int form_xsize;
  int form_ysize;
  float xctr, yctr;
  float delta[3];

  form_xsize= form->pane->w;
  form_ysize= form->pane->w;

  xctr= 0.5*form_xsize;
  yctr= 0.5*form_ysize;

  delta[0]= ((float)x-xctr)/xctr;
  delta[1]= (yctr-(float)y)/yctr;

  if (delta[0]*delta[0] + delta[1]*delta[1] <= 1.0) {
    /* mouse is in the ball */
    delta[2]= sqrt(1.0- (delta[0]*delta[0] + delta[1]*delta[1]));

    if (cball_roll_flag) {
      GLenum old_mode;
      glGetIntegerv(GL_MATRIX_MODE,(int*)&old_mode);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      mult_aligning_rotation(cball_ballbase,delta);
      glMultMatrixUntransposed(cball_matrix);
      glGetModelViewUntransposed(cball_matrix);
      glPopMatrix();
      glMatrixMode(old_mode);
    }
    else cball_roll_flag= 1;

    cball_ballbase[0]= delta[0];
    cball_ballbase[1]= delta[1];
    cball_ballbase[2]= delta[2];
  }
  redisplay();
}

static void cball_shift( int x, int y ) {
  int form_xsize;
  int form_ysize;
  float xctr, yctr;
  float delta[3];

  form_xsize= form->pane->w;
  form_ysize= form->pane->w;

  xctr= 0.5*form_xsize;
  yctr= 0.5*form_ysize;

  delta[0]= ((float)x-xctr)/xctr;
  delta[1]= (yctr-(float)y)/yctr;

  if (delta[0]*delta[0] + delta[1]*delta[1] <= 1.0) {
    /* mouse is in the ball */
    delta[2]= 0.0;

    if (cball_shift_flag) {
      GLenum old_mode;
      float scale;

      scale= 0.5*view_range*sin(DEGTORAD*VFOV);

      glGetIntegerv(GL_MATRIX_MODE,(int*)&old_mode);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glTranslatef(scale*(delta[0]-cball_shiftbase[0]),
		   scale*(delta[1]-cball_shiftbase[1]),
		   scale*(delta[2]-cball_shiftbase[2]));
      glMultMatrixUntransposed(cball_matrix);
      glGetModelViewUntransposed(cball_matrix);
      glPopMatrix();
      glMatrixMode(old_mode);
    }
    else cball_shift_flag= 1;

    cball_shiftbase[0]= delta[0];
    cball_shiftbase[1]= delta[1];
    cball_shiftbase[2]= delta[2];
  }
  redisplay();
}

static void light_init() {
    /* set up OpenGL to do lighting
     * we've set up three lights */

    /* set material properties */
    GLfloat white8[] = {.8, .8, .8, 1.};
    GLfloat white2[] = {.2, .2, .2, 1.};
    GLfloat black[] = {0., 0., 0., 1.};
    GLfloat mat_shininess[] = {50.};		/* Phong exponent */

    GLfloat light0_position[] = {1., 1., 5., 0.}; /* directional light (w=0) */
    GLfloat white[] = {1., 1., 1., 1.};

    GLfloat light1_position[] = {-3., 1., -1., 0.};
    GLfloat red[] = {1., .3, .3, 1.};

    GLfloat light2_position[] = {1., 8., -2., 0.};
    GLfloat blue[] = {.3, .4, 1., 1.};

    /* Set up the right context */
    assert(glXMakeCurrent(pane.display, pane.window, pane.context));

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, black);	/* no ambient */
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white8);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white2);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    /* set up several lights */
    /* one white light for the front, red and blue lights for the left & top */

    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, red);
    glLightfv(GL_LIGHT1, GL_SPECULAR, red);
    glEnable(GL_LIGHT1);

    glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, blue);
    glLightfv(GL_LIGHT2, GL_SPECULAR, blue);
    glEnable(GL_LIGHT2);

    glEnable(GL_NORMALIZE);	/* normalize normal vectors */
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);	/* two-sided lighting*/

    

}

void pane_init()
{
  int red_bits, green_bits, blue_bits;
  struct {GLint x, y, width, height;} viewport;

  /* Set up the right context */
  assert(glXMakeCurrent(pane.display, pane.window, pane.context));
  
  glEnable(GL_DEPTH_TEST);	/* turn on z-buffer */
  
  glGetIntegerv(GL_RED_BITS, &red_bits);
  glGetIntegerv(GL_GREEN_BITS, &green_bits);
  glGetIntegerv(GL_BLUE_BITS, &blue_bits);
  glGetIntegerv(GL_VIEWPORT, &viewport.x);
  printf("OpenGL window has %d bits red, %d green, %d blue; viewport is %dx%d\n",
	 red_bits, green_bits, blue_bits, viewport.width, viewport.height);

  /* setup perspective camera with OpenGL */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective( VFOV, 
		 /*aspect ratio*/ (double)viewport.width/viewport.height,
		  ZNEAR, ZFAR );

  /* from here on we're setting modeling transformations */
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(VIEW_X, VIEW_Y, -view_range);

}

void loop_canvas_init()
{
  int red_bits, green_bits, blue_bits;

  /* Set up the right context */
  assert(glXMakeCurrent(pane.display, pane.window, pane.context));

  glGetIntegerv(GL_RED_BITS, &red_bits);
  glGetIntegerv(GL_GREEN_BITS, &green_bits);
  glGetIntegerv(GL_BLUE_BITS, &blue_bits);
  printf("Loop window has %d bits red, %d bits green, %d bits blue\n",
    red_bits, green_bits, blue_bits);

  /* disable z-buffer, lighting, and shading */
  glDisable(GL_DEPTH_TEST);

}

static void loop_redisplay() {
  int i;
  if (DEBUG) fprintf(stderr,"loop_redisplay()\n");

  /* Set up the right context */
  assert(glXMakeCurrent(loop_canvas.display, loop_canvas.window, 
			loop_canvas.context));

  /* set viewport for OpenGL (part 1) */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  gluOrtho2D(-2.0, 2.0, -2.0, 2.0);

  /* set viewport for OpenGL (part 2) */
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  /* clear image buffer to black */
  glClearColor(0, 0, 0, 255);
  glClear(GL_COLOR_BUFFER_BIT);

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);

  if (loop_ctl) {
    /* Draw the loop */
    glColor3f(1.0,1.0,1.0);
    draw_loop();

    /* Draw the control points */
    glColor3f(0.0,0.0,1.0);
    glPointSize(3.0);
    glBegin(GL_POINTS);
    for (i=0; i<loop_ctl->numofpoints; i++) 
      glVertex3f(loop_scale*loop_ctl->points[i].x,
		 loop_scale*loop_ctl->points[i].y,
		 loop_scale*loop_ctl->points[i].z);
    glEnd();
  }

  glFlush();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void step_skel_trans(float v, int first) {
  point3d herept;
  vec3d gradvec;
  float here[3];
  float grad[3];
  static float oldgrad[3];

  spline_calc(&herept, skel_ctl, v,
	      fl_get_slider_value(form->skel_alpha_slider));
  here[0]= herept.x; here[1]= herept.y; here[2]= herept.z;

  /* We'll try to get the gradient correctly, but it can be zero 
   * if the tension parameter is zero.  If that happens, we'll do a 
   * simple forward difference.  If that too fails, make a guess.
   */
  spline_grad(&gradvec, skel_ctl, v,
	      fl_get_slider_value(form->skel_alpha_slider));
  grad[0]= gradvec.x; grad[1]= gradvec.y; grad[2]= gradvec.z;

  if (v3_dot(grad,grad)<=LENSQR_EPSILON) {
    /* Gradient is locally zero */
    float dv= 0.05;
    point3d steppt;
    spline_calc(&steppt, skel_ctl, v+dv,
		fl_get_slider_value(form->skel_alpha_slider));
    grad[0]= (steppt.x - herept.x)/dv;
    grad[1]= (steppt.y - herept.y)/dv;
    grad[2]= (steppt.z - herept.z)/dv;

    if (v3_dot(grad,grad)<=LENSQR_EPSILON) {
      /* Sigh; still zero */
      if (first) {
	grad[0]= 1.0; grad[1]= 0.0; grad[2]= 0.0; /* arbitrary vec in xy pln */
      }
      else {
	grad[0]= oldgrad[0], grad[1]= oldgrad[1], grad[2]= oldgrad[2];
      }
    }
  }

  if (first) {
    /* want identity in skel matrix */
    int i,j;
    for (i=0;i<4;i++)
      for (j=0;j<4;j++)
	skel_matrix[i][j]= 0.0;
    for (i=0;i<4;i++) skel_matrix[i][i]= 1.0;

    /* want to start with a rot that will flip z direction into grad dir */
    oldgrad[0]= 0.0; oldgrad[1]= 0.0; oldgrad[2]= 1.0;
  }

  /* Want to tag the new rotation on *before* previous rots */
  glPushMatrix();
  glLoadIdentity();
  mult_aligning_rotation(oldgrad,grad);
  glMultMatrixUntransposed(skel_matrix);
  glGetModelViewUntransposed(skel_matrix);
  glPopMatrix();

  skel_offset[0]= here[0];
  skel_offset[1]= here[1];
  skel_offset[2]= here[2];

  oldgrad[0]= grad[0]; oldgrad[1]= grad[1]; oldgrad[2]= grad[2];

}

void regen_skel() {
  skel_length= (int)fl_get_slider_value(form->skel_segs_slider)+1;
  if (DEBUG) fprintf(stderr,"skel length %d\n",skel_length);
  regen_mesh();
  redisplay_all();
}

void regen_skel_cb(FL_OBJECT *obj, long val) {
  regen_skel();
}

static void add_cross(float out[3], point3d* ctr, point3d* t1, point3d* t2) {
  float v1[3];
  float v2[3];
  float cross[3];

  v1[0]= t1->x - ctr->x;
  v1[1]= t1->y - ctr->y;
  v1[2]= t1->z - ctr->z;
  
  v2[0]= t2->x - ctr->x;
  v2[1]= t2->y - ctr->y;
  v2[2]= t2->z - ctr->z;

  v3_cross(v1,v2,cross);

  out[0] += cross[0];
  out[1] += cross[1];
  out[2] += cross[2];
}

void calc_mesh_norms() {
  int i,j;
  point3d* mhere;
  vec3d* here;
  float tmp[3];

  if (!loop_length || !skel_length) return;

  /* Sum the normals of all adjacent triangles */
  for (i=0; i<skel_length; i++) {
    for (j=0; j<loop_length; j++) {
      here= norm_buf+(i*loop_length+j);
      mhere= mesh_buf+(i*loop_length+j);
      
      tmp[0]= tmp[1]= tmp[2]= 0.0;

      if (i<(skel_length-1) && j>0) 
	add_cross(tmp, mhere, mhere+loop_length, mhere-1);

      if (i<(skel_length-1) && j<(loop_length-1)) 
	add_cross(tmp, mhere, mhere+1, mhere+loop_length);

      if (i>0 && j<(loop_length-1)) 
	add_cross(tmp, mhere, mhere-loop_length, mhere+1);

      if (i>0 && j>0) 
	add_cross(tmp, mhere, mhere-1, mhere-loop_length);

      if (loop_length*skel_length > 1) v3_normalize(tmp);
      else {
	tmp[0]= 0.0; tmp[1]= 0.0; tmp[2]= 1.0; /* 1-vertex mesh; paranoia */
      }
      here->x= tmp[0]; here->y= tmp[1]; here->z= tmp[2];
    }
  }
}

void regen_mesh() {
  int i,j;
  int mesh_length;
  point3d* here;
  TexPair* t_here;
  float v;
  float dv;
  float u;
  float du;

  if (!loop_length || !skel_length) return;

  if (loop_length*skel_length>mesh_buf_size) {
    if (mesh_buf) free(mesh_buf);
    if (!(mesh_buf=(point3d*)malloc(loop_length*skel_length
				    *sizeof(point3d)))) {
      fprintf(stderr,"Unable to allocate %d bytes!\n",
	      loop_length*skel_length*sizeof(point3d));
      exit(-1);
    }
    if (norm_buf) free(norm_buf);
    if (!(norm_buf=(vec3d*)malloc(loop_length*skel_length
				    *sizeof(vec3d)))) {
      fprintf(stderr,"Unable to allocate %d bytes!\n",
	      loop_length*skel_length*sizeof(vec3d));
      exit(-1);
    }
    if (texcoord_buf) free(texcoord_buf);
    if (!(texcoord_buf=(TexPair*)malloc(loop_length*skel_length
					*sizeof(TexPair)))) {
      fprintf(stderr,"Unable to allocate %d bytes!\n",
	      loop_length*skel_length*sizeof(TexPair));
      exit(-1);
    }
    mesh_buf_size= loop_length*skel_length;
  }

  v= 0.0;
  dv= 1.0/(float)(skel_length-1);

  for (i=0; i<skel_length; i++) {
    step_skel_trans(v,(i==0));
    for (j=0; j<loop_length; j++) {
      here= mesh_buf+(i*loop_length+j);
      matrix_transform_affine(skel_matrix,
			      loop_buf[j].x,loop_buf[j].y,loop_buf[j].z,
			      (float*)here);
      here->x += skel_offset[0];
      here->y += skel_offset[1];
      here->z += skel_offset[2];
    }
    v += dv;
  }

  du= loop_ctl->numofpoints/(float)(loop_length-3);
  dv= skel_ctl->numofpoints/(float)(skel_length-1);
  v= 0.0;
  for (i=0; i<skel_length; i++) {
    u= 0.0;
    for (j=0; j<loop_length; j++) {
      t_here= texcoord_buf+(i*loop_length+j);
      t_here->u= u;
      t_here->v= v;
      u += du;
    }
    v += dv;
  }
  
  calc_mesh_norms();
}

static void redisplay() {
    int i;

    /* Set up the right context */
    assert(glXMakeCurrent(pane.display, pane.window, pane.context));

    /* clear image buffer to black */
    glClearColor(0, 0, 0, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); /* clear image, zbuf */

    glEnable(GL_DEPTH_TEST);	/* turn on z-buffer */
  

    glPushMatrix();			/* save current transform matrix */

    glMultMatrixUntransposed(cball_matrix);
    if (skel_length>0) {
      if (triad) {
	draw_skel();
      }
    }

    if (skin) draw_skin();
    else {
      draw_mesh();
      draw_skel();
    }

    if (norms) draw_norms();
      
    if (triad) {
      if (skel_length>0) {
	float v= 0.0;
	float dv= 1.0/(float)(skel_length-1);
	glPushMatrix();
	for (i=0; i<skel_length; i++) {
	  step_skel_trans(v,(i==0));
	  glPushMatrix();
	  glTranslatef(skel_offset[0],skel_offset[1],skel_offset[2]);
	  glMultMatrixUntransposed(skel_matrix);
	  if (triad) draw_triad();
	  glPopMatrix();
	  v += dv;
	}
	glPopMatrix();
      }
    }

    glPopMatrix();

    /* swap buffers (for double buffering) */
    glXSwapBuffers(pane.display, pane.window);
}

void redisplay_all()
{
  redisplay();
  loop_redisplay();
}

/* Callbacks from form. */
void redisplay_proc(FL_OBJECT *obj, long val) {
    norms = fl_get_button(form->norms_button);
    triad  = fl_get_button(form->triad_button);
    skin   = fl_get_button(form->skin_button);
    redisplay_all();
}


void save_callback(FL_OBJECT *obj, long val) {
 
  fl_show_fselector("Save to Tiff File", ".", "*.tiff", "");
  save(strdup(fl_get_filename()));

}

void load_loop( char* fname )
{
  ctlpts* tmp= NULL;
  if (DEBUG) fprintf(stderr,"Loading spline loop from <%s>\n",fname);
  tmp= ReadCtlpts(fname);
  if (tmp != NULL) {
    PrintCP(tmp);
    free_ctlpts(loop_ctl);
    loop_ctl= tmp;
    regen_loop();
  }
  else {
    fprintf(stderr,"Read failed on <%s!>\n",fname);
  }
}

void load_loop_cb(FL_OBJECT *obj, long val) {
  fl_show_fselector("Load Loop Spline", ".", "*.sp", "");
  load_loop(strdup(fl_get_filename()));
}

void load_skel( char* fname )
{
  ctlpts* tmp= NULL;
  if (DEBUG) fprintf(stderr,"Loading spline skeleton from <%s>\n",fname);
  tmp= ReadCtlpts(fname);
  if (tmp != NULL) {
    PrintCP(tmp);
    free_ctlpts(skel_ctl);
    skel_ctl= tmp;
    regen_skel();
  }
  else {
    fprintf(stderr,"Read failed on <%s!>\n",fname);
  }
}

void load_skel_cb(FL_OBJECT *obj, long val) {
  fl_show_fselector("Load Skeleton Spline", ".", "*.sp", "");
  load_skel(strdup(fl_get_filename()));
}

void save (char *filename) {
  int i;
  int j;
  Pic *in = pic_alloc(640,480,3,NULL);

  printf("File to save to: %s\n", filename);
  redisplay();

  glXSwapBuffers(pane.display, pane.window);
  /*Gets the most recently updated image (as made by redisplay()).*/

  for (i=479; i>=0; i--) {
    glReadPixels(0,479-i,640,1,GL_RGB, GL_UNSIGNED_BYTE, 
		 &in->pix[i*in->nx*in->bpp]);
  }

  if (tiff_write(strdup(filename), in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");
  pic_free(in);
}

static void error_check(int loc) {
    /* this routine checks to see if OpenGL errors have occurred recently */
    GLenum e;

    while ((e = glGetError()) != GL_NO_ERROR)
	fprintf(stderr, "Error: %s before location %d\n",
	    gluErrorString(e), loc);
}


static void maincanvas_handle_event(XEvent *event) {
  switch (event->type) {
  case KeyPress:          /* key pressed */
  case KeyRelease:        /* key released */
    {
      char buf[10];
      int n;
      KeySym keysym;
      
      n = XLookupString(&event->xkey, buf, sizeof buf - 1,
			&keysym, 0);
      buf[n] = 0;
      if (keysym=='q') exit(0);
    }
    break;
    
  case ButtonPress:    /* button press */
    if (event->xbutton.button==1) {
      cball_roll(event->xmotion.x,event->xmotion.y);
    }
    else if (event->xbutton.button==2) {
      cball_shift(event->xmotion.x,event->xmotion.y);
    }
    break;
    
  case ButtonRelease:  /* button release */
    if (event->xbutton.button==1) cball_roll_flag= 0;
    else if (event->xbutton.button==2) cball_shift_flag= 0;
    glFlush();
    break;
    
  case MotionNotify:   /* mouse pointer moved */
    if (cball_roll_flag) cball_roll(event->xmotion.x,event->xmotion.y);
    if (cball_shift_flag) cball_shift(event->xmotion.x,event->xmotion.y);    
    break;

  case Expose:            /* window just uncovered */
    redisplay();
    break;
  }
}

static void loop_canvas_handle_event(XEvent *event) {
    switch (event->type) {
	case KeyPress:          /* key pressed */
	case KeyRelease:        /* key released */
	    {
		char buf[10];
		int n;
		KeySym keysym;

		n = XLookupString(&event->xkey, buf, sizeof buf - 1,
		    &keysym, 0);
		buf[n] = 0;
		if (keysym=='q') exit(0);
	    }
	    break;

	case Expose:            /* window just uncovered */
	    loop_redisplay();
	    break;
    }
}

int main(int argc, char **argv) {
  /* Example Function calls to the Spline Functions.. 
     ctlpts *test = ReadCtlpts("test0.sp");
     if (test != NULL) {printf("Got Points.. \n"); PrintCP(test);}
     FreeCtlpts(test);
  */
  
  /* initialize form */
  fl_initialize(&argc, argv, "cyl", 0, 0);
  form = create_form_cyl();
  
  fl_set_button(form->norms_button, norms);
  fl_set_button(form->triad_button, triad);
  fl_set_button(form->skin_button, skin);
  
  fl_set_slider_value(form->loop_alpha_slider, 0.5);
  
  /* show the form */
  fl_show_form(
	       form->cyl,  	/* form */
	       FL_PLACE_SIZE,   /* pos & size flags */
	       FL_FULLBORDER,   /* border flags */
	       argv[0]          /* window name */
	       );
  
  /* bind OpenGL panes to display */
  glxf_bind_pane(&pane,
		 form->pane,
#ifdef HOME
		 0,             /* double buffer? */
#else
		 1,             /* double buffer? */
#endif
		 maincanvas_handle_event   /* event handler */
		 );
  glxf_bind_pane(&loop_canvas,
		 form->loop_canvas,
		 0,             /* double buffer? */
		 loop_canvas_handle_event   /* event handler */
		 );
  
  cball_init();
  pane_init();
  loop_canvas_init();
  light_init();	/* set up lights for surface display */
  tex_init("building.tiff");
  
  redisplay_all();	/* draw scene once */
  
  /* Xforms handles events until "exit" */
  while (fl_check_forms() != form->exit_button) {
    /* Calling the following routine occasionally can aid debugging.
     * It will print something only if there's been an OpenGL error
     * since it was last called. */
    error_check(__LINE__);
  }
  
  return 0;
}
