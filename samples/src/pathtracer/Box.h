#ifndef BOX_H
#define BOX_H
#include "Vector.h"
#include "trax.hpp"
class Box
{
 public:
  Vector c1; // min corner
  Vector c2; // max corner
  
  Box()
    {}
  
  inline Box(const Vector &_c1, const Vector &_c2)
    {
      c1 = _c1;
      c2 = _c2;
    }
  
  
  Box& operator=(const Box& copy) {
    c1 = copy.c1;
    c2 = copy.c2;
    return *this;
  }
  
  inline float intersect(HitRecord& hit, const Ray& ray) const
  {
    float tnear;
    Vector inv = Vector( 1.0f ) / ray.direction();
    Vector p1 = ( c1 - ray.origin() ) * inv;
    Vector p2 = ( c2 - ray.origin() ) * inv;
    Vector mins = p1.vecMin( p2 );
    Vector maxs = p1.vecMax( p2 );
    tnear = max( mins.x(), max( mins.y(), mins.z() ) );
    float t2 = min( maxs.x(), min( maxs.y(), maxs.z() ) );

    if(tnear < t2)
      {
	if(!hit.hit(tnear, 0))
	  {
	    hit.hit(t2, 0, true); // hit occured on the inside of the box
	  }
      }
    return hit.minT();
  }
};

inline Box loadBoxFromMemory(const int &address)
{
  return Box(loadVectorFromMemory(address), loadVectorFromMemory(address + 3));
}

#endif

