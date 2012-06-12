/*
 *  HitRecord.cc
 *  
 *
 *  Created by Erik Brunvand on 9/10/11.
 *
 */

#include "trax.hpp"    // trax-specific stuff
#include "HitRecord.h" 

// Constructors and destructors

// uninitialized constructor
HitRecord::HitRecord(void)
  : Tmin(FLOAT_MAX), ObjAddr(0), Hitp(false) {}

// init only Tmin - defaults for the rest
HitRecord::HitRecord(const float t_in)
  : Tmin(t_in), ObjAddr(0), Hitp(false) {}

// Init all data values
HitRecord::HitRecord(const float t_in, const int o_in, const bool h_in)
  : Tmin(t_in), ObjAddr(o_in), Hitp(h_in) {}

// copy constructor (needed?) 
HitRecord::HitRecord(const HitRecord& h)
  : Tmin(h.getTmin()), ObjAddr(h.getObjAddr()), Hitp(h.didHit()) {}

// destructor (needed?) 
HitRecord::~HitRecord(void) {}

  
