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
private:	     // public for better performance? 
  Point3D minpt;     // min corner of the box
  Point3D maxpt;     // max corner of the box
  int address;       // address of the box in memory
  int child;         // address of (left) child. Right child is child+4
  int num_prim;      // Number of primitives, or -1 for interior node
	
public: 
  // Constructors and destructors
  Box(void);		             // uninitialized constructor
  Box(const Point3D& minin, const Point3D& maxin, // init all elements
      const int& cin, const int& numin);  
  Box(const int& addr);              // init box from memory
  ~Box(void);	  		     // destructor (needed?)
  
  // return the elements of the Box (inlined)
  Point3D getMin() const {return minpt;} ;  // Min point
  Point3D getMax() const {return maxpt;} ;  // Max point
  int getChild() const{return child;} ;     // address of left child
  int getNum_Prim() const {return num_prim;} ; // number of primitives, or -1 for interior node
	int getObjId() const {return address;} ; // return address as ID
  
  // Member functions
  void intersects(const Ray& ray, HitRecord& hr); //computes intersection in hit record
  Normal normal(const Point3D& hp); // computes and returns normal at hit point
  
}; // end Box class definition

#endif
