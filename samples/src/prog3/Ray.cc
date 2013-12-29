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
: org(Point3D(0.f)), dir(Vector3D(0.f)) {};

// construct with o and d
Ray::Ray(const Point3D& oin, const Vector3D& din) 
  : org(oin), dir(din) {}

// copy constructor (needed?)
Ray::Ray(const Ray& r)
  : org(r.org), dir(r.dir) {}

// Destructor (needed?)
Ray::~Ray(void) {}

// Access functions (not really needed because components are public)

// return class data
Point3D Ray::getorg() const {return org;}
Vector3D Ray::getdir() const {return dir;}
  
// Overloaded operators

// assignment
Ray& Ray::operator= (const Ray& r) {
  org=r.org; dir=r.dir;
  return *this;
}



       