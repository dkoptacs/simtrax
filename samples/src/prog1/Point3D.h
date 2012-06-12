/*
 *  Point3D.h
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#ifndef __Point3D_h__
#define __Point3D_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Vector3D.h"


// The Point3D class
class Point3D {
public:		        // public for better performance? 
  float x, y, z;	// The three elements of the vector
	
public: 
  // Constructors and destructors
  Point3D(void);		            // uninitialized constructor
  Point3D(float a);			        // Init all three elements to the same float
  Point3D(float xin, float yin, float zin); // init all three elements
  Point3D(const Point3D& p);	    // copy constructor (needed?) 
  ~Point3D(void);			        // destructor (needed?)
  
  // return the elements of the vector
  // Technically not needed because components are public
  float getx() const;			// x component
  float gety() const;			// y component
  float getz() const;			// z component
  
  // define overloaded operators on Point3D class
  Point3D operator+ (const Vector3D& v) const;	// add a point and a vector
  Point3D& operator+= (const Vector3D& v);        // add vector to the current Point3D
  
  Vector3D operator- (const Point3D& p) const;   // subtract two Point3Ds - get a Vector3D!
  Point3D operator- () const;		       // negate the Point3D 
  
  Point3D operator* (const float a) const;      // multiply by a float
  Point3D& operator*= (const float a);          // multiply current Point3D by a float
  
  Point3D operator/ (const float a) const;      // divide by a float
  Point3D& operator/= (const float a);	       // div current Point3D by a float
  
  Point3D& operator= (const Point3D& p);	       // assignment
  
  // Functions that return information about the Point3D
  float dist_sq (const Point3D& p) const;      // distance between points, squared
  float distance (const Point3D& p) const;     // distance between points
  
}; // end Point3D class definition

// Prototypes for non-member functions defined in Point3D.cpp

// Multiply Point3D by constant on the left
Point3D operator* (const float a, const Point3D& p);

#endif

