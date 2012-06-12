/*
 *  Tri.cc
 *  
 *
 *  Created by Erik Brunvand on 9/25/11.
 *
 *
 */

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "MyMath.h"
#include "Point3D.h"
#include "Ray.h"
#include "Normal.h" 
#include "HitRecord.h"
#include "Tri.h"

  // Constructors and destructors

// uninitialized constructor
Tri::Tri(void)
  : p1(Point3D(0,0,1)), p2(Point3D(0,1,0)), p3(Point3D(1,0,0)),
    ID(0), material(0), address(0) {
  e1 = p1 - p3;  // compute two edges that define the triangle
  e2 = p2 - p3;  // we'll need these for intersection later
}

// Init three points, ID, and material (e1, e2 are derived)
Tri::Tri(const Point3D& p1in, const Point3D& p2in, const Point3D& p3in,
      const int& IDin, const int& matin)
  : p1(p1in), p2(p2in), p3(p3in), ID(IDin), material(matin), address(0){
  e1 = p1 - p3;  // compute two edges that define the triangle
  e2 = p2 - p3;  // we'll need these for intersection later
}

// Init the triangle given an address in global mem
Tri::Tri(const int& addr) {
  address = addr;                // save the address of this triangle
  p1 = loadPtFromMem(addr);   // point p1 is the first three words after addr
  p2 = loadPtFromMem(addr+3); // Next points follow that one
  p3 = loadPtFromMem(addr+6);
  ID = loadi(addr+9);          // next is the ID (int) - offset from triangles_start
  material = loadi(addr+10);   // and the material ID - offset from material_start
  e1 = p1 - p3;                // compute two edges that define the triangle
  e2 = p2 - p3;                // we'll need these for intersection later
}

// destructor (needed?)
Tri::~Tri(void) {}	  		     
  
// Member functions

//computes intersection in hit record
// Use a Muller-Trumbore style triangle hit test (see slide 30 from 04.pdf)
// Note that e1 and e2 edges are pre-computed when the triangle is instantiated
void Tri::intersects(const Ray& ray, HitRecord& hr) {
  Vector3D r1 = Cross(ray.dir, e2); 
  float denom = Dot(e1, r1);        // take the dot product first, before inverting
  if (Abs(denom) < EPSILON) return; // Don't divide if it's too close to 0
  denom = 1.f/denom;                // invert it to be the denominator
  Vector3D s = ray.org - p3;        // translated ray origin
  float b1 = Dot(s, r1) * denom;    // first barycentric coordinate
  if (b1 < 0.f || b1 > 1.f) return; // b1 is outside the allowed range
  Vector3D r2 = Cross(s, e1);
  float b2 = Dot(ray.dir, r2) * denom; // second barycentric coord
  if (b2 < 0.f || (b1 + b2) > 1.f) return; // b2, and b1+b2 are outside allowed range
  float t = Dot(e2, r2) * denom;     // hit - compute the t value - use to check for closest hit
  hr.hit(t, address);                // check for closest hit - save address of triangle as ObjId if so
}

// computes and returns normal at hit point
Normal Tri::normal(const Point3D& hp) {
  norm = Cross(e1, e2); // Cross product of the two triangle edges
  norm.normalize();     // normalize the normal
  return norm;          // return the normal
}

