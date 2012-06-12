/*
 *  PointLight.cc
 *  
 *
 *  Created by Erik Brunvand on 9/10/11.
 *
 */

#include "PointLight.h" 
#include "constants.h"

// Constructors and destructors

// uninitialized constructor
PointLight::PointLight(void)
  : position(Point3D(0.0f)), color(WHITE) {}

// init all components
PointLight::PointLight(const Point3D& p, const ColorRGB& c)
  : position(p), color(c) {}

// copy constructor 
PointLight::PointLight(const PointLight& p)
  : position(p.getPos()), color(p.getColor()) {}

// destructor
PointLight::~PointLight(void) {}

