/*
 *  Box.cc
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
#include "Box.h"

// Constructors and destructors

// uninitialized constructor
Box::Box(void) 
  : minpt(Point3D(0,0,0)), maxpt(Point3D(1,1,1)), child(0), num_prim(-1), address(0) {}

// init all elements
Box::Box(const Point3D& minin, const Point3D& maxin, const int& cin, const int& numin)
  : minpt(minin), maxpt(maxin), child(cin), num_prim(numin), address(0) {}

// init box from memory
// min and max are the first two triples of a BVH node in memory
// words 7 and 8 (address 6 and 7 offset from 0) are child address and number
// of primitives
Box::Box(const int& addr)
  : minpt(loadPtFromMem(addr)), maxpt(loadPtFromMem(addr + 3)),
    child(loadi(addr + 6)), num_prim(loadi(addr + 7)), address(addr) {}

// destructor (needed?)
Box::~Box(void) {}
  
  
// Member functions

//computes intersection in hit record
void Box::intersects(const Ray& ray, HitRecord& hr)  {
  Vector3D dir = ray.dir;                    // ray direction
  Vector3D inv_dir = Vector3D(1.f/dir.x, 1.f/dir.y, 1.f/dir.z); // 1/ray-direction
  Vector3D v1 = (minpt-ray.org) * inv_dir;
  Vector3D v2 = (maxpt-ray.org) * inv_dir;
  Vector3D vmin = MinComps(v1, v2);       // vector of min elements
  Vector3D vmax = MaxComps(v1, v2);       // vector of max elements
  float near = max(max(vmin.x, vmin.y), vmin.z); // the largest min-component
  float far = min(min(vmax.x, vmax.y), vmax.z);  // the smallest max-component
  //if (near < far) {                       // potential hit - commented out so inside-box is still hit
    if (!hr.hit(near, address)) {         // check near first
      hr.hit(far, address);               // If it doesn't hit, check far
    }
  //}
}

// computes and returns normal at hit point. Note that because
// this box is axis-aligned, the normal will be aligned to the
// axes too. (see slides 09.pdf)
Normal Box::normal(const Point3D& hp) {
  if(Abs(hp.x-minpt.x) < EPSILON)
    return Normal(-1.f,0.f,0.f);
  else if(Abs(hp.x-maxpt.x) < EPSILON)
    return Normal(1.f,0.f,0.f);
  else if(Abs(hp.y-minpt.y) < EPSILON)
    return Normal(0.f,-1.f,0.f);
  else if(Abs(hp.y-maxpt.y) < EPSILON)
    return Normal(0.f,1.f,0.f);
  else if(Abs(hp.z-minpt.z) < EPSILON)
    return Normal(0.f,0.f,-1.f);
  else
    return Normal(0.f,0.f,1.f);
}
  
