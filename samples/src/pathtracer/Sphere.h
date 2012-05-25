
#ifndef SPHERE_H
#define SPHERE_H

#include "Sphere.h"
#include "Ray.h"
#include "Vector.h"
#include "Color.h"
#include "HitRecord.h"
class Ray;

class Sphere {
 public:
  Sphere(const Vector& center, const float radius, const int id, const int mat_id);
  Sphere();
  inline void intersect(HitRecord& hit, const Ray& ray) const;
  inline Vector normal(const Vector &hitPoint) const;
  int matl_id;
  int ID;
 private:
  Vector center;
  float radius;
};



Sphere::Sphere(const Vector& center, const float radius, const int id, const int mat_id)
: center(center), radius(radius), ID(id), matl_id(mat_id)
{
}

Sphere::Sphere()
{
}

inline void Sphere::intersect(HitRecord& hit, const Ray& ray) const
{
  Vector O(ray.origin()-center);
  const Vector& V(ray.direction());
  float b = Dot(O, V);
  float c = Dot(O, O)-radius*radius;
  float disc = b*b-c;
  if(disc > 0.0f)
    {
      float sdisc = sqrt(disc);
      float root1 = (-b - sdisc);
      if(!hit.hit(root1, ID)){
	float root2 = (-b + sdisc);
	hit.hit(root2, ID);
	
      }
    }
}

inline Vector Sphere::normal(const Vector &hitPoint) const
{
  Vector n = hitPoint - center;
  n.normalize();
  return n;
}

#endif
