/*
 *  Sphere.h
 *  
 *
 *  Created by Erik Brunvand on 9/5/11.
 *
 * Modified on 9/10/11 to include HitRecord in the Intersection method
 *
 */
#ifndef __Sphere_h__
#define __Sphere_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Point3D.h"
#include "Ray.h"
#include "Vector3D.h"
#include "Normal.h" 
#include "HitRecord.h"

// The Sphere class
class Sphere {
private:	     // public for better performance? 
  Point3D Center;    // Center of the sphere
  float Radius;      // radius
  int ID;            // ID of this sphere object
  int Material;      // ID of the material this sphere is made of
	
public: 
  // Constructors and destructors
  Sphere(void);		                     // uninitialized constructor
  Sphere(const Point3D& c, const float r, const int i, const int m);  // init all elements
  ~Sphere(void);	  		     // destructor (needed?)
  
  // return the elements of the Sphere (inlined)
  Point3D getCenter() const {return Center;} ;  // center component
  float getRadius() const {return Radius;} ;    // radius component
  int getID() const {return ID;} ;              // ID component
  int getMat() const {return Material;};        // material ID

  // Modify the elements of the Sphere (inlined)
  void setCenter(const Point3D& c) {Center = c;};
  void setCenter(const float xin, const float yin, const float zin){
    Center.x = xin;
    Center.y = yin;
    Center.z = zin;
  }
  void setRadius(const float r) {Radius = r;};
  void setID(const int i) {ID = i;};
  void setMaterial(const int m) {Material = m;}; 
  
  // Member functions
  void intersects(const Ray& ray, HitRecord& hr); //computes intersection in hit record
  Normal normal(const Point3D& hp); // computes and returns normal at hit point
  
}; // end Sphere class definition

// inline member function definitions

// Compute normal by subtracting the center from the hitpoint to find direction.
// This could be normalized by calling .normalize on the new Normal, but we
// can also just divide by the Radius since we know that this new Normal
// must have length equal to the radius. 
inline Normal Sphere::normal(const Point3D& hp) {
  Normal norm(hp - Center);          // make new Normal from center to hitpoint
  return norm/Radius;                // return the normalized version
}

#endif
