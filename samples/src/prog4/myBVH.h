/*
 *  myBVH.h
 *  
 *
 *  Created by Erik Brunvand on 10/01/11.
 *
 *
 */
#ifndef __myBVH_h__
#define __myBVH_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Ray.h"
#include "HitRecord.h"

// The myBVH class
class myBVH {
private:	     // public for better performance? 
  int start_bvh;     // address of the root myBVH node
	
public: 
  // Constructors and destructors
  myBVH(void);		             // uninitialized constructor
  myBVH(const int& startin);           // init with a specific starting address
  ~myBVH(void);	  		     // destructor (needed?)
  
  // return the elements of the myBVH (inlined)
  int getAddr() const {return start_bvh;} ;  // myBVH starting address
  
  // Member functions

  // Walk the tree, return hit in hit record - O is optimized version
  void walk(const Ray& ray, HitRecord& hr, const int& start_tris);

  // walk the tree for a shadow - return after first hit that's less
  // than the light distance (ldist is pre-loaded in HitRecord)
  // O is optimized version
  void walkS(const Ray& ray, HitRecord& hr, const int& start_tris);
  
}; // end myBVH class definition

#endif
