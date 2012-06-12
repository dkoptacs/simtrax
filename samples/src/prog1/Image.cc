/*
 *  Image.cpp
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Image.h"
#include "ColorRGB.h"

// Constructors and destructors

// uninitialized constructor
// This uses the functions from trax.hpp to get the xres, yres, and 
// fb-starting-address from the trax-specific memory locations 
Image::Image(void) 
  : xres(GetXRes()), yres(GetYRes()), start_fb(GetFrameBuffer()) {}

// init all three elements to specific values
Image::Image(const int xresin, const int yresin, const int fbin )
    : xres(xresin), yres(yresin), start_fb(fbin) {}

// destructor (needed?)
Image::~Image(void) {}

  
// return the elements of the Image
int Image::getxres() const {return xres;}   // xres component
int Image::getyres() const {return yres;}	// yres component
int Image::getfb() const {return start_fb;}	// start_fb component
  
// Member functions that modify the Image

// set the (x,y) pixel to Color c
void Image::set (const int x, const int y, const ColorRGB& cin) {
  
  ColorRGB c;  // local color

  // Compute the pixel number by indexing into the 2d array defined by
  // xres and yres. Start with the start_fb address. To that, add y*xres for
  // walking down the array to find the right row, then add x to get into
  // that row. Finally, multiply that by 3 because there are three values 
  //(rgb)for each pixel. 
  const int pixel = start_fb + (y*xres + x) * 3;
	
  // Clamp the cin color values to be between 0 and 1 for the local c
  c.r = (cin.r < 0.f) ? 0.f : (cin.r > 1.f) ? 1.f : cin.r;
  c.g = (cin.g < 0.f) ? 0.f : (cin.g > 1.f) ? 1.f : cin.g;
  c.b = (cin.b < 0.f) ? 0.f : (cin.b > 1.f) ? 1.f : cin.b; 

  // Now set the clamped values into the frame buffer. The third argument
  // is the offset from the Pixel starting location for each of the r, g, b
  // colors. 
  storef(c.r, pixel, 0);
  storef(c.g, pixel, 1);
  storef(c.b, pixel, 2);
}

 
  
  
       

      
      
	    
	    
	    
    
    
	    
  
  
