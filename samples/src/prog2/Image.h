/*
 *  Image.h
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#ifndef __Image_h__
#define __Image_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "ColorRGB.h"

// The Image class
class Image {
private:		// public for better performance? 
  int xres, yres, start_fb; 
	
public: 
  // Constructors and destructors
  Image(void);		                    // uninitialized constructor
  Image(const int xresin, const int yresin, const int fbin ); // init all three elements
  ~Image(void);	  		            // destructor (needed?)
  
  // return the elements of the Image
  int getxres() const;			// xres component
  int getyres() const;			// yres component
  int getfb() const;			// start_fb component
  
  // Member functions that modify the Image
  void set (const int x, const int y, const ColorRGB& c); // set the (x,y) pixel to Color c
  
}; // end Image class definition

#endif
