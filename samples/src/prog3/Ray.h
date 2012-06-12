/*
 *  Ray.h
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#ifndef __Ray_h__
#define __Ray_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Vector3D.h"
#include "Point3D.h"

class Ray {
public:           // Public for better performance?
  Point3D org;    // Origin is a point
  Vector3D dir;   // direction is a vector

public:
  // constructors and destructors
  Ray(void);                                  // default constructor
  Ray(const Point3D& oin, const Vector3D& din); // construct with o and d
  Ray(const Ray& r);                          // copy constructor
  ~Ray(void);

  // Access functions (not really needed because components are public)
  Point3D getorg() const;             // return origin
  Vector3D getdir() const;            // return direction
  

  // Overloaded operators
  Ray& operator= (const Ray& r);         // assignment

}; // end Ray class definition

#endif

       
