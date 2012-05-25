#ifndef RAY_CAMERA_H
#define RAY_CAMERA_H

#include "Vector.h"
#include "Ray.h"

#define PINHOLE 0
#define THINLENS 1

// Implements either a pinhole camera or a thin lens camera. 
// If the constructor is passed a focal distance, will use the thin lens model

class RayCamera{
 public:
  RayCamera()
    {}
  RayCamera(int addr);
  RayCamera(int addr, float distance);
  RayCamera(const Vector& eye, const Vector& lookat, const Vector& up, float ulen, float aspect_ratio);
  void makeRay(Ray& ray, float x, float y) const;

 private:
  Vector eye;
  Vector lookat;
  Vector up;
  float ulen;
  float aspect_ratio;
  float radius;
  float focal_distance;
  int type;

  Vector u;
  Vector v;
  Vector unit_u;
  Vector unit_v;
  Vector lookdir;
};

inline RayCamera::RayCamera(int addr)
{
  eye = loadVectorFromMemory(addr);
  up = loadVectorFromMemory(addr + 9);
  lookdir = loadVectorFromMemory(addr + 12);
  u = loadVectorFromMemory(addr + 15);
  v = loadVectorFromMemory(addr + 18);
  type = PINHOLE;
}

inline RayCamera::RayCamera(int addr, float distance)
{
  eye = loadVectorFromMemory(addr);
  up = loadVectorFromMemory(addr + 9);
  lookdir = loadVectorFromMemory(addr + 12);
  u = loadVectorFromMemory(addr + 15);
  v = loadVectorFromMemory(addr + 18);
  unit_u = loadVectorFromMemory(addr + 21);
  unit_v = loadVectorFromMemory(addr + 24);
  radius = loadf(addr + 27);
  focal_distance = distance;
  type = THINLENS;
}

inline RayCamera::RayCamera(const Vector& eye, const Vector& lookat, const Vector& up,
                             float ulen, float aspect_ratio)
: eye(eye), lookat(lookat), up(up), ulen(ulen), aspect_ratio(aspect_ratio)
{
  lookdir = lookat-eye;
  lookdir.normalize();
  u = Cross(lookdir, up);
  v = Cross(u, lookdir);
  u.normalize();
  u *= ulen;
  float vlen = ulen/aspect_ratio;
  v.normalize(); 
  v *= vlen;
}

inline void RayCamera::makeRay(Ray& ray, float x, float y) const
{
  Vector direction = lookdir+u*x+v*y;
  direction.normalize();
  if(type == PINHOLE)
    ray = Ray(eye, direction);
  if(type == THINLENS)
    {
      float x = trax_rand();
      float y = trax_rand();
      direction = direction * focal_distance;
      Vector lensU = x * radius * unit_u;
      Vector lensV = y * radius * unit_v;
      direction = direction - (lensU + lensV);
      Vector origin = eye + lensU + lensV;
      direction.normalize();
      ray = Ray(origin, direction);
    }
}

#endif
