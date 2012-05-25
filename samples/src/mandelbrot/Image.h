
#ifndef IMAGE_H
#define IMAGE_H

#include "Color.h"
#include "trax.hpp"

// Helper class to handle writes to the frame buffer

class Image {
 public:
  Image(int xres, int yres, int start_fb);

  // somewhat confusing since x and y are backwards from xres and yres
  // 'x' is the major index in to the image array (row)
  // 'y' is the minor index in to the image array (col)
  void set(int x, int y, const Color& c) {
    Color col;
    col.R = c.r() < 0.f? 0.f: c.r() >= 1.f? 1.f : c.r();
    col.G = c.g() < 0.f? 0.f: c.g() >= 1.f? 1.f : c.g();
    col.B = c.b() < 0.f? 0.f: c.b() >= 1.f? 1.f : c.b();
    storef(col.r(), start_fb + ( x * xres + y ) * 3, 0 );
    storef(col.g(), start_fb + ( x * xres + y ) * 3, 1 );
    storef(col.b(), start_fb + ( x * xres + y ) * 3, 2 );
  }
  float aspect_ratio() const {
    return float(xres)/float(yres);
  }
  int getXresolution() {
    return xres;
  }
  int getYresolution() {
    return yres;
  }
 protected:
  int xres, yres, start_fb;
};



Image::Image(int xres, int yres, int s_fb)
: xres(xres), yres(yres), start_fb(s_fb)
{
}


#endif
