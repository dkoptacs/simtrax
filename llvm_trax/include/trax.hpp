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

// Trax Intrinsics

  // Main function (user defined)
//   void trax_main();

  // Global Memory
// int loadi( int base, int offset = 0 )
// float loadf( int base, int offset = 0 )
// void storei( int value, int base, int offset = 0 )
// void storef( float value, int base, int offset = 0 )
// int trax_getid( int value = 0 )

  // Arithmetic
// int atomicinc( int location )
// int global_reg_read( int location )
// float min( float left, float right )
// float max( float left, float right )
// float invsqrt( float value )

// float trax_rand()

// Don't use the following unless you really know what you're doing
// If anyone uses these directly, they probably need to be added to 
// trax_cpp.*
// int trax_inc_reset( int reg_num )
// void trax_barrier( int reg_num )
// void trax_write_srf( float reg1, float reg2, float reg3 )
// void trax_write_sri( int reg1, int reg2, int reg3 )
// float trax_callnoise( )

  // Debug
// void profile( int prof_id )
// int loadl1( int base, int offset = 0 )
// void trax_printi(int value)
// void trax_printf( float value )

// Helper functions
// void trax_setup()
// void trax_cleanup()
// void barrier( int reg_num = 5 )
// float trax_noise( float x, float y, float z )

// Some useful functions
inline float sqrt(const float &x){
  return 1.0f / invsqrt(x);
}
inline int GetXRes() {
  return loadi( 0, 1 );
}
inline float GetInvXRes() {
  return loadf( 0, 2 );
}
inline int GetYRes() {
  return loadi( 0, 4 );
}
inline float GetInvYRes() {
  return loadf( 0, 5 );
}
inline int GetFrameBuffer() {
  return loadi( 0, 7 );
}

inline int GetBVH() {
  return loadi( 0, 8 );
}
inline int GetMaterials() {
  return loadi( 0, 9 );
}
inline int GetCamera() {
  return loadi( 0, 10 );
}
inline int GetBackground() {
  return loadi( 0, 11 );
}
inline int GetLight() {
  return loadi( 0, 12 );
}

inline int GetStartTriangles() {
  return loadi( 0, 28 );
}
inline int GetNumTriangles() {
  return loadi( 0, 29 );
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

// More functions to appear (BVH, Tris, etc.)

// Macros to undo a work around from the old compiler
// Shouldn't need to use these except on legacy code
#define usei(x) 
#define usef(x) 

#endif
