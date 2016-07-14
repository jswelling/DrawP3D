/*
 * TestProgram
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/MessageB.h>
#ifdef ultrix
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <signal.h>
#include "p3dgen.h"
#include "drawp3d.h"
#include "xdrawih.h"

#define BEGIN    1
#define IGNORE   2
#define NOTICE   3
#define END      4

static char* exename= NULL;

/* Flag to indicate what kind of renderers to use */
static int automanage= 0;

/* Which step of rotation are we on */
static int rot_step= 0;

/* Light structures */
static P_Point lightloc = {0.0, 20.0, 20.0};
static P_Color lightcolor = {P3D_RGB, 0.5, 0.5, 0.5, 1.0};
static P_Color ambcolor = {P3D_RGB, 0.3, 0.3, 0.3, 1.0};
static P_Color background = {P3D_RGB, 0.0, 0.0, 0.0, 1.0};

/* Camera info */
static P_Point froma = {0.0, 0.0, 28.0};
static P_Point toa   = {0.0, 0.0, 0.0};
static P_Vector upa  = {0.0, 1.0, 0.0};
static P_Point fromb = {3.0, 0.0, 18.0};
static P_Point tob   = {0.0, 0.0, 0.0};
static P_Vector upb  = {0.0, 2.0, 0.0};
static float foveaa  = 15.0;
static float foveab  = 15.0;
/*
static float foveaa  = 30.0;
static float foveab  = 45.0;
*/
static float hither  = -5.0;
static float yon     = -60.0; 

/* gob colors */
static P_Color red = {P3D_RGB, 1.0, 0.0, 0.0, 1.0};
static P_Color green = {P3D_RGB, 0.0, 1.0, 0.0, 1.0};
static P_Color blue = {P3D_RGB, 0.0, 0.0, 1.0, 1.0};
static P_Color purple = {P3D_RGB, 0.5, 0.0, 0.5, 1.0};

/* pline info */
static float coords1a[] = {1.0, 0.0, 0.0,   1.0, 1.0, 1.0, 1.0,
                           2.0, 0.0, 0.0,   1.0, 1.0, 1.0, 1.0};
static float coords1b[] = {-1.0, 0.0, 0.0,  1.0, 1.0, 1.0, 1.0,
                           -2.0, 0.0, 0.0,  1.0, 1.0, 1.0, 1.0};
static float coords2a[] = {0.0, 3.0, 0.0,   1.0, 1.0, 1.0, 1.0,
                           0.0, 4.0, 0.0,   1.0, 1.0, 1.0, 1.0};
static float coords2b[] = {0.0, -3.0, 0.0,  1.0, 1.0, 1.0, 1.0,
                           0.0, -4.0, 0.0,  1.0, 1.0, 1.0, 1.0};
static float coords3a[] = {5.0, 0.0, 0.0,   1.0, 1.0, 1.0, 1.0,
                           6.0, 0.0, 0.0,   1.0, 1.0, 1.0, 1.0};
static float coords3b[] = {-5.0, 0.0, 0.0,  1.0, 1.0, 1.0, 1.0,
                           -6.0, 0.0, 0.0,  1.0, 1.0, 1.0, 1.0};

/* axis about which to rotate */
static P_Vector x_axis = {1.0, 0.0, 0.0};
static P_Vector y_axis = {0.0, 1.0, 0.0};
static P_Vector z_axis = {0.0, 0.0, 1.0};

static void make_new_snaps()
{
  static float theta1 = 0.0;
  static float theta2 = 0.0;
  static float theta3 = 0.0;
  static float theta4 = 0.0;

  dp_open("sphere");
  dp_sphere();
  dp_rotate(&x_axis, theta4);
  dp_close();
  
  dp_open("inner");
  dp_rotate(&y_axis, theta3);
  dp_child("torus1");
  dp_child("sphere");
  dp_close();
  
  dp_open("middle");
  dp_rotate(&x_axis, theta2);
  dp_child("inner");
  dp_child("torus2");
  dp_close();
  
  dp_open("outer");
  dp_child("middle");
  dp_child("torus3");
  dp_close();
  
  dp_open("spin");
  dp_child("outer");
  dp_child("base");
  dp_close();
  
  dp_close_ren("myrenb");
  dp_snap("spin", "mylights", "mycama");
  dp_close_ren("myrena");
  dp_open_ren("myrenb");
  dp_snap("spin", "mylights", "mycamb");
  dp_open_ren("myrena");
  
  theta2 += 2.5;
  theta3 += 5.0;
  theta4 += 10.0;
}

