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
#include "Box.h"
#include "PinholeCamera.h"
#include "constants.h"

// These constants are used in the Scene class to define the arrays that
// hold the objects
#define NUMOBJ  1       // number of objects in the scene
#define NUMMAT  4       // number of materials in the scene
#define NUMLIGHT 2      // number of lights in the scene


// The Scene class
class Scene {
private:	     // public for better performance? 
  //  Box boxes[NUMOBJ];            // the test box
  //  ColorRGB materials[NUMMAT];   // the materials (colors) of the objects
  PointLight light;             // Only one light defined in .obj files
  int xres, yres;               // Extent of the screen
  int start_fb, start_scene;    // addresses of things in global memory... 
  int start_matls, start_camera, start_light; 
  int start_triangles, num_triangles; 
  ColorRGB ambient, bground;    // Ambient and background colors
  float Ka, Kd;                 // constants for ambient and diffuse light components
 
public: 
  // Constructors and destructors
  Scene(void);		             // default constructor
  ~Scene(void);	  		     // destructor (needed?)
  
  // return elements of the Scene (inlined)
  int getXres() const {return xres;} ;      // xres of frame
  int getYres() const {return yres;} ;      // yres of frame
  int getFB() const {return start_fb;};     // Return start addresses of various things
  int getScene() const {return start_scene;} ; // in global memory...
  int getMats() const {return start_matls;};
  int getCamera() const {return start_camera;};
  int getLight() const {return start_light;};
  int getTris() const {return start_triangles;};
  int getNuiTris() const {return num_triangles;};

  // Member functions

  // render all the elements of the scene with shadows
  void render(Image& frame, const PinholeCamera& camera);   
  
}; // end Scene class definition

#endif
