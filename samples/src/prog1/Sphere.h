/*
 *  Sphere.h
 *  
 *
 *  Created by Erik Brunvand on 9/5/11.
 *
 */
#ifndef __Sphere_h__
#define __Sphere_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Point3D.h"
#include "Ray.h"
#include "Vector3D.h"

// The Sphere class
class Sphere {
private:		     // public for better performance? 
  Point3D center;    // Center of the sphere
  float radius;      // radius
	
public: 
  // Constructors and destructors
  Sphere(void);		                         // uninitialized constructor
  Sphere(const Point3D& c, const float r );  // init all three elements
  ~Sphere(void);	  		                 // destructor (needed?)
  
  // return the elements of the Sphere
  Point3D getcenter() const;		// center component
  float getradius() const;			// radius component
  
  // Member functions
  bool intersects(const Ray& r);    // returns true if ray intersects sphere
  
}; // end Sphere class definition

#endif
