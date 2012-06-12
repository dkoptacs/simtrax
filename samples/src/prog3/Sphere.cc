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
#include "HitRecord.h"

// Constructors and destructors

// uninitialized constructor
Sphere::Sphere(void) 
  : Center(Point3D(0.f)), Radius(1.f), ID(0), Material(0) {}

// init all three elements
Sphere::Sphere(const Point3D& c, const float r, const int i, const int m )
  : Center(c), Radius(r), ID(i), Material(m) {}

// destructor (needed?)
Sphere::~Sphere(void) {}	  		    

// Member functions

// Sphere::intersects
// Doesn't return anything - updates the HitRecord argument
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
// c = (o-c) dot (o-c) - r_sq
//
// Solve this and check the sign of the discriminant
// dis = b_sq - 4ac
// to see if there are intersections.
// 
// If the discriminant is negative, there are no intersections
// If the discriminant is positive, there may be intersections
//
// In that case, send the roots (the t values) into the HitRecord and
// see if that T is a hit. If it is, it will update the HitRecord and
// return true. If not, the HitRecord will be unchanged and the .hit
// method will return false. 
//
// 
void Sphere::intersects(const Ray& ray, HitRecord& hr) {
  const Vector3D OC = ray.org - Center;      // ray origin minus sphere center (o-c)
  const Vector3D& DIR = ray.dir;             // local version of ray direction
  float a = Dot(DIR, DIR);                   // a = (dir dot dir); 
  float b = 2.f * Dot(OC, DIR);              // b = 2 ((o-c) dot dir)
  float c = Dot(OC, OC) - Radius*Radius;     // c = (o-c)dot(o-c) - r_sq
  float disc = b*b-4.0f*a*c;                 // disc = b_sq - 4ac
  if (disc > 0.0f) {  // negative discriminant means no intersection
    float disc_sqrt = sqrt(disc);            // take square root of discriminant
    float denom_inv = 1.0f / (2.0f * a);     // 1/2a (inverse of denominator)
    float root1 = (-b + disc_sqrt)*denom_inv;// first root: (-b + disc_sroot) * 1/2a
    float root2 = (-b - disc_sqrt)*denom_inv;// second root: (-b - disc_sroot) * 1/2a
    hr.hit(root1, ID);                       // Check both roots for closest hit
    hr.hit(root2, ID);     
  }
}


