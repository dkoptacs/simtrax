/*
 *  Scene.cc
 *  
 *
 *  Created by Erik Brunvand on 9/12/11.
 *
 *
 *
 */
// Only include stdio for printf on non-trax version
#if TRAX==0
#include <stdio.h>
#endif

#include "trax.hpp" 
#include "Scene.h"
#include "myBVH.h"
#include "Tri.h"

// Constructors and destructors

// default constructor - default values look like Program3
Scene::Scene(void)
  : xres(GetXRes()), yres(GetYRes()), start_fb(GetFrameBuffer()),
    ambient(AMBIENT), Kd(KD), Ka(KA) {
  bground = loadColorFromMem(BACKGROUND);    // load the background color
  start_scene = loadi(0, SCENE);             // address of scene start
  start_camera = loadi(0, CAMERA);           // address of camera components
  start_light = loadi(0, LIGHT);             // address of light vector
  light = PointLight(loadPtFromMem(start_light), WHITE); // one white light
  start_matls = loadi(0, MATERIALS);         // address of materials start
  start_triangles = loadi(0, TRIANGLES);     // address of triangles start
  num_triangles = loadi(0, NUM_TRIS);        // number of triangles 
}

Scene::~Scene(void) {}	  		     // destructor (needed?)
  

// Member functions
// render all the elements of the scene with shadows
void Scene::render(Image& frame, const PinholeCamera& camera, myBVH& bvh){

  Ray ray;         // Make a ray to re-use for ray casting

  // print out the parts of the scene to make sure they look reasonable
#if TRAX==0
  printf("----------------------------------\n");
  printf("Data as my program reads it.... \n");
  printf("----------------------------------\n");
  printf("scene data... \n");
  printf("fb_start = %d\n", start_fb); 
  printf("bground = %f, %f, %f\n", bground.r, bground.g, bground.b);
  printf("start_scene = %d\n", start_scene); 
  printf("start_light = %d\n", start_light);
  printf("light = %f, %f, %f position and %f, %f, %f color\n", light.getPos().x, light.getPos().y, light.getPos().z,
	 light.getColor().r, light.getColor().g, light.getColor().b); 
  printf("start_matls = %d\n", start_matls); 
  printf("start_triangles = %d\n", start_triangles);
  printf("num_triangles = %d\n", num_triangles);
  printf("Ambient = %f, %f, %f\n", ambient.r, ambient.g, ambient.b);
  printf("kd and ka are %f, %f\n", Kd, Ka);
  printf("Camera details.... \n"); 
  printf("start_camera = %d\n", start_camera); 
  printf("Camera eye = %f,%f, %f\n", camera.getEye().x,camera.getEye().y,camera.getEye().z);
  printf("Camera gaze = %f,%f, %f\n", camera.getLdir().x,camera.getLdir().y,camera.getLdir().z);
  printf("Camera up = %f,%f, %f\n", camera.getUp().x,camera.getUp().y,camera.getUp().z);
  printf("Camera u = %f,%f, %f\n", camera.getU().x,camera.getU().y,camera.getU().z);
  printf("Camera v = %f,%f, %f\n", camera.getV().x,camera.getV().y,camera.getV().z); 
  #endif

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

    // walk through the BVH to find the hit
    bvh.walk(ray, hit, start_triangles);
    
    if (hit.didHit()) { // you hit something, so shade hit point according to Slide 34
      resultC = ambient;                     // Start with ambient light component
      resultC *= Ka;                         // scale by ambient scale factor
      Tri t(hit.getObjAddr());               // load up the triangle you hit
      int matl_id = t.getMaterial();         // ID of the material of this triangle
      int matl_addr = start_matls + (matl_id * 25); // get the address of the material
      ColorRGB objColor = loadColorFromMem(matl_addr + 4); // load the triangle's color from global mem
      Point3D HitPoint = ray.org + hit.getTmin() * ray.dir; // Compute actual hit point
      Normal HPnorm = t.normal(HitPoint);    // normal at HitPoint on triangle (normalized) 
      float costheta = Dot(HPnorm, ray.dir); // cos of angle between normal and ray dir
      if (costheta >  0.0f) HPnorm = -HPnorm;// Invert normal if needed - get normal on same side as ray
      HitPoint += (HPnorm * OFFSET);         // offset HitPoint by small amount 
      ColorRGB l_color = light.getColor();   // color of this light
      Vector3D l_dir = light.getPos() - HitPoint; // direction to this light
      float l_dist = l_dir.normalize();    // normalize returns old length
      float cosphi = Dot(HPnorm, l_dir);   // cos of angle between hp normal and light
      if (cosphi > 0) {                    // if on the right side of the object
		  
	// now loop through all the objects again to cast shadow rays at them
	// This will need a new ray and a new HitRecord...
	// If you didn't hit anything before the light, then add in that light's contribution
	Ray s_ray(HitPoint, l_dir);        // shadow ray from hitpoint, in l_dir direction
	HitRecord s_hit(l_dist);           // new HitRecord with l_dist as Tmax
	bvh.walkS(s_ray, s_hit, start_triangles); // use the shadow version - returns after first hit
	if (!s_hit.didHit()) {             // if no hit, add light to result (not in shadow)
	  l_color *= (cosphi * Kd);        // Local diffuse contribution from this light
	  resultC += l_color;              // Add it in to the accumulated light color
	} // end if no hit (i.e. not in shadow)
      } // end if cosphi > 0
    
      resultC *= objColor; // multiply result color by object color for final color (slide 31)
    } // end object hit code for this pixel
    frame.set(j, i, resultC); // write the result color for this pixel into the frame
  } // end loop through all pixels
} // End render


