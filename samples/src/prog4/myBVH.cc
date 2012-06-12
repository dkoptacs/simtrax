/*
 *  myBVH.cc
 *  
 *
 *  Created by Erik Brunvand on 10/01/11.
 *
 *
 */
// Only include stdio for printf on non-trax version
#if TRAX==0
#include <stdio.h>
#endif

#include "myBVH.h"
#include "Box.h"
#include "Tri.h"
#include "constants.h"

// Constructors and destructors
// default constructor - load scene start address from global memory
myBVH::myBVH(void)
  : start_bvh(loadi(0, SCENE)) {}

// init with a specific starting address
myBVH::myBVH(const int& startin)
  : start_bvh(startin) {}

// destructor (needed?)
myBVH::~myBVH(void) {}	     
  
// Member functions
// Walk the tree, return hit in hit record
// Simple for now - optimize later... 
void myBVH::walk(const Ray& ray, HitRecord& hr, const int& start_tris){
  int stack[STACK_DEPTH];           // holds node IDs
  int node_id = 0;                  // current node
  int sp = 0;                       // stack pointer
  while(true) {                     // repeat until you "break"
    int node_addr = start_bvh + (node_id * 8); // address of current BVH node
    Box b(node_addr);               // get the current node's data from global memory
    if (b.intersectP(ray)) {        // ray hits this BVH node
      node_id = b.getChild();       // id of left child (or triangle ID for leaf node)
      int num_children = b.getNum_Prim(); // number of primitives
      if (num_children < 0 ) {      // -1 means curent node is an interior node
	stack[sp++] = node_id +1;   // push the ID of the right child
	continue;                   // go back to "while (true)" 
      } else {                      // node is a leaf node
	for (int i = 0; i < num_children; i++) { // loop through all triangles in leaf
	  Tri t(node_id + (i * 11));// load the triangle from global mem
	  t.intersects(ray, hr);    // check it for intersection
      // hit record will record the address of the triangle that you hit...
	} // end "for"
      } //end "else" 
    } //end "if bit.didHit()"
    if (sp == 0) break;             // if there are no more nodes, break
    node_id = stack[--sp];          // otherwise pop one off the stack and keep going
  }// end "while true"
}// end walk

// Walk the tree, return hit in hit record
// Simple for now - optimize later...
// This is a shadow version - returns after the first hit that is less 
// than the light distance. The HitRecord hr will be loaded with the
// distance to the light source. 
void myBVH::walkS(const Ray& ray, HitRecord& hr, const int& start_tris){
  int stack[STACK_DEPTH];           // holds node IDs
  int node_id = 0;                  // current node
  int sp = 0;                       // stack pointer
  while(true) {                     // repeat until you "break"
    int node_addr = start_bvh + (node_id * 8); // address of current BVH node
    Box b(node_addr);               // get the current node's data from global memory
    if (b.intersectP(ray)) {        // ray hits this BVH node
      node_id = b.getChild();       // id of left child (or triangle ID for leaf node)
      int num_children = b.getNum_Prim(); // number of primitives
      if (num_children < 0 ) {      // -1 means curent node is an interior node
	stack[sp++] = node_id +1;   // push the ID of the right child
	continue;                   // go back to "while (true)" 
      } else {                      // node is a leaf node
	for (int i = 0; i < num_children; i++) { // loop through all triangles in leaf
	  Tri t(node_id + (i * 11));// load the triangle from global mem
	  t.intersects(ray, hr);    // check it for intersection
	  if (hr.didHit()) break;   // return if there's any hit that's less than light distance
	} // end "for"
      } //end "else" 
    } //end "if bit.didHit()"
    if (sp == 0) break;             // if there are no more nodes, break
    node_id = stack[--sp];          // otherwise pop one off the stack and keep going
  }// end "while true"
}// end walkS


