/*
 *  Box.h
 *  
 *
 *  Created by Erik Brunvand on 9/25/11.
 *
 *
 */
#ifndef __Box_h__
#define __Box_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Point3D.h"
#include "Ray.h"
#include "Normal.h" 
#include "HitRecord.h"

// The Box class
class Box {
private:             // public for better performance? 
  Point3D minpt;     // min corner of the box
  Point3D maxpt;     // max corner of the box
  int child;         // address of (left) child. Right child is child+1
  int num_prim;      // Number of primitives, or -1 for interior node
  float Tmin;        // distance to hit when intersected (like a HitRecord)
  bool HitP;         // true if the ray hit the box (like a HitRecord)
  bool inside;       // true if the ray started inside the box (hit was to the far side)
	
public: 
  // Constructors and destructors
  Box(void);		             // uninitialized constructor
  Box(const int& addr);              // init box from memory
  ~Box(void);	  		     // destructor (needed?)
  
  // return the elements of the Box (inlined)
  Point3D getMin() const {return minpt;} ;  // Min point
  Point3D getMax() const {return maxpt;} ;  // Max point
  int getChild() const{return child;} ;     // address of left child
  int getNum_Prim() const {return num_prim;} ; // number of primitives, or -1 for interior node
  bool wasInside() const {return inside;} ; // return the value of the "inside" bit
  float getTmin() const {return Tmin;} ;    // return the hit distance
  bool didHit() const {return HitP;} ;      // return true if the ray hit the BVH node
  void reset() {       // reset the HitRecord portion of the Box to defaults
    Tmin = FLOAT_MAX;  // set Tmin to infinity(ish)
    HitP = false;      // reset the hit flag
    inside = false;    // reset the "hit from inside the box" flag
  }
    
  // Member functions
  void intersects(const Ray& ray); //computes intersection in the BVH's internal hit record
  bool intersectP(const Ray& ray) const; // computes intersection and returns Boolean
  bool intersectPK(const Ray& ray) const; // computes intersection and returns Boolean
  Normal normal(const Point3D& hp); // computes and returns normal at hit point
  
}; // end Box class definition


#endif
