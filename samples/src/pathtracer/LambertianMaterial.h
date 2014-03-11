#ifndef LAMBERTIAN_MATERIAL_H
#define LAMBERTIAN_MATERIAL_H

#include "Triangle.h"
#include "Light.h"
#include "BoundingVolumeHierarchy.h"
#include "Vector.h"
#include "Texture.h"
#include "Scene.h"
#include "trax.hpp"

// Lambertian shader with simple cosine-weighted reflection distribution
// This shader ignores textures
Color shadeLambert(HitRecord &hr, const Ray &ray, const Scene &scene, Vector &hit_point, Vector &new_dir, Color &hit_color)
{
  //float epsilon = loadf(0, 18);
  float epsilon = 0.001f;

  Color retVal(0.f, 0.f, 0.f);
  Vector normal;
  
  if(hr.didHit())
    {
      hit_point = ray.origin() + (hr.minT() * ray.direction());
      Tri hit_tri = loadTriFromMemory(hr.getID());
      normal = hit_tri.normal();
      float costheta = Dot(normal, ray.direction());
      // Flip the normal to face the ray's origin
      if(costheta > 0.0f)
	normal = -normal;
      
      hit_point = hit_point + (normal * epsilon); // offset to prevent self-shadowing
      
      Color lightColor;
      Vector lightDir;
      float lightDistance = scene.light.getLight(lightColor, lightDir, hit_point);
      float cosphi = Dot(normal, lightDir);
      
      // Check if we even need to cast a shadow ray
      if(cosphi > 0.0f)
	{ 
	  Ray shadowRay(hit_point, lightDir);
	  
	  HitRecord shadowHit(lightDistance);
	  scene.bvh.intersect(shadowHit, shadowRay);
	  // if the shadow ray doesn't intersect anything before the light
	  if(!shadowHit.didHit())
	    { 
	      retVal += (lightColor * cosphi);
	    }
	}
      int shader = loadi(hr.getID() + 10);
      shader = scene.start_materials + shader * 25; // mats are 25 words
      hit_color = loadColorFromMemory(shader);
      retVal *= hit_color;
    }
  else
    {
      retVal = scene.background;
    }
  
  // Set up the next ray for path tracing
  // Random reflection on the normal hemisphere, automatically weighted by cosine of angle between the new ray and the normal
  new_dir = randomReflection(normal); 

  return retVal;
}

#endif // LAMBERTIAN_MATERIAL_H
