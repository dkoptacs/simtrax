/*
 *  Scene.cc
 *  
 *
 *  Created by Erik Brunvand on 9/12/11.
 *
 *
 *
 */
#include "trax.hpp" 
#include "Scene.h"
#include "Image.h"
#include "PinholeCamera.h"
#include "constants.h"

// Constructors and destructors

// uninitialized constructor - default values look like Program2
Scene::Scene(void)
  : xres(GetXRes()), yres(GetYRes()), start_fb(GetFrameBuffer()),
    ambient(ColorRGB(0.6f, 0.1f, 0.1f)), bground(ColorRGB(0.5f, 0.8f, 0.9f)),
    Ka(0.3f), Kd(0.7f) {
  // Set the arays to match program2 
  materials[0] = ColorRGB(0.1f, 0.3f, 0.9f);
  materials[1] = ColorRGB(1.0f, 0.2f, 0.2f);
  materials[2] = ColorRGB(1.0f, 0.9f, 0.1f);
  materials[3] = ColorRGB(0.4f, 0.4f, 0.7f);
  spheres[0] = Sphere(Point3D(1.5f, 3.5f, 4.0f), 2.4f, 0, 0);
  spheres[1] = Sphere(Point3D(-0.5f, -1.5f, 2.0f), 1.8f, 1, 1);
  spheres[2] = Sphere(Point3D(0.5f, 1.0f, 1.0f), 1.0f, 2, 2);
  spheres[3] = Sphere(Point3D(0.5f, 1.0f, -100.0f), 100.0f, 3, 3);
  lights[0] = PointLight(Point3D(-30.0f, -20.0f, 80.0f), ColorRGB(0.7f, 0.9f, 0.9f)); 
  lights[1] = PointLight(Point3D(-20.0f, -50.0f, 40.0f), ColorRGB(0.6f, 0.1f, 0.1f)); 
}

// init some elements
Scene::Scene(const ColorRGB& a, const ColorRGB& b, const float ka, const float kd)
  : xres(GetXRes()), yres(GetYRes()), start_fb(GetFrameBuffer()),
    ambient(a), bground(b), Ka(ka), Kd(kd) {
   // Set the arays to match program2 
  materials[0] = ColorRGB(0.1f, 0.3f, 0.9f);
  materials[1] = ColorRGB(1.0f, 0.2f, 0.2f);
  materials[2] = ColorRGB(1.0f, 0.9f, 0.1f);
  materials[3] = ColorRGB(0.4f, 0.4f, 0.7f);
  spheres[0] = Sphere(Point3D(1.5f, 3.5f, 4.0f), 2.4f, 0, 0);
  spheres[1] = Sphere(Point3D(-0.5f, -1.5f, 2.0f), 1.8f, 1, 1);
  spheres[2] = Sphere(Point3D(0.5f, 1.0f, 1.0f), 1.0f, 2, 2);
  spheres[3] = Sphere(Point3D(0.5f, 1.0f, -100.0f), 100.0f, 3, 3);
  lights[0] = PointLight(Point3D(-30.0f, -20.0f, 80.0f), ColorRGB(0.7f, 0.9f, 0.9f)); 
  lights[1] = PointLight(Point3D(-20.0f, -50.0f, 40.0f), ColorRGB(0.6f, 0.1f, 0.1f)); 
} 

Scene::~Scene(void) {}	  		     // destructor (needed?)
  

// Member functions
// render all the elements of the scene with shadows
void Scene::render(Image& frame, const PinholeCamera& camera){

  Ray ray;         // Make a ray to re-use for ray casting
  
  // 
  // This loop iterates through all the pixels. So, it's really the
  // pixel number as a linear walk through the 2d frame. This, ironically
  // is exactly what the image.set function really wants... but we need the
  // x and y to generate each ray. 
  for(int pix = atomicinc(0); pix < xres*yres; pix = atomicinc(0)) {
    ColorRGB resultC = bground; // default color is background - will be reset if hit
    int i = pix / xres;         // column of the pixel
    int j = pix % xres;         // row of the pixel
    HitRecord hit(FLOAT_MAX);   // Initial HitRecord has FLOAT_MAX as hit value

    // These statements magically compute the x and y
    // components of the direction vector for the ray
    float x = 2.0f * (j - xres/2.f + 0.5f)/xres;
    float y = 2.0f * (i - yres/2.f + 0.5f)/yres;

    // Update the ray to be from the eye point through the (x,y) pixel
    camera.makeRay(ray, x, y); 

    for (int i=0; i<NUMOBJ; i++){            // Now check all of the objects for a hit
      spheres[i].intersects(ray, hit);       // run the intersection test on all spheres
    }
    
    if (hit.didHit()) { // you hit something, so shade hit point according to Slide 34
      resultC = ambient;                     // Start with ambient light component
      resultC *= Ka;                         // scale by ambient scale factor
      Sphere obj  = spheres[hit.getObjId()]; // get the sphere you hit (will be closest)
      ColorRGB objColor = materials[obj.getMat()]; // get material ID (color) of that obj
      Point3D HitPoint = ray.org + hit.getTmin() * ray.dir; // Compute actual hit point
      Normal HPnorm = obj.normal(HitPoint);  // normal at HitPoint (normalized) 
      HitPoint += (HPnorm * EPSILON);        // offset HitPoint by small amount in HPnorm dir
      float costheta = Dot(HPnorm, ray.dir); // cos of angle between normal and ray dir
      if (costheta >  0.0f) HPnorm = -HPnorm;// Invert normal if needed

      // now loop through the lights to gather light info from each light source
      for(int l=0; l<NUMLIGHT; l++){         // loop through the lights
	PointLight this_light = lights[l];   // get the current light
	ColorRGB l_color = this_light.getColor();// color of this light
	Vector3D l_dir = this_light.getPos() - HitPoint; // direction to this light
	float l_dist = l_dir.normalize();    // normalize returns old length
	float cosphi = Dot(HPnorm, l_dir);   // cos of angle between hp normal and light
	if (cosphi > 0) {                    // if on the right side of the object
	  // now loop through all the objects again to cast shadow rays at them
	  // This will need a new ray and a new HitRecord...
	  // If you didn't hit anything before the light, then add in that light's contribution
	  Ray s_ray(HitPoint, l_dir);        // shadow ray from hitpoint, in l_dir direction
	  HitRecord s_hit(l_dist);           // new HitRecord with l_dist as Tmax

	  // cast shadow rays from hit point to light source, check and see if you
	  // hit any other objects in the scene before you get to the hit point. 
	  for (int k=0; k<NUMOBJ; k++){     // loop through all objects again for shadows
	    spheres[k].intersects(s_ray, s_hit);// run the intersection test on the spheres
	  }
	  
	  if (!s_hit.didHit()) {             // if no hit, add light to result (not in shadow)
	    l_color *= (cosphi * Kd);        // Local diffuse contribution from this light
	    resultC += l_color;              // Add it in to the accumulated light color
	  } // end if no hit (i.e. not in shadow)
	} // end if cosphi > 0
      } // end loop through all lights. resultC now has all light source contributions
      resultC *= objColor; // multiply result color by object color for final color (slide 31)
    } // end object hit code for this pixel
    frame.set(j, i, resultC); // write the result color for this pixel into the frame
  } // end loop through all pixels
}


