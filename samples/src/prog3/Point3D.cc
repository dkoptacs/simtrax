/*
 *  Point3D.cpp
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 * All the method code for the Point3D class.
 *
 * I'm unclear about when you define the code in the .h file, and when you split it up
 * into a .h and a .cpp, and what the tradeoffs are... 
 *
 */
#include "trax.hpp"
#include "Point3D.h"
#include "Vector3D.h" 


// uninitialized constructor
Point3D::Point3D(void)		            
  : x(0.f), y(0.f), z(0.f) {}

// Init all three elements to the same float
Point3D::Point3D(float a)		     
  : x(a), y(a), z(a) {}

// init all three elements
Point3D::Point3D(float xin, float yin, float zin) 
  : x(xin), y(yin), z(zin) {}

// copy constructor (needed?) 
Point3D::Point3D(const Point3D& p)
  : x(p.x), y(p.y), z(p.z) {}

// destructor (needed?)
Point3D::~Point3D(void) {}

// return the elements of the vector
// Note that because I made the Point3D components public, you
// don't really need to use these functions... 
float Point3D::getx() const {return x;};	// x component
float Point3D::gety() const {return y;};	// y component
float Point3D::getz() const {return z;};	// z component

// define overloaded operators on Point3D class

// add a point and a vector
Point3D Point3D::operator+ (const Vector3D& v) const {
  return Point3D(x+v.x, y+v.y, z+v.z);
}

// add vector to the current Point3D
Point3D& Point3D::operator+= (const Vector3D& v){     
  x+=v.x; y+=v.y; z+=v.z;
  return *this;
}

// subtract two Point3Ds - get a vector
Vector3D Point3D::operator- (const Point3D& p) const {
  return Vector3D(x-p.x, y-p.y, z-p.z);
}

// negate the Point3D -
Point3D Point3D::operator- () const {
  return Point3D(-x, -y, -z);
}

// multiply by a float
Point3D Point3D::operator* (const float a) const {
  return Point3D(x*a, y*a, z*a);
}

// multiply current Point3D by a float
Point3D& Point3D::operator*= (const float a){
  x*=a; y*=a; z*=a;
  return *this;
}

// divide by a float
Point3D Point3D::operator/ (const float a) const {
  float inv_a = 1.f/a;
  return Point3D(x*inv_a, y*inv_a, z*inv_a);
}

// div current Point3D by a float
Point3D& Point3D::operator/= (const float a){
  float inv_a = 1.f/a;
  x*=inv_a; y*=inv_a; z*=inv_a;
  return *this;
}
  
// assignment
Point3D& Point3D::operator= (const Point3D& p){
  x=p.x; y=p.y; z=p.z;
  return *this;
}

// Functions that return information about the Point3D

// distance between points, squared
float Point3D::dist_sq (const Point3D& p) const {
  return ((x-p.x)*(x-p.x) + (y-p.y)*(y-p.y)+ (z-p.z)*(z-p.z));
}

// distance between points
float Point3D::distance (const Point3D& p) const{
  return (sqrt((x-p.x)*(x-p.x) + (y-p.y)*(y-p.y)+ (z-p.z)*(z-p.z)));
}
  

//
//---------------- non member functions related to Point3D--------------
//
// Note that because the x, y, and z of Point3D are public, you don't need
// to call the access function to get the components... 

// Multiply Point3D by constant on the left
Point3D operator* (const float a, const Point3D& p) {
  return Point3D(a*p.x, a*p.y, a*p.z);
}
// load a point from memory - x, y, and z are in consecutive locations
// starting at addr
Point3D loadPtFromMem(const int& addr) {
   return Point3D(loadf(addr,0), loadf(addr,1), loadf(addr, 2)); 
}

// return point with smaller components
Point3D MinComps(const Point3D& p1, const Point3D& p2) {
  return Point3D(min(p1.x, p2.x), min(p1.y, p2.y), min(p1.z, p2.z)); 
}

// return point with larger components
Point3D MaxComps(const Point3D& p1, const Point3D& p2) {
  return Point3D(max(p1.x, p2.x), max(p1.y, p2.y), max(p1.z, p2.z)); 
}


  
  
