#ifndef TRAX_HPP
#define TRAX_HPP

#define usei(x) x
#define usef(x) x

extern "C"
{
  extern int loadi( int base, int offset = 0 ) asm("llvm.trax.loadi");
  extern float loadf( int base, int offset = 0 ) asm("llvm.trax.loadf");
  extern void profile( int prof_id ) asm("llvm.trax.profile");
//     extern int loadi( int base, int offset = 0 ) __attribute__ ((pure));
//     extern float loadf( int base, int offset = 0 ) __attribute__ ((pure));
    extern int loadl1( int base, int offset = 0 ) __attribute__ ((pure));
  extern int atomicinc( int location) asm("llvm.trax.atominc");
  extern void storei( int value, int base, int offset = 0 ) asm("llvm.trax.storei");
  extern void storef( float value, int base, int offset = 0 ) asm("llvm.trax.storef");
//     extern void storei( int value, int base, int offset = 0 );
//     extern void storef( float value, int base, int offset = 0 );
  extern float min( float left, float right ) asm("llvm.trax.min");
  extern float max( float left, float right ) asm("llvm.trax.max");
//     extern float min( float left, float right ) __attribute__ ((const));
//     extern float max( float left, float right ) __attribute__ ((const));
//    extern float invsqrt( float value ) __attribute__ ((const));
  extern float invsqrt( float value ) asm("llvm.trax.invsqrt");
  extern void printi(int value) asm("llvm.trax.printi");
//     extern void printi( int value );
  extern void printf( float value ) asm("llvm.trax.printf");
  //    extern void printf( float value );
  // removed because they're not needed in the new compiler
//     extern void usei( int value );
//     extern void usef( float value );
}

struct vec {
    float x, y, z;
    inline vec( float x, float y, float z ) : x( x ), y( y ), z( z ) {}
    inline vec( float v ) : x( v ), y( v ), z( v ) {}
    inline vec() : x( 0.0f ), y( 0.0f ), z( 0.0f ) {}
    inline vec operator+( vec const &right ) const {
        return vec( x + right.x, y + right.y, z + right.z );
    }
    inline vec operator-( vec const &right ) const {
        return vec( x - right.x, y - right.y, z - right.z );
    }
    inline vec operator*( vec const &right ) const {
        return vec( x * right.x, y * right.y, z * right.z );
    }
    inline vec operator*( float right ) const {
        return vec( x * right, y * right, z * right );
    }
    inline friend vec operator*( float left, vec right ) {
        return vec( left * right.x, left * right.y, left * right.z );
    }
    inline vec operator/( vec const &right ) const {
        return vec( x / right.x, y / right.y, z / right.z );
    }
    inline vec operator/( float right ) const {
        return vec( x / right, y / right, z / right );
    }
    inline vec min( vec const &right ) const {
        return vec( ::min( x, right.x ), ::min( y, right.y ), ::min( z, right.z ) );
    }
    inline vec max( vec const &right ) const {
        return vec( ::max( x, right.x ), ::max( y, right.y ), ::max( z, right.z ) );
    }
    inline float dot( vec const &right ) const {
        return x * right.x + y * right.y + z * right.z;
    }
    inline vec cross( vec const &right ) const {
        return vec( y * right.z - z * right.y,
                    z * right.x - x * right.z,
                    x * right.y - y * right.x );
    }
    inline float length() const {
        return 1.0f / invsqrt( x * x + y * y + z * z );
    }
    inline vec unit() const {
        float inverse = invsqrt( x * x + y * y + z * z );
        return vec( x * inverse, y * inverse, z * inverse );
    }
};

#endif
