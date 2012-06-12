/*
 *  ColorRGB.h
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#ifndef __ColorRGB_h__
#define __ColorRGB_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...


// The ColorRGB class
class ColorRGB {
public:		        // public for better performance? 
  float r, g, b;	// The three elements of the vector
	
public: 
  // Constructors and destructors
  ColorRGB(void);		            // uninitialized constructor
  ColorRGB(float a);			    // Init all three colors to the same float
  ColorRGB(float rin, float gin, float bin); // init all three elements
  ColorRGB(const ColorRGB& c);	            // copy constructor (needed?) 
  ~ColorRGB(void);			    // destructor (needed?)
  
  // return the elements of the color
  // Note that because the components are public, you don't really need
  // to use these access functions
  float getr() const;			// r component
  float getg() const;			// g component
  float getb() const;			// b component
  
  // define overloaded operators on ColorRGB class
  ColorRGB operator+ (const ColorRGB& c) const;	// add two ColorRGBs
  ColorRGB& operator+= (const ColorRGB& c);	// add to current ColorRGB
  
  ColorRGB operator- (const ColorRGB& c) const;   // subtract two ColorRGBs
  ColorRGB operator- () const;		        // negate the ColorRGB
  ColorRGB& operator-= (const ColorRGB& c);	// sub from current ColorRGB

  ColorRGB operator* (const ColorRGB& c);        // multiply two ColorRGBs
  ColorRGB operator* (const float a) const;	 // multiply by a float
  ColorRGB& operator*= (const ColorRGB& c);       // multiply current color by a color
  ColorRGB& operator*= (const float a);          // multiply current ColorRGB by a float
  
  ColorRGB operator/ (const float a) const;      // divide by a float
  ColorRGB& operator/= (const float a);		// div current ColorRGB by a float
  
  ColorRGB& operator= (const ColorRGB& c);	// assignment
  bool operator== (const ColorRGB& c) const;    // Comparison - are two colors equal?
  
  // Functions that return info about the color
  float average () const;                       // average the color
  
}; // end ColorRGB class definition

#endif
