#ifndef VEC_HPP
#define VEC_HPP

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


#endif //VEC_HPP
