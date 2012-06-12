/*
 *  constants.h
 *  
 *
 *  Created by Erik Brunvand on 9/10/11.
 *
 * Just a bunch of constants that can be use by various parts of the
 * program. 
 */
#ifndef __constants_h__
#define __constants_h__


// numeric constants
#define PI         3.1415926535897932384f;
#define TWO_PI     6.2831853071795864769f;
#define PI_DIV_180 0.0174532925199432957f;
#define PIinv      0.3183098861837906715f;
#define TWO_PIinv  0.1591549430918953358f;
// This is an all-purpose epsilon. It may be that some functions need
// a different epslion. You can also read epsilon from global memory
#define EPSILON    0.0001f
#define OFFSET     0.01f // an offset in the normal direction for shading

// Some default color definitions
#define BLACK ColorRGB(0.0f)
#define WHITE ColorRGB(1.0f)
#define	RED   ColorRGB(1.0f, 0.0f, 0.0f)
#define GREEN ColorRGB(0.0f, 1.0f, 0.0f)
#define BLUE  ColorRGB(0.0f, 0.0f, 1.0f)

// These are offsets in gloabal memory that point to various addresses that
// need to be loaded.
// Use with "loadi(0, SCENE)" or "loadColorFromMem(BACKGROUND)"
#define SCENE       8
#define MATERIALS   9
#define CAMERA      10
#define BACKGROUND  11
#define LIGHT       12
#define TRIANGLES   28
#define NUM_TRIS    29

// Offsets for use in the BVH nodes
#define MINPT 0
#define MAXPT 3
#define NUM_PRIM 6
#define CHILD 7

// Some defaults for global shading 
#define AMBIENT ColorRGB(0.4f, 0.4f, 0.4f)
#define KD 0.7f
#define KA 0.3f

// Depth of the traversal stack
#define STACK_DEPTH 64

#endif
