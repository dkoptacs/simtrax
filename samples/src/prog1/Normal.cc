/*
 *  Normal.cpp
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 * All the method code for the Normal class.
 *
 *
 */
#include "trax.hpp"
#include "Normal.h"
#include "Vector3D.h"
#include "Point3D.h" 

// uninitialized constructor
Normal::Normal(void)		            
  : x(0.f), y(0.f), z(0.f) {}

// Init all three elements to the same float
Normal::Normal(float a)		     
  : x(a), y(a), z(a) {}

// init all three elements
Normal::Normal(float xin, float yin, float zin) 
  : x(xin), y(yin), z(zin) {}

// copy constructor (needed?) 
Normal::Normal(const Normal& n)
  : x(n.x), y(n.y), z(n.z) {}

// construct normal from Vector3
Normal::Normal(const Vector3D& v)
  : x(v.x), y(v.y), z(v.z) {}

// destructor (needed?)
Normal::~Normal(void) {}

// return the elements of the Normal
// Note that because I made the Normal components public, you
// don't really need to use these functions... 
float Normal::getx() const {return x;};	// x component
float Normal::gety() const {return y;};	// y component
float Normal::getz() const {return z;};	// z component
  
// define overloaded operators on Normal class

// add two Normals
Normal Normal::operator+ (const Normal& n) const {
  return Vector3D(x+n.x, y+n.y, z+n.z);
}

// add a Vector3D to a Normal - return Vector3D
Vector3D Normal::operator+ (const Vector3D& v) const {
  return Vector3D(x+v.x, y+v.y, z+v.z);
}

// add to current Normal
Normal& Normal::operator+= (const Normal& n){     
  x+=n.x; y+=n.y; z+=n.z;
  return *this;
}

// negate the Normal
Normal Normal::operator- () const {
  return Normal(-x, -y, -z);
}


// multiply by a float
Normal Normal::operator* (const float a) const {
  return Normal(x*a, y*a, z*a);
}

// multiply current Normal by a float
Normal& Normal::operator*= (const float a){
  x*=a; y*=a; z*=a;
  return *this;
}

// divide by a float
Normal Normal::operator/ (const float a) const {
  float inv_a = 1.f/a;
  return Normal(x*inv_a, y*inv_a, z*inv_a);
}

// div current Normal by a float
Normal& Normal::operator/= (const float a){
  float inv_a = 1.f/a;
  x*=inv_a; y*=inv_a; z*=inv_a;
  return *this;
}
  
// Three assignments - convert from Vector3 and Point3 to Normal

// assignment
Normal& Normal::operator= (const Normal& n){
  x=n.x; y=n.y; z=n.z;
  return *this;
}

// assign Vector3 to Normal
Normal& Normal::operator= (const Vector3D& v){
  x=v.x; y=v.y; z=v.z;
  return *this;
}

// assign Point3 to Normal
Normal& Normal::operator= (const Point3D& p){
  x=p.x; y=p.y; z=p.z;
  return *this;
}
  
// Functions that return information about the Normal

// normalize this Normal (sounds wierd!)
void Normal::normalize() {
  float inv_len = 1.f/sqrt(x*x + y*y + z*z); // Get 1/length of vector
  x*=inv_len; y*=inv_len; z*=inv_len; // multiply vec components by 1/length
}

// normalize the normal and return it
Normal& Normal::hat(){
  float inv_len = 1.f/sqrt(x*x + y*y + z*z); // Get 1/length of vector
  x*=inv_len; y*=inv_len; z*=inv_len; // multiply vec components by 1/length
  return *this; // return the normalized vector
}

//
//---------------- non member functions related to Normal--------------
//
// Note that because the x, y, and z of Normal are public, you don't need
// to call the access function to get the components... 

// Multiply Normal by float on the left
Normal operator* (const float a, const Normal& n) {
  return Normal(a*n.x, a*n.y, a*n.z);
}

// Addition of a vector3D on the left to return a vector3
Vector3D operator+ (const Vector3D& v, const Normal& n) {
  return Vector3D(v.x+n.x, v.y+n.y, v.z+n.z);
}

// Subtraction of a normal from a Vector3D to return a Vector3D
Vector3D operator- (const Vector3D& v, const Normal& n) {
  return Vector3D(v.x-n.x, v.y-n.y, v.z-n.z);
}

// Dot product - normal on the left, Vector3D on the right
float Dot (const Normal& n, const Vector3D& v) {
  return n.x*v.x + n.y*v.y + n.z*v.z;
}

// Dot product - Vector3D on the left, Normal on the right
float Dot (const Vector3D& v, const Normal& n) {
  return v.x*n.x + v.y*n.y + v.z*n.z;
}

  
  
