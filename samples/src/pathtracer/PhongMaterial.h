#ifndef PHONG_MATERIAL_H
#define PHONG_MATERIAL_H

#include "Triangle.h"
#include "Light.h"
#include "BoundingVolumeHierarchy.h"
#include "Vector.h"
#include "Texture.h"
#include "trax.hpp"

// Lambertian shading with the Phong approximation for specular reflections

Color shadePhong(HitRecord &hr, const Ray &orig_ray, const BoundingVolumeHierarchy &bvh, const Light &pl, const int &start_matls, const int &start_tex_coords, Vector &hit_point, Vector &new_dir, Color &hit_color)
{
  Color background = Color(0.561f * 3.f, 0.729f * 3.f, 0.988f * 3.f); // hard coded for run-rt
  //alternatively, load the background from trax memory

  //float epsilon = loadf(0, 18);
  float epsilon = 0.001f;

  Ray ray = orig_ray;
  Color retVal(0.f, 0.f, 0.f);
  retVal.A = 0.f;

  Vector normal;

  do
    {
      if(hr.didHit())
	{
	  hit_point = ray.origin() + (hr.minT() * ray.direction());
	  Tri hit_tri = loadTriFromMemory(hr.getID());
	  normal = hit_tri.normal();
	  float costheta = Dot(normal, ray.direction());
	  if(costheta > 0.0f)
	    normal = -normal;
	  
	  hit_point = hit_point + (normal * epsilon); // offset to prevent self-shadowing
	  //-1200, 150, 90
	  Color tempColor(0.f, 0.f, 0.f);
	  
	  Color lightColor;
	  Vector lightDir;
	  float lightDistance = pl.getLight(lightColor, lightDir, hit_point);
	  float cosphi = Dot(normal, lightDir);
	  int inLightLOS = 0;
	  
	  if(cosphi > 0.0f)
	    { 
	      Ray shadowRay(hit_point, lightDir);
	      Color shadowSurfaceColor;
	      do // let the shadow ray continue through transparent textures
		{
		  HitRecord shadowHit(lightDistance);
		  bvh.intersect(shadowHit, shadowRay);
		  // if the shadow ray doesn't intersect the scene
		  if(!shadowHit.didHit())
		    {
		      inLightLOS = 1;
		      tempColor += (lightColor * cosphi * 0.7f);
		      break;
		    }
		  int shadowMatID = loadi(shadowHit.getID() + 10);
		  shadowMatID = start_matls + shadowMatID * 25;
		  int diffuse_texture = loadi(shadowMatID, 15);
		  if(diffuse_texture >=0)
		    shadowSurfaceColor = getTextureColor(shadowHit, start_tex_coords, diffuse_texture);
		  
		  // if the material is completely transparent, pass the shadow ray along
		  if(shadowSurfaceColor.A == 0.f)
		    {
		      Vector shadow_hit_point = shadowRay.origin() + (shadowHit.minT() * shadowRay.direction());
		      shadowRay.org = shadow_hit_point + (shadowRay.dir * epsilon);
		    }
		  else
		    break;
		}
	      while(true);
	    }
	  
	  /* phong term */
	  if(inLightLOS)
	    {
	      Vector H = lightDir - ray.direction();
	      H.normalize();
	      float cosalpha = Dot(H, normal);
	      if(cosalpha > 0.f)
		{
		  float exp = cosalpha * cosalpha; // cosalpha ^ 2
		  exp *= exp; // cosalpha ^ 4
		  exp *= exp; // cosalpha ^ 8
		  exp *= exp; // cosalpha ^ 16
		  exp *= exp; // cosalpha ^ 32
		  exp *= exp; // cosalpha ^ 64
		  // either 64 or 128 should be good
		  tempColor = tempColor + (lightColor * exp);
		}
	    }
	  /* ---------- */

	  int shader = loadi(hr.getID() + 10);
	  shader = start_matls + shader * 25; // mats are 25 words
	  
	  int diffuse_texture = loadi(shader, 15);
	  
	  if(diffuse_texture >= 0) // just do diffuse for now
	    hit_color = getTextureColor(hr, start_tex_coords, diffuse_texture);
	  else
	    hit_color = loadColorFromMemory(shader);
	  
	  tempColor *= hit_color;
	  tempColor.A = hit_color.A;

	  retVal = retVal.blend(tempColor);
	  
	  if(tempColor.A < 1.f)
	    {
	      hr = HitRecord();
	      ray.org = (hit_point - (normal * epsilon)) + (ray.dir * epsilon);
	      bvh.intersect(hr, ray);
	    }
	  else
	    break;
	}
      else
	{
	  retVal = retVal.blend(background);
	  break;
	}
    }
  while(true);
  new_dir = randomReflection(normal); // set up the next ray
  return retVal;
}

#endif
