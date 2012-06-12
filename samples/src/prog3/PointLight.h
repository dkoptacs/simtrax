/*
 *  PointLight.h
 *  
 *
 *  Created by Erik Brunvand on 9/10/11.
 *
 */
#ifndef __PointLight_h__
#define __PointLight_h__

#include "trax.hpp"    // trax-specific stuff
#include "constants.h" // my constants
#include "Point3D.h"
#include "ColorRGB.h"
#include "Vector3D.h" 

// The PointLight class
class PointLight {
private:                // public for better performance? 
  Point3D position;     // location of the point light
  ColorRGB color;       // Color of the light
	
public: 
  // Constructors and destructors
  PointLight(void);		        // uninitialized constructor
  PointLight(const Point3D& p, const ColorRGB& c); // init all components
  PointLight(const PointLight& p);      // copy constructor 
  ~PointLight(void);			// destructor
  
  // return the elements of the vector - inlined
  Point3D getPos() const {return position;};   // ObjId component
  ColorRGB getColor() const {return color;};   // Tmin component
  
  // Functions that return information about the PointLight
  // Return the distance to the light from the hitpoint, and update light color
  float getLight(ColorRGB& light_color, Vector3D& light_dir, Point3D& hitPoint) const; 
  
}; // end PointLight class definition

// Inlined member functions for this class

// Return the distance to the light from the hitpoint, and update light color and direction
inline float PointLight::getLight(ColorRGB& light_color, Vector3D& light_dir,
				  Point3D& hitPoint) const {
  light_color = color;                // update to the color of this light
  Vector3D dir = position - hitPoint; // direction to the light from the hitPoint
  float distance = dir.normalize();   // normalize the direction (returns old length)
  light_dir = dir;                    // update the direction to the light
  return distance;                    // return distance to the light

}
// prototypes for non-member functions defined in PointLight.cc

// None for this class... 

#endif
