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
#include "constants.h" 

// Constructors and destructors

// uninitialized constructor
Box::Box(void) 
  : minpt(Point3D(0,0,0)), maxpt(Point3D(1,1,1)), child(0), num_prim(-1),
    inside(false), HitP(false), Tmin(FLOAT_MAX){}

// init all elements
//Box::Box(const Point3D& minin, const Point3D& maxin, const int& cin, const int& numin, const bool& inin)
//  : minpt(minin), maxpt(maxin), child(cin), num_prim(numin), inside(inin) {}

// init box from memory
// min and max are the first two triples of a BVH node in memory
// words 7 and 8 (address 6 and 7 offset from 0) are child address and number
// of primitives
Box::Box(const int& addr)
  : minpt(loadPtFromMem(addr + MINPT)), maxpt(loadPtFromMem(addr + MAXPT)),
    child(loadi(addr + CHILD)), num_prim(loadi(addr + NUM_PRIM)),
    inside(false), HitP(false), Tmin(FLOAT_MAX) {}

// destructor (needed?)
Box::~Box(void) {}
  
  
// Member functions

// computes intersection in hit record
// This is based on Steve's box intersection code. I'm sure that there's a slightly faster
// version based on Brian Smit's code.
// Does this even need to use a HitRecord? Or is returning a Boolean good enough? 
void Box::intersects(const Ray& ray)  {
  Vector3D dir = ray.dir;                    // ray direction
  Vector3D inv_dir = Vector3D(1.f/dir.x, 1.f/dir.y, 1.f/dir.z); // 1/ray-direction
  Vector3D v1 = (minpt-ray.org) * inv_dir;
  Vector3D v2 = (maxpt-ray.org) * inv_dir;
  Vector3D vmin = MinComps(v1, v2);       // vector of min elements
  Vector3D vmax = MaxComps(v1, v2);       // vector of max elements
  float near = max(max(vmin.x, vmin.y), vmin.z); // the largest min-component
  float far = min(min(vmax.x, vmax.y), vmax.z);  // the smallest max-component
  if (near < far) {                       // check that we're on the right side of the box
    if (near < Tmin && near > EPSILON) {  // check near point for hit
      Tmin = near;                        // if hit - update Tmin to near
      HitP = true;                        // and set HitP boolean
    } else {                              // if didn't hit near point
      if (far < Tmin && far > EPSILON) {  // check far point
	Tmin = far;                       // if hit, update Tmin to far
	HitP = true;                      // and set HitP boolean
	inside = true;                    // This hit was from inside the box... 
      } // end far test
    }// end else (didn't hit near)
  }//end near<far
}// end intersects


// This version of the box test just returns a boolean that tells whether you hit or
// not. It's used for BVH traversal and not for rendering a box. It doesn't update a
// hit record with the nearest point - it just says whether you hit or not. 
bool Box::intersectP(const Ray& ray) const {
  Vector3D dir = ray.dir;                    // ray direction
  Vector3D inv_dir = Vector3D(1.f/dir.x, 1.f/dir.y, 1.f/dir.z); // 1/ray-direction
  Vector3D v1 = (minpt-ray.org) * inv_dir;
  Vector3D v2 = (maxpt-ray.org) * inv_dir;
  Vector3D vmin = MinComps(v1, v2);       // vector of min elements
  Vector3D vmax = MaxComps(v1, v2);       // vector of max elements
  float near = max(max(vmin.x, vmin.y), vmin.z); // the largest min-component
  float far = min(min(vmax.x, vmax.y), vmax.z);  // the smallest max-component
  return ((near < far) && (far > EPSILON)); 
}

// This version of the box intersection algorithm is based on Suffern's book, which
// in turn is based on the code from Shirley's paper (I think). It doesn't use a hit record.
// It just returns a Boolean if the ray hits the box. It's used for BVH traversal and
// not for rendering boxes.
// It also doesn't seem to work! THe image is all black when I use this one. I haven't
// looked in to fixing it yet... 
bool Box::intersectPK(const Ray& ray) const {
  float ox = ray.org.x;  // get the components of the ray separately 
  float oy = ray.org.y;
  float oz = ray.org.z;
  float dx = ray.dir.x;
  float dy = ray.dir.y;
  float dz = ray.dir.z;

  float tx_min, ty_min, tz_min; // get the min and max of the intersetcions with 
  float tx_max, ty_max, tz_max; // the "slabs" corresponding to the box faces

  float a = 1.f/dx;    // apparently this still works, even if dx==0?
  if (a >=0) {
    tx_min = (getMin().x - ox) * a;
    tx_max = (getMax().x - ox) * a;
  } else {
    tx_min = (getMax().x - ox) * a;
    tx_max = (getMin().x - ox) * a;
  }
  a = 1.f/dy;    // apparently this still works, even if dy==0?
  if (a >=0) {
    tx_min = (getMin().y - oy) * a;
    tx_max = (getMax().y - oy) * a;
  } else {
    tx_min = (getMax().y - oy) * a;
    tx_max = (getMin().y - oy) * a;
  }
  a = 1.f/dz;    // apparently this still works, even if dz==0?
  if (a >=0) {
    tx_min = (getMin().z - oz) * a;
    tx_max = (getMax().z - oz) * a;
  } else {
    tx_min = (getMax().z - oz) * a;
    tx_max = (getMin().z - oz) * a;
  }

  float t0, t1; // floats to hold overlap region
  // find largest entering t value (max of the mins)
  t0 = (tx_min > ty_min) ? tx_min : ty_min;
  if (tz_min > t0) t0 = tz_min;

  // find smallest exiting t value (min of the maxes)
  t1 = (tx_max < ty_max) ? tx_max : ty_max;
  if (tz_max < t1) t1 = tz_max;

  // return true if the regions overlap (i.e. a hit)
  return ((t0 < t1) && t1 > EPSILON); 
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


  
