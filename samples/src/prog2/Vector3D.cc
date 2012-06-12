/*
 *  Vector3D.cpp
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 * All the method code for the Vector3D class.
 *
 *
 */
#include "trax.hpp"
#include "Vector3D.h"
#include "Normal.h"
#include "Point3D.h"

// uninitialized constructor
Vector3D::Vector3D(void)		            
  : x(0.f), y(0.f), z(0.f) {}

// Init all three elements to the same float
Vector3D::Vector3D(float a)		     
  : x(a), y(a), z(a) {}

// init all three elements
Vector3D::Vector3D(float xin, float yin, float zin) 
  : x(xin), y(yin), z(zin) {}

// copy constructor (needed?) 
Vector3D::Vector3D(const Vector3D& v)
  : x(v.x), y(v.y), z(v.z) {}

// construct vector from normal
Vector3D::Vector3D(const Normal& n)
  : x(n.x), y(n.y), z(n.z) {}

// construct vector from point3
Vector3D::Vector3D(const Point3D& p)
  : x(p.x), y(p.y), z(p.z) {}

// destructor (needed?)
Vector3D::~Vector3D(void) {}

// return the elements of the vector
// Note that because I made the Vector3D components public, you
// don't really need to use these functions... 
float Vector3D::getx() const {return x;};	// x component
float Vector3D::gety() const {return y;};	// y component
float Vector3D::getz() const {return z;};	// z component

// define overloaded operators on Vector3D class

// add two Vector3Ds
Vector3D Vector3D::operator+ (const Vector3D& v) const {
  return Vector3D(x+v.x, y+v.y, z+v.z);
}

// add to current Vector3D
Vector3D& Vector3D::operator+= (const Vector3D& v){     
  x+=v.x; y+=v.y; z+=v.z;
  return *this;
}

// subtract two Vector3Ds
Vector3D Vector3D::operator- (const Vector3D& v) const {
  return Vector3D(x-v.x, y-v.y, z-v.z);
}

// negate the Vector3D
Vector3D Vector3D::operator- () const {
  return Vector3D(-x, -y, -z);
}

// sub from current Vector3D
Vector3D& Vector3D::operator-= (const Vector3D& v){
  x-=v.x; y-=v.y; z-=v.z;
  return *this;
}

// multiply by a float
Vector3D Vector3D::operator* (const float a) const {
  return Vector3D(x*a, y*a, z*a);
}

// multiply current Vector3D by a float
Vector3D& Vector3D::operator*= (const float a){
  x*=a; y*=a; z*=a;
  return *this;
}

// divide by a float
Vector3D Vector3D::operator/ (const float a) const {
  float inv_a = 1.f/a;
  return Vector3D(x*inv_a, y*inv_a, z*inv_a);
}

// div current Vector3D by a float
Vector3D& Vector3D::operator/= (const float a){
  float inv_a = 1.f/a;
  x*=inv_a; y*=inv_a; z*=inv_a;
  return *this;
}
  
// Three assignments - convert from Normal and Point3D to Vector3D

// assignment
Vector3D& Vector3D::operator= (const Vector3D& v){
  x=v.x; y=v.y; z=v.z;
  return *this;
}

// assign Normal to Vector3D
Vector3D& Vector3D::operator= (const Normal& n){
  x=n.x; y=n.y; z=n.z;
  return *this;
}

// assign Point3D to Vector3D
Vector3D& Vector3D::operator= (const Point3D& p){
  x=p.x; y=p.y; z=p.z;
  return *this;
}
  
// Functions that return information about the Vector3D

// return Vector3D length
float Vector3D::length() const {
  return sqrt(x*x + y*y + z*z);
}

// Return Vector3D length squared 
float Vector3D::length_sq() const {
  return x*x + y*y + z*z;
}

// normalize this Vector3D
float Vector3D::normalize() {
  float len = sqrt(x*x + y*y + z*z); // Get length of vector
  float inv_len = 1.f/len;           // get 1/length
  x*=inv_len; y*=inv_len; z*=inv_len; // multiply vec components by 1/length
  return len;                        // return the length
}

// normalize the vector and return it
Vector3D& Vector3D::hat(){
  float inv_len = 1.f/sqrt(x*x + y*y + z*z); // Get 1/length of vector
  x*=inv_len; y*=inv_len; z*=inv_len; // multiply vec components by 1/length
  return *this; // return the normalized vector
}

//
//---------------- non member functions related to Vector3D--------------
//
// Note that because the x, y, and z of Vector3D are public, you don't need
// to call the access function to get the components... 

// Multiply Vector3D by constant on the left
Vector3D operator* (const float a, const Vector3D& v) {
  return Vector3D(a*v.x, a*v.y, a*v.z);
}

// Dot product
float Dot (const Vector3D& v1, const Vector3D& v2) {
  return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

// Cross product
Vector3D Cross (const Vector3D& v1, const Vector3D& v2) {
  return Vector3D(v1.y*v2.z - v1.z*v2.y,
		  v1.z*v2.x - v1.x*v2.z,
		  v1.x*v2.y - v1.y*v2.x);
}

  
  
