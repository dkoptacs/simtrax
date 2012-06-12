/*
 *  Tri.h
 *  
 *
 *  Created by Erik Brunvand on 9/25/11.
 *
 *
 */
#ifndef __Tri_h__
#define __Tri_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Point3D.h"
#include "Ray.h"
#include "Normal.h" 
#include "HitRecord.h"

// The Tri class
class Tri {
private:	      // public for better performance? 
  Point3D p1, p2, p3; // Points that define the triangle
  Vector3D e1, e2;    // Edges defined by those points
  Normal norm;        // Normal for this triangle
  int ID;             // ID of this triangle - offset from start_triangles
  int address;        // the global memory address of this triangle
  int material;       // material ID for this triangle (offset in material memory)
    	
public: 
  // Constructors and destructors
  Tri(void);		                  // uninitialized constructor
  Tri(const Point3D& p1in, const Point3D& p2in, const Point3D& p3in,// Init three points,
      const int& IDin, const int& matin); //ID, and material (e1, e2 are derived)
  Tri(const int& addr);                   // Init the triangle given an address in global mem
  ~Tri(void);	  	 	          // destructor (needed?)
  
  // return the elements of the Tri (inlined)
  Point3D getP1() const {return p1;} ;  // p1 point
  Point3D getP2() const {return p2;} ;  // p2 point
  Point3D getP3() const {return p3;} ;  // p3 point
  Vector3D getE1() const {return e1;};  // e1 edge (p1-p3)
  Vector3D getE2() const {return e2;};  // e1 edge (p2-p3)
  int getID() const {return ID;}        // triangle ID
  int getAddr() const {return address;}; // triangle address in global memory
  int getMaterial() const {return material;}; // material ID - offset into material memory

  
  // Member functions
  void intersects(const Ray& ray, HitRecord& hr); //computes intersection in hit record
  Normal normal(const Point3D& hp); // computes and returns normal at hit point
  
}; // end Tri class definition

#endif
