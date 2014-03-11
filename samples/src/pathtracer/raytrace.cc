#include "Color.h"
#include "Image.h"
#include "Ray.h"
#include "Sphere.h"
#include "Box.h"
#include "Triangle.h"
#include "BoundingVolumeHierarchy.h"
#include "RayCamera.h"
#include "Light.h"
#include "LambertianMaterial.h"
#include "LambertianTexturedMaterial.h"
#include "Scene.h"
#include "trax.hpp"

void trax_main()
{

  // Load scene parameters
  //BoundingVolumeHierarchy bvh(loadi(0, 8)); // tell the BVH where the scene starts
  int start_bvh = loadi(0, 8);
  Light light = loadLightFromMemory(loadi(0, 12)); // load the light from memory
  int xres = loadi( 0, 1 );
  int yres = loadi( 0, 4 ); 
  float inv_width = loadf(0, 2);
  float inv_height = loadf(0, 5);
  int start_fb = loadi( 0, 7 );
  int start_matls = loadi(0, 9);
  int num_samples = loadi(0, 17);
  int max_depth = loadi(0, 16);
  int start_tex_coords = loadi(0, 30);

  // set up frame buffer
  Image image(xres, yres, start_fb);
  
  // load camera
  RayCamera camera(loadi(0, 10));

  // Blue sky background
  Color background(0.561f * 3.f, 0.729f * 3.f, 0.988f * 3.f); 

  Scene scene(start_bvh, start_matls, start_tex_coords, light, background);

  // loop over pixels
  for(int pix = atomicinc(0); pix < xres * yres; pix = atomicinc(0))
    {
      int i = pix / xres;
      int j = pix % xres;

      Color result(0.f, 0.f, 0.f);
      
      // find center of pixel
      float x = 2.0f * (j - xres/2.f + 0.5f)/xres;
      float y = 2.0f * (i - yres/2.f + 0.5f)/yres;
      
      // Loop over samples
      for(int i=0; i < num_samples; i++)
	{
	  Ray ray;

	  // Generate a ray for this pixel
	  if(num_samples == 1) // through center of pixel if not multisampling
	    camera.makeRay(ray, x, y);
	  else // randomize ray within pixel if multisampling
	    {  
	      // very naive sampling scheme (could be easily improved with jittering)
	      float x_off = (trax_rand() - 0.5f) * 2.f;
	      float y_off = (trax_rand() - 0.5f) * 2.f;
	      
	      x_off *= inv_width;
	      y_off *= inv_height;
	      camera.makeRay(ray, x + x_off, y + y_off);
	    }

	  // Keep track of current energy of ray path - starts at full white
	  Color attenuation(1.f, 1.f, 1.f);
	  // Keep track of current ray path depth
	  int depth = 0;

	  // let ray bounce max_depth-1 times (camera ray counts)
	  while(depth++ < max_depth)
	    {	      
	      HitRecord hit(100000.f);

	      // Keep track of hit color, so we can attenuate ray path energy
	      Color hit_color;
	      // Shader will set up next bounce direction (pass this in by reference)
	      Vector new_dir;
	      // Shader will set up next bounce origin (pass this in by reference)
	      Vector hit_point;

	      // intersect the ray with the scene
	      scene.bvh.intersect(hit, ray);

	      // Add light for this ray
	      result += shadeLambert(hit, ray, scene, hit_point, new_dir, hit_color) * attenuation;

	      // Use this instead of the above if you want to support textures
	      //result += shadeTexturedLambert(hit, ray, scene, hit_point, new_dir, hit_color) * attenuation;

	      // Can't continue the ray path if it didn't hit anything
	      if(!hit.didHit())
		break;

	      // Set up ray for next bounce, origin and direction were set by reference in the shader
	      ray.org = hit_point;
	      ray.dir = new_dir;
	      attenuation *= hit_color; // path loses energy on each bounce
	    }

	}

      // Box filter
      result /= num_samples;

      // Apply a simple tone map
      float scale = 1.f / (1.f + result.luminance());
      result *= scale;

      image.set(i, j, result);
  }
}
