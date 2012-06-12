/*
 *  Sphere.cpp
 *  
 *
 *  Created by Erik Brunvand on 9/5/11.
 *
 */
#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Sphere.h"
#include "Vector3D.h"
#include "Point3D.h"

// Constructors and destructors

// uninitialized constructor
Sphere::Sphere(void) 
  : center(Point3D(0.f)), radius(1.f) {}

// init all three elements
Sphere::Sphere(const Point3D& c, const float r )
  : center(c), radius(r) {}

// destructor (needed?)
Sphere::~Sphere(void) {}	  		    

// return the elements of the Sphere

// Get class components
Point3D Sphere::getcenter() const {return center;}
float Sphere::getradius() const {return radius;}
  
// Member functions

// Sphere::intersects
// Returns true if ray intersects sphere.
//
// The ray we're testing against has an origin o and direction d.
// We start with the equation for a sphere as:
// (p-c) dot (p-c) -r_sq = 0
// where p is a point on the sphere and c is the center of the sphere.
// 
// A point on the ray is p = o + td. So, substitute that equation for
// a point on the ray into the sphere equation and solve for t.
//
// You end up with a quadratic equation at2 + bt + c where:
// a = d dot d
// b = 2(o-c) dot d
// c = (o-c) dot (o-c) -r_sq
//
// Solve this and check the sign of the discriminant
// dis = b_sq - 4ac
// to see if there are intersections.
// 
// If the discriminant is negative, there are no intersections
// If the discriminant is positive, there may be intersections - but only
// if there are positive roots when you solve for t. 
//
// Note that this intersection function doesn't return the hit point, it only
// returns true or false as to whether there was a hit!
// 
bool Sphere::intersects(const Ray& r) {
  const Vector3D tempV = r.o - center; // Ray origin minus sphere center (o-c)
  float a = Dot(r.d, r.d);             // a =  (d dot d)
  float b = 2.0f * Dot(tempV, r.d);    // b = 2((o-c) dot d)
  float c = Dot(tempV, tempV) - (radius * radius); // c = (o-c)dot(o-c)-r_sq
  float disc = (b*b) - 4.0f * a * c;   // Discriminant b_sq - 4ac

  if (disc < 0.0f)   // negative discriminant means no intersection
    return false;
  else {
    float disc_sroot = sqrt(disc); // take square root of discriminant
    float denom_inv = 1.0f / (2.0f * a); // 1/2a (inverse of denominator)
    float root1 = (-b + disc_sroot)*denom_inv; // (-b + disc_sroot) * 1/2a
    float root2 = (-b - disc_sroot)*denom_inv; // (-b - disc_sroot) * 1/2a

    if ((root1 > 0.0f) || (root2 > 0.0f)) return true;
    else return false;
  }
}
