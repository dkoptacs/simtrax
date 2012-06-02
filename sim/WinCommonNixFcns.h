// define some Linux/Unix functions not present under Win :D
// Konstantin Shkurko (kshkurko@cs.utah.edu)
#ifndef COMMONFCNS_H_
#define COMMONFCNS_H_

#if defined(WIN32) && TRAX==0

#include <math.h>
#include <stdlib.h>      // used later for ppm writer
#include <stdio.h>
#include <time.h>        // used to seed random number generator

// random number function
inline float drand48() {
    return( rand() * (1.f/RAND_MAX) );
}


// log base 2
inline float log2( const float &val ) {
    return( log(val) * (1.f/log(2.0f)) );
}


#endif // WIN32

#endif // COMMONFCNS_H_