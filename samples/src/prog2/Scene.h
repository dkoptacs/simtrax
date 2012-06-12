/*
 *  Scene.h
 *  
 *
 *  Created by Erik Brunvand on 9/12/11.
 *
 *
 *
 */
#ifndef __Scene_h__
#define __Scene_h__

#include "trax.hpp" // trax-specific stuff
// May be more includes here...
#include "Sphere.h"
#include "ColorRGB.h"
#include "PointLight.h"
#include "Image.h"
#include "PinholeCamera.h"
#include "constants.h"

// The Scene class
class Scene {
private:	     // public for better performance? 
  Sphere spheres[NUMOBJ];       // the spheres from program1
  ColorRGB materials[NUMMAT];   // the materials (colors) of the objects
  PointLight lights[NUMLIGHT];  // the lights
  int xres, yres;               // Extent of the screen
  int start_fb;                 // address of frame buffer start
  ColorRGB ambient, bground;    // Ambient and background colors
  float Ka, Kd;                 // constants for ambient and diffuse light components
 
public: 
  // Constructors and destructors
  Scene(void);		             // uninitialized constructor
  Scene(const ColorRGB& a, const ColorRGB& b, const float ka, const float kd);  // init some elements
  ~Scene(void);	  		     // destructor (needed?)
  
  // return elements of the Scene (inlined)
  float getXres() const {return xres;} ;      // xres of frame
  float getYres() const {return yres;} ;      // yres of frame
  float getFB() const {return start_fb;};     // start address of frame buffer

  // Member functions

  // render all the elements of the scene with shadows
  void render(Image& frame, const PinholeCamera& camera);   
  
}; // end Scene class definition

#endif
