#ifndef LAMBERTIAN_TEXTURED_MATERIAL_H
#define LAMBERTIAN_TEXTURED_MATERIAL_H

#include "Triangle.h"
#include "Light.h"
#include "BoundingVolumeHierarchy.h"
#include "Vector.h"
#include "Texture.h"
#include "trax.hpp"


// Lambertian shader with simple cosine-weighted reflection distribution
// Will use textures if applicable
Color shadeTexturedLambert(HitRecord &hr, const Ray &orig_ray, const Scene &scene, Vector &hit_point, Vector &new_dir, Color &hit_color)
{

  //float epsilon = loadf(0, 18);
  float epsilon = 0.001f;

  // Passing a ray through a transparent texture may require changing the ray.
  // Start with a copy of it
  Ray ray = orig_ray;
  Color retVal(0.f, 0.f, 0.f);
  retVal.A = 0.f;

  Vector normal;

  // Transparent textures complicate things a bit. Need to loop until the ray hits an opaque surface
  do
    {
      if(hr.didHit())
	{
	  hit_point = ray.origin() + (hr.minT() * ray.direction());
	  Tri hit_tri = loadTriFromMemory(hr.getID());
	  normal = hit_tri.normal();
	  float costheta = Dot(normal, ray.direction());
	  // Flip normal to face the ray
	  if(costheta > 0.0f)
	    normal = -normal;
	  
	  hit_point = hit_point + (normal * epsilon); // offset to prevent self-shadowing

	  // tempColor will accumulate the blended texture color if we pass through transparent texels
	  Color tempColor(0.f, 0.f, 0.f);
	  Color lightColor;
	  Vector lightDir;
	  float lightDistance = scene.light.getLight(lightColor, lightDir, hit_point);
	  float cosphi = Dot(normal, lightDir);
	  
	  // Did we need to cast a shadow ray at all?
	  if(cosphi > 0.0f)
	    { 
	      Ray shadowRay(hit_point, lightDir);
	      Color shadowSurfaceColor;
	      do // let the shadow ray continue through transparent textures
		{
		  HitRecord shadowHit(lightDistance);
		  scene.bvh.intersect(shadowHit, shadowRay);
		  // if the shadow ray doesn't intersect the scene, we're done
		  if(!shadowHit.didHit())
		    { 
		      tempColor += (lightColor * cosphi);
		      break;
		    }
		  // Load the material ID of the hit object
		  int shadowMatID = loadi(shadowHit.getID() + 10);
		  shadowMatID = scene.start_materials + shadowMatID * 25;
		  // Load the texture pointer
		  int diffuse_texture = loadi(shadowMatID, 15);
		  // Check if this material has a texture
		  if(diffuse_texture >=0)
		    shadowSurfaceColor = getTextureColor(shadowHit, scene.start_tex_coords, diffuse_texture);
		  
		  // if the material is completely transparent, pass the shadow ray along (create new ray and intersect)
		  // ideally we would let it continue through translucent materials as well, then blend the returned light
		  if(shadowSurfaceColor.A == 0.f)
		    {
		      Vector shadow_hit_point = shadowRay.origin() + (shadowHit.minT() * shadowRay.direction());
		      shadowRay.org = shadow_hit_point + (shadowRay.dir * epsilon); // avoid self shadowing
		    }
		  else
		    break;
		}
	      while(true);
	    }
	  
	  // Once the shadow ray is done, shade the original hit point
	  int shader = loadi(hr.getID() + 10);
	  shader = scene.start_materials + shader * 25; // mats are 25 words
	  
	  // Check if this material has a texture
	  int diffuse_texture = loadi(shader, 15);
	  if(diffuse_texture >= 0) 
	    hit_color = getTextureColor(hr, scene.start_tex_coords, diffuse_texture);
	  else
	    hit_color = loadColorFromMemory(shader);
	  
	  tempColor *= hit_color;
	  tempColor.A = hit_color.A;

	  retVal = retVal.blend(tempColor);
	  
	  // If the shaded surface is not opaque, pass the ray through and intersect again
	  if(tempColor.A < 1.f)
	    {
	      hr = HitRecord();
	      ray.org = (hit_point - (normal * epsilon)) + (ray.dir * epsilon);
	      scene.bvh.intersect(hr, ray);
	    }
	  else
	    break;
	}
      else
	{
	  retVal = retVal.blend(scene.background);
	  break;
	}
    }
  while(true);

  // Set up the next ray for path tracing
  // Random reflection on the normal hemisphere, automatically weighted by cosine of angle between the new ray and the normal
  new_dir = randomReflection(normal);
  return retVal;
}

#endif // LAMBERTIAN_TEXTURED_MATERIAL_H
