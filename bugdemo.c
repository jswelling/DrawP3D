/*****************
 * Compile this with 
 * cc -I/usr/tmp/libaux -L/usr/tmp/libaux bugdemo.c -laux -lGLU -lGL -lX11 -lm
 *
 */

#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "aux.h" 
#include "sphere.h"

#define major_divisions 8
#define minor_divisions 8
#define pi 3.14159265
#define torus_vertices major_divisions*minor_divisions
#define torus_facets major_divisions*minor_divisions

static int mode= 0;

static GLfloat theta= 0.0;

static GLuint sphere_list= 0;
static GLuint torus_list= 0;
static int lists_ready= 0;

void mysphere()
{
  int i;

  glPushMatrix();
  glScalef(0.5,0.5,0.5);
  glBegin(GL_TRIANGLES);

  for (i=0; i<3*sphere_facets; i++) {
    glNormal3fv(sphere_coords[sphere_connect[i]]);
    glVertex3fv(sphere_coords[sphere_connect[i]]);
  }

  glEnd();
  glPopMatrix();
}

void mytorus(float major, float minor)
{
  float coords[torus_vertices][3];
  float norms[torus_vertices][3];
  float theta= 0.0, dt= 2.0*pi/minor_divisions;
  float phi=0.0, dp= 2.0*pi/major_divisions;
  int iloop, jloop, ip, jp, vcount;

  /* Calculate vertex positions */
  vcount= 0;
  for (iloop=0; iloop<major_divisions; iloop++) {
    for (jloop=0; jloop<minor_divisions; jloop++) {
      coords[ vcount ][0]=
        ( major + minor*cos(theta) ) * cos(phi);
      coords[ vcount ][1]=
        ( major + minor*cos(theta) ) * sin(phi);
      coords[ vcount ][2]= minor * sin(theta);
      norms[ vcount ][0]= cos(theta)*cos(phi);
      norms[ vcount ][1]= cos(theta)*sin(phi);
      norms[ vcount ][2]= sin(theta);
      vcount++;
      theta += dt;
    }
    phi += dp;
  }

  /* Generate the connectivity array */
  for (iloop=0; iloop<major_divisions; iloop++)
    for (jloop=0; jloop<minor_divisions; jloop++) {
      ip= (iloop+1) % major_divisions;
      jp= (jloop+1) % minor_divisions;
      glBegin(GL_QUADS);
      glNormal3fv(norms[jp+iloop*minor_divisions]);
      glVertex3fv(coords[jp+iloop*minor_divisions]);
      glNormal3fv(norms[jloop+iloop*minor_divisions]);
      glVertex3fv(coords[jloop+iloop*minor_divisions]);
      glNormal3fv(norms[jloop+ip*minor_divisions]);
      glVertex3fv(coords[jloop+ip*minor_divisions]);
      glNormal3fv(norms[jp+ip*minor_divisions]);
      glVertex3fv(coords[jp+ip*minor_divisions]);
      glNormal3fv(norms[jp+iloop*minor_divisions]);
      glVertex3fv(coords[jp+iloop*minor_divisions]);
      glEnd();
    }

}

void myinit(void)
{
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 }; 
    GLfloat light_ambient[] = { 0.8, 0.8, 0.8, 1.0 };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position); 
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
} 
void display(void)
{
  if (!lists_ready) {
    glNewList(sphere_list=glGenLists(1),GL_COMPILE);
    mysphere();
    glEndList();
    glNewList(torus_list= glGenLists(1),GL_COMPILE);
    mytorus(1.0,0.25);
    glEndList();
    lists_ready= 1;
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  if (mode) {
    mysphere();
    glRotatef(theta, 1.0, 0.0, 0.0);
    mytorus(1.0,0.25);
  }
  else {
    glCallList(sphere_list);
    glRotatef(theta, 1.0, 0.0, 0.0);
    glCallList(torus_list);
  }
  glPopMatrix();
  glFlush();
  auxSwapBuffers();
  theta;
} 
void idlefunc()
{
  display();
  theta += 1.5;
}
void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h) 
        glOrtho (-1.5, 1.5, -1.5*(GLfloat)h/(GLfloat)w, 
            1.5*(GLfloat)h/(GLfloat)w, -10.0, 10.0);
    else 
        glOrtho (-1.5*(GLfloat)w/(GLfloat)h, 
            1.5*(GLfloat)w/(GLfloat)h, -1.5, 1.5, -10.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}  
int main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr,"%s: usage: %s [0|1]\n",argv[0],argv[0]);
    fprintf(stderr,"%s:        %s 0 shows bug\n",argv[0]);
    fprintf(stderr,"%s:        %s 1 shows correct result\n",argv[0]);
    exit(-1);
  }

  mode= atoi(argv[1]);

  auxInitDisplayMode (AUX_DOUBLE | AUX_RGBA | AUX_DEPTH);
  auxInitPosition (0, 0, 500, 500);
  auxInitWindow (argv[0]);
  myinit();
  auxReshapeFunc (myReshape);
  auxIdleFunc(idlefunc);
  auxMainLoop(display);
}
