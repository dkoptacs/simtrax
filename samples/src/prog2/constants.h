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

// These constants are used in the Scene class to define the arrays that
// hold the objects
#define NUMOBJ  4       // number of objects in the scene
#define NUMMAT  4       // number of materials in the scene
#define NUMLIGHT 2      // number of lights in the scene

#define PI         3.1415926535897932384f;
#define TWO_PI     6.2831853071795864769f;
#define PI_DIV_180 0.0174532925199432957f;
#define PIinv      0.3183098861837906715f;
#define TWO_PIinv  0.1591549430918953358f;
#define EPSILON    0.0001f

#define BLACK ColorRGB(0.0f)
#define WHITE ColorRGB(1.0f)
#define	RED   ColorRGB(1.0f, 0.0f, 0.0f)
#define GREEN ColorRGB(0.0f, 1.0f, 0.0f)
#define BLUE  ColorRGB(0.0f, 0.0f, 1.0f)

#endif
