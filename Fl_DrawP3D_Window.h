#ifndef INCL_FL_DRAWP3D_WINDOW
#define INCL_FL_DRAWP3D_WINDOW

class Fl_DrawP3D_Window: public Fl_Gl_Window {

  int initialized;
  static int count;

public:
  Fl_DrawP3D_Window(int X, int Y, int W, int H, const char *L);
  void draw();
};

#endif
