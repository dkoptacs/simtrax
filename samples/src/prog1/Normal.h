/*
 *  Normal.h
 *  
 *
 *  Created by Erik Brunvand on 9/4/11.
 *
 */
#ifndef __Normal_h__
#define __Normal_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Vector3D.h"

// Forward reference to the Point3D class
class Point3D; 

// The Normal class
class Normal {
public:		        // public for better performance? 
  float x, y, z;	// The three elements of the vector
	
public: 
  // Constructors and destructors
  Normal(void);		            // uninitialized constructor
  Normal(float a);			    // Init all three elements to the same float
  Normal(float xin, float yin, float zin); // init all three elements
  Normal(const Normal& n);	            // copy constructor (needed?) 
  Normal(const Vector3D& v);	            // construct normal from vector
  ~Normal(void);			    // destructor (needed?)
  
  // return the elements of the vector
  float getx() const;			// x component
  float gety() const;			// y component
  float getz() const;			// z component
  
  // define overloaded operators on Normal class
  Normal operator+ (const Normal& n) const;     // add two Normals
  Vector3D operator+ (const Vector3D& v) const; // add Vector3D to Normal
  Normal& operator+= (const Normal& n);	        // add to current Normal
   
  Normal operator- () const;		      // negate the Normal

  Normal operator* (const float a) const;      // multiply by a float
  Normal& operator*= (const float a);          // multiply current Normal by a float
  
  Normal operator/ (const float a) const;      // divide by a float
  Normal& operator/= (const float a);		// div current Normal by a float
  
  // Three assignments - convert from Vector3D and Point3D to Normal
  Normal& operator= (const Normal& n);	        // assignment
  Normal& operator= (const Vector3D& v);		// assign Vector to Normal
  Normal& operator= (const Point3D& p);		// assign Point3D to Normal
  
  // Functions that return information about the Normal
  void normalize();				// normalize this Normal
  Normal& hat();				// normalize the normal and return it
  
}; // end Normal class definition

// prototypes for non-member functions defined in Normal.cc

// Multiply Normal by a float on the left
Normal operator* (const float a, const Normal& n);

// Addition of a vector3D on the left to return a vector3D
Vector3D operator+ (const Vector3D& v, const Normal& n);

// Subtraction of a normal from a Vector3D to return a Vector3D
Vector3D operator- (const Vector3D& v, const Normal& n);

// Dot product - normal on the left, Vector3D on the right
float Dot (const Normal& n, const Vector3D& v);

// Dot product - Vector3D on the left, Normal on the right
float Dot (const Vector3D& v, const Normal& n);

#endif
