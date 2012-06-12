/*
 * Main raytrace loop
 *
 * Erik Brunvand, 9/5/2011
 *
 * based on Danny and Josef's examples.
 *
 */
// Only include stdio for printf on non-trax version
#if TRAX==0
#include <stdio.h>
#endif

// Include a bunch of stuff... 
#include "trax.hpp"
#include "ColorRGB.h"
#include "Image.h"
#include "Ray.h"
#include "Sphere.h"
#include "HitRecord.h"
#include "PinholeCamera.h"
#include "PointLight.h"
#include "Scene.h"
#include "constants.h"

void trax_main()
{
   
  // Make a new frame that includes all the objects in the scene
  Scene myScene; // use the default constructor

  // Make a new image object - called frame
  Image frame(myScene.getXres(), myScene.getYres(), myScene.getFB());
  
  // Make a new PinholeCamera from which to generate rays
  PinholeCamera camera; // use the default constructor for now
  camera.init();        // initialize the derived components of the camera

  
#if TRAX==0
  printf("Starting pixel iteration...\n");
#endif

  // Render the image
  myScene.render(frame, camera); 

#if TRAX==0
  printf("Done computing pixels...\n");
#endif

} // end main
