#ifndef TRAX_HPP
#define TRAX_HPP

#define FLOAT_MAX 1e36

#ifndef TRAX
#define TRAX 0
#endif

#if TRAX // Trax version
#include "trax_trax.hpp"
#else // C++ version
#include "trax_cpp.hpp"
#endif

#define TRAX_XRES                  1
#define TRAX_INV_XRES              2
#define TRAX_F_XRES                3
#define TRAX_YRES                  4
#define TRAX_INV_YRES              5
#define TRAX_F_YRES                6
#define TRAX_START_FB              7
#define TRAX_START_SCENE           8
#define TRAX_START_MATLS           9
#define TRAX_START_CAMERA          10
#define TRAX_START_BG              11
#define TRAX_START_LIGHT           12
#define TRAX_START_PERMUTE         13
#define TRAX_END_MEMORY            14
#define TRAX_MEM_SIZE              15
#define TRAX_RAY_DEPTH             16
#define TRAX_NUM_SAMPLES           17
#define TRAX_EPSILON               18
#define TRAX_START_HAMMERSLEY      19
// unused (deprecated)             20
#define TRAX_NUM_NODES             21
#define TRAX_START_COSTS           22
// unused (deprecated)             23
#define TRAX_START_ALT_BVH         24
// unused (deprecated)             25
#define TRAX_START_SUBTREE_SIZES   26
// unused (deprecated)             27
#define TRAX_START_TRIANGLES       28
#define TRAX_NUM_TRIANGLES         29
#define TRAX_START_TEX_COORDS      30
#define TRAX_START_ZBUFFER         31
#define TRAX_START_PARENT_POINTERS 32
#define TRAX_START_SUBTREE_IDS     33
#define TRAX_NUM_SUBTREES          34
#define TRAX_NUM_TMS               35
#define TRAX_PACK_SPLIT_AXIS       38
#define TRAX_NUM_MATLS             39


// Some useful functions
// sqrt() has issue within MSVC
#ifndef WIN32
inline float sqrt(const float &x){
  return 1.0f / invsqrt(x);
}

#endif
inline int GetXRes() {
  return loadi( 0, TRAX_XRES );
}
inline float GetInvXRes() {
  return loadf( 0, TRAX_INV_XRES );
}
inline int GetYRes() {
  return loadi( 0, TRAX_YRES );
}
inline float GetInvYRes() {
  return loadf( 0, TRAX_INV_YRES );
}
inline int GetFrameBuffer() {
  return loadi( 0, TRAX_START_FB );
}

inline int GetBVH() {
  return loadi( 0, TRAX_START_SCENE );
}
inline int GetMaterials() {
  return loadi( 0, TRAX_START_MATLS );
}
inline int GetNumMaterials() {
  return loadi( 0, TRAX_NUM_MATLS );
}
inline int GetCamera() {
  return loadi( 0, TRAX_START_CAMERA );
}
inline int GetBackground() {
  return loadi( 0, TRAX_START_BG );
}
inline int GetLight() {
  return loadi( 0, TRAX_START_LIGHT );
}

inline int GetStartTriangles() {
  return loadi( 0, TRAX_START_TRIANGLES );
}

inline int GetNumTriangles() {
  return loadi( 0, TRAX_NUM_TRIANGLES );
}

inline int GetThreadID() {
  return trax_getid(1);
}
inline int GetCoreID() {
  return trax_getid(2);
}
inline int GetL2ID() {
  return trax_getid(3);
}

// Macros to undo a work around from the old compiler
// Shouldn't need to use these except on legacy code
#define usei(x) 
#define usef(x) 

#endif
