#ifndef BOUNDING_VOLUME_HIERARCHY_OPT_H
#define BOUNDING_VOLUME_HIERARCHY_OPT_H

#include "trax.hpp"
#include "Box.h"
#include "Triangle.h"

struct StackNode
{
  float dist;
  int ID;
  bool inside;
  StackNode()
    : dist(0.f), ID(0), inside(false) 
  {}
};

class BoundingVolumeHierarchy {
 public:
  
  BoundingVolumeHierarchy(const int &_start_scene)
    {
      start_bvh = _start_scene;
    }

  inline void intersect(HitRecord& hit, const Ray& ray) const
  {
    // test the scene bounds
    Box root = loadBoxFromMemory(start_bvh);
    HitRecord rootHit;
    root.intersect(rootHit, ray);
    if(!rootHit.didHit())
      return;
    
    StackNode stack[32];
    int node_id = 0;
    int sp = 0;
    while(true)
    {
      int node_addr = start_bvh + node_id * 8;
      int num_children = loadi( node_addr, 6 );
      int left_ID = loadi(node_addr, 7);
      if(num_children < 0) // interior node
	{
	  int right_ID = left_ID + 1;

	  Box left = loadBoxFromMemory(start_bvh + (left_ID * 8));
	  Box right = loadBoxFromMemory(start_bvh + (right_ID * 8));
	  
	  HitRecord boxHitL, boxHitR;
	  float leftDist = left.intersect(boxHitL, ray);
	  float rightDist = right.intersect(boxHitR, ray);

	  if(boxHitL.didHit() || boxHitR.didHit())
	    {
	      // traverse closer node first to increase chance of culling branches
	      if(leftDist < rightDist) // left node is closer
		{
		  node_id = left_ID;
		  if(boxHitR.didHit())
		    {
		      stack[ sp ].dist = boxHitR.minT();
		      stack[ sp ].inside = boxHitR.didHitInside();
		      stack[ sp ].ID = right_ID;
		      sp++;
		    }
		}
	      else // right node is closer
		{
		  node_id = right_ID;
		  if(boxHitL.didHit())
		    {
		      stack[ sp ].dist = boxHitL.minT();
		      stack[ sp ].inside = boxHitL.didHitInside();
		      stack[ sp ].ID = left_ID;
		      sp++;
		    }
		}
	      continue;
	    }
	}
      else // leaf node
	{
	  node_addr = left_ID;
	  for ( int i = 0; i < num_children; ++i)
	    {
	      Tri t = loadTriFromMemory(node_addr);
	      t.intersect(hit, ray);
	      node_addr = node_addr + 11;
	    }
        }
  
      // get next node ID that needs to be traversed from stack
      do
	{
	  if( --sp < 0 ) // stack is empty
	    return;
	  node_id = stack[ sp ].ID;
	}
      while(!stack[ sp ].inside && stack[ sp ].dist > hit.minT()); // ignore node if we found a closer hit

    }
  }

 private:
  int start_bvh;
  
};
#endif
