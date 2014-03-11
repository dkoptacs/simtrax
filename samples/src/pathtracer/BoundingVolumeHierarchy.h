#ifndef BOUNDING_VOLUME_HIERARCHY_H
#define BOUNDING_VOLUME_HIERARCHY_H

#include "trax.hpp"
#include "Box.h"
#include "Triangle.h"

class BoundingVolumeHierarchy {
 public:
  
  BoundingVolumeHierarchy(const int &_start_scene)
    {
      start_bvh = _start_scene;
    }
  BoundingVolumeHierarchy()
    {
      start_bvh = loadi(0, 8);
    }

  inline void intersect(HitRecord& hit, const Ray& ray) const
  {
    
    int stack[32];
    int node_id = 0;
    int sp = 0;
    while(true)
    {
      int node_addr = start_bvh + node_id * 8;
      Box b = loadBoxFromMemory(node_addr);
      HitRecord boxHit;
      b.intersect(boxHit, ray);
      if(boxHit.didHit())
	{
	  // only traverse the node if we haven't found something closer
	  if(boxHit.didHitInside() || (hit.minT() >= boxHit.minT())) 
	    {
	      // get left child id
	      node_id = loadi( node_addr, 7 );
	      int num_children = loadi( node_addr, 6 );
	      if ( num_children < 0 ) // interior node
		{
		  stack[ sp++ ] = node_id + 1; // push right sibling on to stack
		  continue;
		}
	      // leaf node
	      int tri_addr = node_id;
	      for ( int i = 0; i < num_children; ++i)
		{
		  Tri t = loadTriFromMemory(tri_addr);
		  t.intersect(hit, ray);
		  tri_addr = tri_addr + 11;
		}
	    }
	}
      if ( --sp < 0 ) // stack empty?
	{
	  return;
	}
      node_id = stack[ sp ]; // next node is on top of stack
    }
  }

 private:
  int start_bvh;
  
};
#endif
