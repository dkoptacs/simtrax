
#include "Image.h"
#include "trax.hpp"

Image::Image(int xres, int yres, int s_fb)
: xres(xres), yres(yres), start_fb(s_fb)
{
}

// 'i' is the major index in to the image array (row)
// 'j' is the minor index in to the image array (col)
// Framebuffer holds a full float for each color channel (r, g, b),
// instead of the usual char
void Image::set(int i, int j, const Color& c) 
{
  Color col;
  // clamp the color to [0..1]
  col.R = c.r() < 0.f? 0.f: c.r() >= 1.f? 1.f : c.r();
  col.G = c.g() < 0.f? 0.f: c.g() >= 1.f? 1.f : c.g();
  col.B = c.b() < 0.f? 0.f: c.b() >= 1.f? 1.f : c.b();
  
  // Translate (i, j) 2-D coordinates in to array address,
  //   and store the rgb values to main memory
  // "storef" intrinsic stores a float to main memory
  // storef(value_to_write, location to write, offset (immediate))
  storef(col.r(), start_fb + ( i * xres + j ) * 3, 0 );
  storef(col.g(), start_fb + ( i * xres + j ) * 3, 1 );
  storef(col.b(), start_fb + ( i * xres + j ) * 3, 2 );
}





