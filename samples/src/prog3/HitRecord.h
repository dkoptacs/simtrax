/*
 *  HitRecord.h
 *  
 *
 *  Created by Erik Brunvand on 9/10/11.
 *
 */
#ifndef __HitRecord_h__
#define __HitRecord_h__

#include "trax.hpp"    // trax-specific stuff
#include "constants.h" // my constants

// The HitRecord class
class HitRecord {
private:                // public for better performance? 
  float Tmin;           // minimum hit distance
  int ObjAddr;          // address of thr object (triangle) that you hit
  bool Hitp;            // A boolean to record that a hit happened
	
public: 
  // Constructors and destructors
  HitRecord(void);		        // uninitialized constructor
  HitRecord(const float t_in);  // init only Tmin - defaults for the rest
  HitRecord(const float t_in, const int o_in, const bool h_in); // Init all data values
  HitRecord(const HitRecord& n);// copy constructor (needed?) 
  ~HitRecord(void);			    // destructor (needed?)
  
  // return the elements of the vector - inlined
  int getObjAddr() const {return ObjAddr;};   // ObjId component
  float getTmin() const {return Tmin;};   // Tmin component
  bool didHit() const {return Hitp;};     // DidHit component
  
  // Functions that return information about the HitRecord
  bool hit(const float t, const int prim); // Compute a hit, return true if hit
  
}; // end HitRecord class definition

// Inlined member functions for this class

// return true if you hit the primitive. Update the HitRecord to store
// the Tmin and the primitive ID if you do hit
inline bool HitRecord::hit(const float t, const int addr) {
  if (t < Tmin && t > EPSILON) { // if this value is closer than previous hit
    Tmin = t;        // reset the HitRecord's Tmin to this new hit's t value
    ObjAddr = addr;    // reset the HitRecord's ObjAddr to this triangle's address
    Hitp = true;     // Set Hitp to indicate that there was a hit
    return true; }   // return true for a hit
  else return false; // return false if the input t was bigger than the current Tmin
}
// prototypes for non-member functions defined in HitRecord.cc

// None for this class... 

#endif
