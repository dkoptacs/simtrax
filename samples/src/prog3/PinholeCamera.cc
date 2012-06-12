/*
 *  PinholeCamera.cc
 *  
 *
 *  Created by Erik Brunvand on 9/11/11.
 *
 */
#include "PinholeCamera.h"
#include "Ray.h" 

// Constructors and destructors

// uninitialized constructor
// Use defaults from program2 assignment
PinholeCamera::PinholeCamera(void)
  : eye(Point3D(-24.f, -2.f, 5.2f)), lookat(Point3D(1.f, 1.f, 2.4f)),
    up(Point3D(0.f, 0.f, 1.f)), u_len(0.194f), aspect(1.f) {}

// Init most data values
PinholeCamera::PinholeCamera(const Point3D& e, const Point3D& l, const Vector3D& u,
			     const float ul, const float a)
  : eye(e), lookat(l), up(u), u_len(ul), aspect(a) {}

//init based on global memory camera
// Assume default values for u_len and aspect ratio... 
PinholeCamera::PinholeCamera(const int& addr) {
  eye = loadPtFromMem(addr); // Camera components defined by global memory spec
  up = loadPtFromMem(addr + 9);
  lookat_dir = loadVecFromMem(addr + 12);
  u = loadVecFromMem(addr + 15);    // I assume u and v are already normalized? 
  v = loadVecFromMem(addr + 18);
}

// copy constructor (needed?) 
PinholeCamera::PinholeCamera(const PinholeCamera& p)
  : eye(p.getEye()), lookat(p.getLookat()), up(p.getUp()),
    u_len(p.getU_len()), aspect(p.getAspect()) {}

// destructor (needed?)
PinholeCamera::~PinholeCamera(void) {}

// Functions that return information about the PinholeCamera
// or modify parts of the camera

// generate a ray based on (x,y) (slide 13)
// Return by modifying the argument ray address (why not return a ray&?)
void PinholeCamera::makeRay(Ray& ray, const float x, const float y) const {
  Vector3D ray_dir = lookat_dir + u*x + v*y; // parametric form of new ray's direction
  ray_dir.normalize();                       // remember to normalize
  ray = Ray(eye, ray_dir);                   // set ray arg based on eye point and ray direction
}

// init the derived portions of the PinholeCamera (see slide 10 in the notes)
// Only needed if you aren't loading the camera from global memory where
// u and v are precomputed. 
void PinholeCamera::init() {
  lookat_dir = lookat-eye;    // compute lookat_dir vector
  lookat_dir.normalize();     // normalize that vector
  u = Cross(lookat_dir, up);  // compute utmp vector
  v = Cross(u, lookat_dir);   // compute vtmp vector
  u.normalize();              // normalize utmp
  v.normalize();              // normalize vtmp
  u *= u_len;                 // multiply by u_len (precomputed) to get final u
  v *= u_len/aspect;          // multiple by (derived) v_len to get final v
}
	

