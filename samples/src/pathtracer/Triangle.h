#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "trax.hpp"

// Can't call it "Triangle", since the simulator already uses that class name, 
// and the functional simulator (run_rt), needs to compile the simulator in to the executable.
// A proper fix for this would be to use separate namespaces
class Tri
{

 public:
  Tri(){}
  
 inline Tri(const Vector &_p1, const Vector &_p2, const Vector &_p3)
   : p1(_p1), p2(_p2), p3(_p3) {}

#if 0
 Tri operator*()
   {
     //typedef intptr_t int;
     int address = (int)((long long int)this);
     //printf("address = %d\n", address);
     Tri t(loadVectorFromMemory(address), loadVectorFromMemory(address + 3), loadVectorFromMemory(address + 6));
     t.setAddr(address);
     return t; 
   }
#endif

inline Vector normal() const
{
  Vector edge1 = p1 - p3;
  Vector edge2 = p2 - p3;
  Vector n = Cross(edge1, edge2);
  n.normalize();
  return n;
}

 inline void intersect(HitRecord& hit, const Ray& ray) const
 {
   Vector edge1 = p1 - p3;
   Vector edge2 = p2 - p3;
   Vector r1 = Cross( ray.direction(), edge2 );
   float denomenator = Dot( edge1, r1 );

   // save 1.0 / denominator so we can multiply instead of divide
   float inverse = 1.0f / denomenator;
   Vector s = ray.origin() - p3;
   float b1 = Dot( s, r1 ) * inverse;
   if ( b1 < 0.0f || b1 > 1.0f ){
     return;
   }
   
   Vector r2 = Cross( s, edge1 );
   float b2 = Dot( ray.direction(), r2 ) * inverse;
   if ( b2 < 0.0f || ( b2 + b1 ) > 1.0f ){
     return;
   }
   
   float t = Dot( edge2, r2 ) * inverse;
   if(t > 0.0f)
     {
       if(hit.hit(t, addr))
	 hit.setBaryCoords(b1, b2);
     }
 }
 
 // Address of this triangle
inline void setAddr(const int &a)
 {
   addr = a;
 }
 
inline int getAddr()
 {
   return addr;
 }

//private:
  Vector p1;
  Vector p2;
  Vector p3;
  int addr;
  int matl_ID; 

};


inline Tri loadTriFromMemory(int address)
{
  Tri t(loadVectorFromMemory(address), loadVectorFromMemory(address + 3), loadVectorFromMemory(address + 6));
  t.setAddr(address);
  return t;
}

#endif