static void finish_and_shutdown()
{
  fprintf(stderr, "%s: shutdown\n",exename);
  dp_shutdown();
  /* In the non-automanage case, should really destroy app context here */
  exit(0);
}

static Boolean my_work_proc( XtPointer junk )
{
  if (rot_step<180) {
    make_new_snaps();
    rot_step++;
    return False;
  }
  else {
    return True;
  }
}

static void exit_cb(Widget w, XtPointer data, XtPointer cb)
{
  finish_and_shutdown();
}

static void show_cb(Widget w, XtPointer data, XtPointer cb)
{
  fprintf(stderr, "%s: about to render\n",exename);
  rot_step= 0;
  XtAddWorkProc(my_work_proc,NULL);
}

main(int argc, char *argv[])
{
  int i;
  char r[30], s[10];
  char r2[30], s2[10];
  int width=300, height=300;

  exename= argv[0];

  if (argc>1) { /* Check for command line arguments */
    i= 1;
    while ((--argc > 0) && (argv[i][0]=='-')) {
      int c= argv[i][1];
      switch (c) {
      case 'a':
	automanage= 1;
	fprintf(stderr,"%s: Windows will be automanaged.\n",argv[0]);
	break;
      default:
	fprintf(stderr,"%s: illegal option %c\n",argv[0],c);
	fprintf(stderr,"usage: %s [-a]\n",argv[0]);
	argc= 0;
	break;
      }
      i++;
    }
  }

  if (!automanage) {
    Arg args[16]; 
    Widget toplevel, main_window, menu_bar, menu_pane, button;
    Widget draw_form, drawa, drawb;
    XmString title_string;

    i = 0;
    toplevel = XtInitialize("p3d", "xpainter", NULL, 0, &i, NULL);

    i= 0;
    main_window= XmCreateMainWindow(toplevel, "main_window", args, i);

    i= 0;
    menu_bar= XmCreateMenuBar(main_window, "menu_bar", args, i);
    XtManageChild(menu_bar);

    i= 0;
    menu_pane= XmCreatePulldownMenu(menu_bar, "menu_pane", args, i);

    title_string= XmStringCreateSimple("File");
    i= 0;
    XtSetArg(args[i], XmNsubMenuId, menu_pane); i++;
    XtSetArg(args[i], XmNlabelString, title_string); i++;
    button= XmCreateCascadeButton(menu_bar,"file",args,i);
    if (title_string) XmStringFree(title_string);
    XtManageChild(button);

    title_string= XmStringCreateSimple("Show");
    i= 0;
    XtSetArg(args[i], XmNlabelString, title_string); i++;
    button= XmCreatePushButtonGadget(menu_pane,"show",args,i);
    XtAddCallback(button,XmNactivateCallback,show_cb,NULL);
    if (title_string) XmStringFree(title_string);
    XtManageChild(button);

    title_string= XmStringCreateSimple("Exit");
    i= 0;
    XtSetArg(args[i], XmNlabelString, title_string); i++;
    button= XmCreatePushButtonGadget(menu_pane,"exit",args,i);
    XtAddCallback(button,XmNactivateCallback,exit_cb,NULL);
    if (title_string) XmStringFree(title_string);
    XtManageChild(button);

    i= 0;
    draw_form= XmCreateForm(main_window,"draw_form", args, i);
    XtManageChild(draw_form);

    i = 0;
    XtSetArg(args[i], XtNwidth, width); i++;
    XtSetArg(args[i], XtNheight, height); i++;
    XtSetArg(args[i], XmNtopAttachment, XmATTACH_FORM); i++;
    XtSetArg(args[i], XmNbottomAttachment, XmATTACH_FORM); i++;
    XtSetArg(args[i], XmNleftAttachment, XmATTACH_FORM); i++;
    drawa = XmCreateDrawingArea(draw_form, "spina", args, i);
    XtManageChild(drawa);
    sprintf(r, "widget=%ld", (long)drawa);
    sprintf(s, "%dx%d", width, height);
    
    i = 0;
    XtSetArg(args[i], XtNwidth, width); i++;
    XtSetArg(args[i], XtNheight, height); i++;
    XtSetArg(args[i], XmNtopAttachment, XmATTACH_FORM); i++;
    XtSetArg(args[i], XmNbottomAttachment, XmATTACH_FORM); i++;
    XtSetArg(args[i], XmNleftAttachment, XmATTACH_WIDGET); i++;
    XtSetArg(args[i], XmNleftWidget, drawa); i++;
    XtSetArg(args[i], XmNrightAttachment, XmATTACH_FORM); i++;
    drawb = XmCreateDrawingArea(draw_form, "spinb", args, i);
    XtManageChild(drawb);
    sprintf(r2, "widget=%ld", (long)drawb);
    sprintf(s2, "%dx%d", width, height);

    XtManageChild(main_window);

    XtRealizeWidget(toplevel);

#ifdef never
    dp_init_ren("myrena", "xpainter", r, s);
#endif
    dp_init_ren("myrena", "gl", r, s);
#ifdef never
    dp_init_ren("myrenb", "xpainter", r2, s2);
#endif
    dp_init_ren("myrenb", "gl", r2, s2);
  }
  else { /* automanage renderers */
#ifdef never
    dp_init_ren("myrena", "xpainter","automanage","");
    dp_init_ren("myrenb", "xpainter","automanage","");
#endif
    dp_init_ren("myrena", "gl","automanage","");
    dp_init_ren("myrenb", "gl","automanage","");
  }

  fprintf(stderr, "%s: renderer initialized\n",exename);

  dp_open("torus1");
  /*
    dp_torus(2.5, 0.5);
  */
    dp_polyline(P3D_CCVTX, P3D_RGB, coords1a, 2);
    dp_polyline(P3D_CCVTX, P3D_RGB, coords1b, 2);
    dp_gobcolor(&blue);
  dp_close();

  dp_open("torus2");
  /*
    dp_torus(4.5, 0.5);
  */
    dp_polyline(P3D_CCVTX, P3D_RGB, coords2a, 2);
    dp_polyline(P3D_CCVTX, P3D_RGB, coords2b, 2);
    dp_gobcolor(&red);
  dp_close();

  dp_open("torus3");
  /*
    dp_torus(6.5, 0.5);
  */
    dp_backcull(1);
    dp_polyline(P3D_CCVTX, P3D_RGB, coords3a, 2);
    dp_polyline(P3D_CCVTX, P3D_RGB, coords3b, 2);
    dp_gobcolor(&green);
  dp_close();

  dp_open("base");
    dp_cylinder();
    dp_backcull(1);
    dp_rotate(&x_axis, 90);
    dp_ascale(6.0, 0.5, 6.0);
    dp_translate(0.0, -7.5, 0.0);
  dp_close();

  dp_open("mylights");
    dp_ambient(&ambcolor);
    dp_light(&lightloc, &lightcolor);
  dp_close();

  dp_camera("mycama", &froma, &toa, &upa, foveaa, hither, yon);
  dp_camera_background("mycama", &background);

  dp_camera("mycamb", &fromb, &tob, &upb, foveab, hither, yon);
  dp_camera_background("mycamb", &background);

  if (!automanage) {
    XtMainLoop();
  }
  else {
    fprintf(stderr, "%s: about to render\n",exename);
    for (i=0; i<180; i++) make_new_snaps();
    finish_and_shutdown();
  }
}
