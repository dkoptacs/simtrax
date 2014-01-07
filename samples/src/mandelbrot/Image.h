
#ifndef IMAGE_H
#define IMAGE_H

#include "Color.h"
#include "trax.hpp"

// Helper class to handle writes to the frame buffer

class Image {
 public:
  Image(int xres, int yres, int start_fb);

  void set(int i, int j, const Color& c);

 protected:
  int xres, yres, start_fb;
};

#endif
