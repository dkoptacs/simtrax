/*
 * Main raytrace loop
 *
 * Erik Brunvand, 9/5/2011
 *
 * based on Danny and Josef's examples.
 *
 */

// The "#if TRAX==0" directive lets me put in printf and other 
// includes that only happen if we're using the "run_rt" 
// version, and not on the version that runs on the TRaX simulator. 

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

void trax_main()
{
  
  int xres = GetXRes();             // screen x resolution from trax
  int yres = GetYRes();             // screen y resolution from trax
  int start_fb = GetFrameBuffer();  // frame buffer start address from trax
  const float zdim = -3.0f;         // z dimension of ray origin point
   
  // Make a new image object - called frame
  Image frame(xres, yres, start_fb);  // could have been frame(); 

  // Make some spheres for the prog01 image
  // Each sphere is a center Point3 and a float radius
  Sphere s1(Point3D(0.0f, 0.2f, 1.1f), 1.0f);
  Sphere s2(Point3D(1.4f, 1.5f, 1.2f), 1.3f);
  Sphere s3(Point3D(-1.5f, -.5f, 1.0f), 1.9f);

#if TRAX==0
  printf("Starting pixel iteration...\n");
#endif

  // This loop iterates through all the pixels. So, it's really the
  // pixel number as a linear walk through the 2d frame. This, ironically
  // is exactly what the image.set function really wants... but we need the
  // x and y to generate each ray. 
  for(int pix = atomicinc(0); pix < xres*yres; pix = atomicinc(0)) {
    ColorRGB result;      // color of the hit point
    int i = pix / xres;   // column of the pixel
    int j = pix % xres;   // row of the pixel

    // These statements magically compute the x and y
    // components of the direction vector for the ray
    float x = 2.0f * (j - xres/2.f + 0.5f)/xres;
    float y = 2.0f * (i - yres/2.f + 0.5f)/yres;

    // This is the ray from the eye point through the center of the pix
    // from this iteration of the loop. 
    Ray ray(Point3D(0.f,0.f,zdim), Vector3D(x, y, 1.f));
    
	// Brute force check of each of the three spheres to see if we hit
    if(s1.intersects(ray))
      result = ColorRGB(1.f, .4f, 1.f);
    else if(s2.intersects(ray))
      result = ColorRGB(.2f, .3f, 1.f);
    else if(s3.intersects(ray))
      result = ColorRGB(1.f, .3f, .2f);
    else // background color
      result = ColorRGB(.2f, .1f, .5f);

    // write the color into the frame
    frame.set(j, i, result);
  } // end for
#if TRAX==0
  printf("Done computing pixels...\n");
#endif

} // end trax_main
