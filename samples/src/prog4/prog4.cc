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
#include "myBVH.h"
#include "constants.h"

void trax_main()
{
  //  trax_setup(); // Trax setup code
   
  // Make a new frame that includes all the objects in the scene
  Scene myScene; // use the default constructor

  // Make a new image object - called frame
  Image frame(myScene.getXres(), myScene.getYres(), myScene.getFB());

  // load the camera from the global memory
  PinholeCamera camera(myScene.getCamera());

  // make a bvh to hold the scene. This actually just holds the
  // addresses of the nodes, but has the intersect method that
  // does the ray traversal
  myBVH bvh; // default constructor grabs data from global mem
  
#if TRAX==0
  printf("Starting pixel iteration...\n");
#endif

  // Render the image
  myScene.render(frame, camera, bvh); 

#if TRAX==0
  printf("Done computing pixels...\n");
#endif

  //  trax_cleanup(); // trax cleanup code
} // end main
