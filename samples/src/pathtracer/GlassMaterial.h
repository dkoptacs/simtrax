#ifndef GLASS_MATERIAL_H
#define GLASS_MATERIAL_H

#include "Triangle.h"
#include "Light.h"
#include "BoundingVolumeHierarchy.h"
#include "Vector.h"
#include "trax.hpp"
#include <stdio.h>

// Glass material designed for use with a path tracer only. 
// Stochasitcally casts reflection OR refraction ray weighted by fresnel-schlick, but never both
// This shader never samples the light source directly (so you won't get Phong highlights). It's designed as a pure (non-Kajiya) path tracer shader.
// TODO: Attenuate the ray color by Beer's law

Color shadeGlass(HitRecord &hr, const Ray &orig_ray, const BoundingVolumeHierarchy &bvh, const Light &pl, const int &start_matls, const int &start_tex_coords, Vector &hit_point, Vector &new_dir, Color &hit_color)
{
  Color background = Color(0.561f * 3.f, 0.729f * 3.f, 0.988f * 3.f); // hard coded for run-rt

  //float epsilon = loadf(0, 18);
  float epsilon = 0.001f;

  Ray ray = orig_ray;
  Color retVal(0.f, 0.f, 0.f);
  retVal.A = 0.f;

  Vector normal;
  
  if(hr.didHit())
    {
      hit_point = ray.origin() + (hr.minT() * ray.direction());
      Tri hit_tri = loadTriFromMemory(hr.getID());
      normal = hit_tri.normal();
      float costheta = Dot(normal, ray.direction());
	  
      int shader = loadi(hr.getID() + 10);
      shader = start_matls + shader * 25; // mats are 25 words
      hit_color = loadColorFromMemory(shader);

      //TODO: fix this
      // Index of refraction is stored in diffuse texture scale0 (total hack), hopefully glass won't have a texture
      float eta = loadf(shader, 16);
      
      // Do we need to flip the normal?
      if(costheta > 0.0f)
	{
	  normal = -normal;
	  eta = 1.f / eta;
	}
      else
	costheta = -costheta;
      
      float costheta2_squared = 1.f + ((costheta * costheta) - 1.f) / (eta * eta);
      
      Vector bounceRayDir; // will be either reflection or refraction
      Ray bounceRay;
      HitRecord bounceHit;
      
      if(costheta2_squared < 0.f) // total internal reflection
	{	  
	  new_dir = ray.direction() + normal * 2.f * costheta;
	  new_dir.normalize();
	  hit_point = hit_point + (new_dir * epsilon);
	  return Color(0.f, 0.f, 0.f);
	}
      
      // no TIR

      float costheta2 = sqrt(costheta2_squared);
      float cosmin = min(costheta, costheta2);
      float R0 = (eta - 1.f) / (eta + 1.f);
      R0 = R0 * R0;
      
      // schlick approximation: Fr = R0 + (1 - R0) * (1 - costheta)^5
      float schlick = 1.f - cosmin;
      float schlick4 = schlick * schlick; // ^2
      schlick4 *= schlick4;               // ^4
      schlick = schlick * schlick4;       // ^5
      
      float F_r = R0 + (1.f - R0) * schlick; // reflection term 

      // Now randomly select either reflection or refraction, weighted by fresnel-schlick
      float rand = trax_rand();
      if(rand < F_r) // do reflection
	{
	  new_dir = ray.direction() + normal * 2.f * costheta;
	  new_dir.normalize();
	  hit_point = hit_point + (new_dir * epsilon);
	  return Color(0.f, 0.f, 0.f);
	}
      else // refraction implied by 1 - F_r
	{
	  float F_t = 1.f - F_r; // transmission term (refraction)
	  new_dir = ray.direction() * (1.f / eta) + normal * ((costheta / eta) - costheta2);
	  new_dir.normalize();
	  hit_point = hit_point + (new_dir * epsilon);
	  return Color(0.f, 0.f, 0.f);
	}
    }
  else
    {
      return background;
    }
}

#endif
