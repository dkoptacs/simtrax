/*
 *  PinholeCamera.h
 *  
 *
 *  Created by Erik Brunvand on 9/11/11.
 *
 */
#ifndef __PinholeCamera_h__
#define __PinholeCamera_h__

#include "trax.hpp"    // trax-specific stuff
#include "Point3D.h"
#include "Vector3D.h"
#include "Ray.h"
#include "constants.h" // my constants

// The PinholeCamera class
class PinholeCamera {
private:                // public for better performance? 
  Point3D eye;          // Eye point
  Point3D lookat;       // Look-at point - where you're looking
  Vector3D up;          // Up vector defines camera orientation
  float u_len;          // precomp field of view tan(hfov/2.f*PI_BY_180)
  float aspect;         // aspect ratio of the camera (of the scene?)

  // The following are derived data from the previous data
  Vector3D u, v;        // u and v parameters
  Vector3D lookat_dir;  // normalized (lookat - eye)
	
public: 
  // Constructors and destructors
  PinholeCamera(void);		        // uninitialized constructor
  // Init most data values
  PinholeCamera(const Point3D& e, const Point3D& l, const Vector3D& u,
		const float ul, const float a);
  PinholeCamera(const PinholeCamera& p);// copy constructor (needed?) 
  ~PinholeCamera(void);			// destructor (needed?)
  
  // return the elements of the vector - inlined
  Point3D getEye() const {return eye;};     
  Point3D getLookat() const {return lookat;}; 
  Vector3D getUp() const {return up;};
  float getU_len() const {return u_len;};
  float getAspect() const {return aspect;};
  Vector3D getU() const {return u;};
  Vector3D getV() const {return v;};
  Vector3D getLdir() const {return lookat_dir;};
  
  // Functions that return information about the PinholeCamera
  // or modify parts of the camera
  void makeRay(Ray& ray, const float x, const float y) const; // modify the ray based on (x,y)
  void init(); // init the derived portions of the PinholeCamera
  
}; // end PinholeCamera class definition

// Inlined member functions for this class

// prototypes for non-member functions defined in PinholeCamera.cc

// None for this class... 

#endif
