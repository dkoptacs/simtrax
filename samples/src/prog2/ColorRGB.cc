/*
 *  ColorRGB.cpp
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 * All the method code for the ColorRGB class.
 *
 *
 */
#include "trax.hpp"
#include "ColorRGB.h"

// uninitialized constructor - init color to white
ColorRGB::ColorRGB(void)		            
  : r(1.f), g(1.f), b(1.f) {}

// Init all three elements to the same float - i.e. gray
ColorRGB::ColorRGB(float a)		     
  : r(a), g(a), b(a) {}

// init all three elements
ColorRGB::ColorRGB(float rin, float gin, float bin) 
  : r(rin), g(gin), b(bin) {}

// copy constructor (needed?) 
ColorRGB::ColorRGB(const ColorRGB& c)
  : r(c.r), g(c.g), b(c.b) {}

// destructor (needed?)
ColorRGB::~ColorRGB(void) {}

// return the elements of the color
// Note that because I made the ColorRGB components public, you
// don't really need to use these functions... 
float ColorRGB::getr() const {return r;};	// r component
float ColorRGB::getg() const {return g;};	// g component
float ColorRGB::getb() const {return b;};	// b component

// define overloaded operators on ColorRGB class

// add two ColorRGBs
ColorRGB ColorRGB::operator+ (const ColorRGB& c) const {
  return ColorRGB(r+c.r, g+c.g, b+c.b);
}

// add to current ColorRGB
ColorRGB& ColorRGB::operator+= (const ColorRGB& c){     
  r+=c.r; g+=c.g; b+=c.b;
  return *this;
}

// subtract two ColorRGBs
ColorRGB ColorRGB::operator- (const ColorRGB& c) const {
  return ColorRGB(r-c.r, g-c.g, b-c.b);
}

// negate the ColorRGB
ColorRGB ColorRGB::operator- () const {
  return ColorRGB(-r, -g, -b);
}

// sub from current ColorRGB
ColorRGB& ColorRGB::operator-= (const ColorRGB& c){
  r-=c.r; g-=c.g; b-=c.b;
  return *this;
}

// Multiply two ColorRGBs
ColorRGB ColorRGB::operator* (const ColorRGB& c) {
  return ColorRGB(r*c.r, g*c.g, b*c.b);
}

// multiply by a float
ColorRGB ColorRGB::operator* (const float a) const {
  return ColorRGB(r*a, g*a, b*a);
}

// multiply current ColorRGB by a color
ColorRGB& ColorRGB::operator*= (const ColorRGB& c){
  r*=c.r; g*=c.g; b*=c.b;
  return *this;
}

// multiply current ColorRGB by a float
ColorRGB& ColorRGB::operator*= (const float a){
  r*=a; g*=a; b*=a;
  return *this;
}

// divide by a float
ColorRGB ColorRGB::operator/ (const float a) const {
  float inv_a = 1.f/a;
  return ColorRGB(r*inv_a, g*inv_a, b*inv_a);
}

// div current ColorRGB by a float
ColorRGB& ColorRGB::operator/= (const float a){
  float inv_a = 1.f/a;
  r*=inv_a; g*=inv_a; b*=inv_a;
  return *this;
}
  
// assignment
ColorRGB& ColorRGB::operator= (const ColorRGB& c){
  r=c.r; g=c.g; b=c.b;
  return *this;
}

// Comparison
bool ColorRGB::operator== (const ColorRGB& c) const {
  return (r==c.r && g==c.g && b==c.b);
}

// Functions that return info about the color

// Return ColorRGB average
float ColorRGB::average() const {
  return (0.333333333333f * (r + g + b));
}

//
//---------------- non member functions related to ColorRGB--------------
//
// Note that because the x, y, and z of ColorRGB are public, you don't need
// to call the access function to get the components... 

// Multiply ColorRGB by constant on the left
ColorRGB operator* (const float a, const ColorRGB& c) {
  return ColorRGB(a*c.r, a*c.g, a*c.b);
}

