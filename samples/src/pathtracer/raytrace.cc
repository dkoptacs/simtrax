#include "Color.h"
#include "Image.h"
#include "Ray.h"
#include "Sphere.h"
#include "Box.h"
#include "Triangle.h"
//#include "BoundingVolumeHierarchy_opt.h" // full optimization isn't as fast as just the closer hit check
#include "BoundingVolumeHierarchy.h"
#include "RayCamera.h"
#include "Light.h"
#include "LambertianMaterial.h"
#include "GlassMaterial.h"
#include "PhongMaterial.h"
#include "trax.hpp"

void trax_main()
{

  BoundingVolumeHierarchy bvh(loadi(0, 8)); // tell the BVH where the scene starts

  Light light = loadLightFromMemory(loadi(0, 12)); // load the light from memory
  //Light light(Vector(-0.1f, .8f, .1f), Color(20.f, 20.f, 20.f), DIRECTIONALLIGHT); // crytek
  //Light light(Vector(-0.274531f, -0.252461f, 0.213216f), Color(3.5f, 3.5f, 3.5f), 0.006f); // luxroom 
  //Light light(Vector(0.338314f, 0.703444f, 0.952079f), Color(3.5f, 3.5f, 3.5f), DIRECTIONALLIGHT); // luxsala
  
  int xres = loadi( 0, 1 );
  int yres = loadi( 0, 4 ); 
  float inv_width = loadf(0, 2);
  float inv_height = loadf(0, 5);
  int start_fb = loadi( 0, 7 );
  int start_matls = loadi(0, 9);
  int num_samples = loadi(0, 17);
  int max_depth = loadi(0, 16);

  int start_triangles = loadi(0, 28);
  int start_tex_coords = loadi(0, 30);

  // set up frame buffer
  Image image(xres, yres, start_fb);
  
  // load camera
  //RayCamera camera(loadi(0, 10)); // pinhole
  RayCamera camera(loadi(0, 10), 4.320378f); // luxroom

  // loop over pixels
  for(int pix = atomicinc(0); pix < xres * yres; pix = atomicinc(0))
    {

      int i = pix / xres;
      int j = pix % xres;

      Color result(0.f, 0.f, 0.f);
      
      // find center of pixel
      float x = 2.0f * (j - xres/2.f + 0.5f)/xres;
      float y = 2.0f * (i - yres/2.f + 0.5f)/yres;

      for(int i=0; i < num_samples; i++)
	{
	  Color attenuation(1.f, 1.f, 1.f);
	  int depth = 0;
	  Ray ray;

	  // cast a ray for this pixel
	  if(num_samples == 1)
	    camera.makeRay(ray, x, y);
	  else // randomize ray within pixel
	    {  // very naive sampling scheme (could be easily improved with jittering)
	      float x_off = (trax_rand() - 0.5f) * 2.f;
	      float y_off = (trax_rand() - 0.5f) * 2.f;
	      
	      x_off *= inv_width;
	      y_off *= inv_height;
	      camera.makeRay(ray, x + x_off, y + y_off);
	    }
	  // let ray bounce max_depth-1 times (camera ray counts)
	  while(depth++ < max_depth)
	    {	      
	      HitRecord hit(100000.f);
	      Color hit_color;
	      Vector new_dir;
	      Vector hit_point;

	      // intersect the ray with the scene
	      bvh.intersect(hit, ray);
	      
	      // Add light for this ray
	      int matl_type = 2; // default lambertian material
	      if(hit.didHit())
		{
		  int shader = loadi(hit.getID() + 10);
		  shader = start_matls + shader * 25; // mats are 25 words
		  matl_type = loadi(shader); // this is the "illum" parameter from obj mtl format
		}
	      if(matl_type == 0)
		result += shadeLambert(hit, ray, bvh, light, start_matls, start_tex_coords, hit_point, new_dir, hit_color) * attenuation;
	      else if (matl_type == 2)
		result += shadePhong(hit, ray, bvh, light, start_matls, start_tex_coords, hit_point, new_dir, hit_color) * attenuation;
	      else if(matl_type == 7)
		result += shadeGlass(hit, ray, bvh, light, start_matls, start_tex_coords, hit_point, new_dir, hit_color) * attenuation;
	      else // Just use Lambertian if the material isn't implemented
		result += shadeLambert(hit, ray, bvh, light, start_matls, start_tex_coords, hit_point, new_dir, hit_color) * attenuation;
	      
	      // can't continue the ray path if we hit the background
	      if(!hit.didHit())
		break;

	      // Set up ray for next bounce
	      ray.org = hit_point;
	      ray.dir = new_dir;
	      attenuation *= hit_color; // path loses energy on each bounce
	    }

	}
      result /= num_samples;

      // apply a tone-map
      float scale = 1.f / (1.f + result.luminance());
      result *= scale;

      image.set(i, j, result);
  }
}
