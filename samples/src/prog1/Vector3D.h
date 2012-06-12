/*
 *  Vector3D.h
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#ifndef __Vector3D_h__
#define __Vector3D_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...

// Forward references to other classes
class Point3D;
class Normal;

// The Vector3D class
class Vector3D {
public:		        // public for better performance? 
  float x, y, z;	// The three elements of the vector
	
public: 
  // Constructors and destructors
  Vector3D(void);		            // uninitialized constructor
  Vector3D(float a);			    // Init all three elements to the same float
  Vector3D(float xin, float yin, float zin); // init all three elements
  Vector3D(const Vector3D& v);	    // copy constructor (needed?) 
  Vector3D(const Normal& n);	    // construct vector from normal
  Vector3D(const Point3D& p);	    // construct vector from point3
  ~Vector3D(void);			        // destructor (needed?)
  
  // return the elements of the vector
  // Note that because the components are public, you don't really need
  // to use these access functions
  float getx() const;			// x component
  float gety() const;			// y component
  float getz() const;			// z component
  
  // define overloaded operators on Vector3D class
  Vector3D operator+ (const Vector3D& v) const;	// add two Vector3Ds
  Vector3D& operator+= (const Vector3D& v);	    // add to current Vector3D
  
  Vector3D operator- (const Vector3D& v) const; // subtract two Vector3Ds
  Vector3D operator- () const;		            // negate the Vector3D
  Vector3D& operator-= (const Vector3D& v);	    // sub from current Vector3D
  
  Vector3D operator* (const float a) const;	    // multiply by a float
  Vector3D& operator*= (const float a);         // multiply current Vector3D by a float
  
  Vector3D operator/ (const float a) const;     // divide by a float
  Vector3D& operator/= (const float a);		    // div current Vector3D by a float
  
  // Three assignments - convert from Normal and Point3D to Vector3D
  Vector3D& operator= (const Vector3D& v);	// assignment
  Vector3D& operator= (const Normal& n);	// assign Normal to Vector3D
  Vector3D& operator= (const Point3D& p);	// assign Point3D to Vector3D
  
  // Functions that return information about the Vector3D
  float length() const;		    // return Vector3D length
  float length_sq() const;		// Return Vector3D length squared 
  void normalize();				// normalize this Vector3D
  Vector3D& hat();				// normalize the vector and return it
  
}; // end Vector3D class definition

// Prototypes for non-member functions defined in Vector3D.cpp

Vector3D operator* (const float a, const Vector3D& v);   // Mult by const on the left
float Dot (const Vector3D& v1, const Vector3D& v2);      // Dot product (v dot v)
Vector3D Cross (const Vector3D& v1, const Vector3D& v2); // cross product (v cross v

#endif
