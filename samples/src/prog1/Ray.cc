/*
 *  Ray.cpp
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#include "Ray.h"
#include "Vector3D.h"
#include "Point3D.h"

// constructors and destructors

// default constructor
Ray::Ray(void) 
: o(Point3D(0.f)), d(Vector3D(0.f)) {};

// construct with o and d
Ray::Ray(const Point3D& oin, const Vector3D& din) 
  : o(oin), d(din) {}

// copy constructor (needed?)
Ray::Ray(const Ray& r)
  : o(r.o), d(r.d) {}

// Destructor (needed?)
Ray::~Ray(void) {}

// Access functions (not really needed because components are public)

// return class data
Point3D Ray::geto() const {return o;}
Vector3D Ray::getd() const {return d;}
  
// Overloaded operators

// assignment
Ray& Ray::operator= (const Ray& r) {
  o=r.o; d=r.d;
  return *this;
}



       
