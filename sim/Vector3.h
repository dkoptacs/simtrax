
/*****************************************************************************\
 *                                                                           *
 * filename : Vector3.h                                                      *
 * author   : R. Keith Morley                                                *
 * last mod : 10/09/03                                                       *
 *                                                                           *
 * Vector3  - 3D vector class for graphics applications. Uses doubles.       *
 * Vector3f - 3D vector class for data storage purposes. Uses floats.        *
 *                                                                           *
\*****************************************************************************/

#ifndef _VECTOR3_H_
#define _VECTOR3_H_ 1


#include <iostream>
#include <iomanip>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

class Vector3f;

/// A 3D Vector class for graphics applications.  Uses doubles.
/**
   \class Vector3 math/Vector3.h
 */
class Vector3
{
public:

    Vector3() {}
    Vector3(double e0, double e1, double e2) {e[0]=e0; e[1]=e1; e[2]=e2;}
    Vector3(const Vector3& v) {e[0]=v.e[0]; e[1]=v.e[1]; e[2]=v.e[2];}
    Vector3(const Vector3f& v);

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }
    void setX(double a) { e[0] = a; }
    void setY(double a) { e[1] = a; }
    void setZ(double a) { e[2] = a; }

    const Vector3& operator+() const { return *this; }
    Vector3 operator-() const        { return Vector3(-e[0], -e[1], -e[2]); }
    double& operator[](int i)      { assert(i >=0); assert(i<=2); return e[i]; }
    double operator[](int i) const { assert(i >=0); assert(i<=2); return e[i]; }

    Vector3& operator= (const Vector3& v);
    Vector3& operator+=(const Vector3& v2);
    Vector3& operator-=(const Vector3& v2);
    Vector3& operator*=(const Vector3& v2);
    Vector3& operator*=(const double t);
    Vector3& operator/=(const double t);
    Vector3& operator/=(const Vector3& v2);



    double length() const { return sqrt(e[0]*e[0] + e[1]*e[1] + e[2]*e[2]); }
    double squaredLength() const { return e[0]*e[0] + e[1]*e[1] + e[2]*e[2]; }
    double makeUnitVector();
    double normalize();

    inline int indexOfMinAbsComponent() const;
    inline int indexOfMaxAbsComponent() const;
    double minComponent() const { return e[indexOfMinComponent()]; }
    double maxComponent() const { return e[indexOfMaxComponent()]; }
    double maxAbsComponent() const { return e[indexOfMaxAbsComponent()]; }
    double minAbsComponent() const { return e[indexOfMinAbsComponent()]; }
    int indexOfMinComponent() const
    { return (e[0]< e[1] && e[0]< e[2]) ? 0 : (e[1] < e[2] ? 1 : 2); }
    int indexOfMaxComponent() const
    { return (e[0]> e[1] && e[0]> e[2]) ? 0 : (e[1] > e[2] ? 1 : 2); }


    double e[3];
};

/// A 3D vector storage class using floats instead of doubles.
/**
   \class Vector3f math/Vector3.h
 */
class Vector3f
{
public:
    Vector3f() {}
    Vector3f(float x, float y, float z) {e[0]=x; e[1]=y; e[2]=z;}
    Vector3f(const Vector3f& v)
    {e[0]=v.e[0]; e[1]=v.e[1]; e[2]=v.e[2];}
    Vector3f(const Vector3& v)
    {e[0]=static_cast<float>(v.e[0]); e[1]=static_cast<float>(v.e[1]); e[2]=static_cast<float>(v.e[2]);}

    Vector3f& operator=(const Vector3f& v)
    {e[0]=v.e[0]; e[1]=v.e[1]; e[2]=v.e[2]; return *this; }
    Vector3f& operator=(const Vector3& v)
    {e[0]=static_cast<float>(v.e[0]); e[1]=static_cast<float>(v.e[1]); e[2]=static_cast<float>(v.e[2]); return *this; }

    float x() const { return e[0]; }
    float y() const { return e[1]; }
    float z() const { return e[2]; }
    float& operator[](int i)       { assert(i >=0); assert(i<=2); return e[i]; }
    float  operator[](int i) const { assert(i >=0); assert(i<=2); return e[i]; }


    float e[3];
};


inline Vector3::Vector3(const Vector3f& v)
{e[0] = v.e[0]; e[1] = v.e[1]; e[2] = v.e[2];}

inline bool operator==(const Vector3 &t1, const Vector3 &t2) {
   return ((t1[0]==t2[0])&&(t1[1]==t2[1])&&(t1[2]==t2[2]));
}

inline bool operator!=(const Vector3 &t1, const Vector3 &t2) {
   return ((t1[0]!=t2[0])||(t1[1]!=t2[1])||(t1[2]!=t2[2]));
}

inline std::istream& operator>>(std::istream &is, Vector3 &t) {
   is >> t[0] >> t[1] >> t[2];
   return is;
}

inline std::ostream& operator<<(std::ostream &os, const Vector3 &t) {
   os << t[0] << " " << t[1] << " " << t[2];
   return os;
}

inline Vector3 unitVector(const Vector3& v) {
    double k = 1.0f / sqrt(v.e[0]*v.e[0] + v.e[1]*v.e[1] + v.e[2]*v.e[2]);
    return Vector3(v.e[0]*k, v.e[1]*k, v.e[2]*k);
}

inline bool containsNan(const Vector3& v)
{
    for ( int i = 0; i < 3; i++ )
    {
        if ( !(v[i] <= 0) && !(v[i] >= 0) )
        {
            printf("Singularity found: %lf\n", v[i]);
            return true;
        }
    }
    return false;
}

inline bool containsSingularity(const Vector3& v)
{
   for ( int c = 0; c < 3; c++ )
   {
      if ( !(v[c] <= 0) && !(v[c] >= 0) )
         return true;

      if ( v[c] > FLT_MAX || v[c] < -FLT_MAX )
         return true;
   }
   return false;
}

inline double Vector3::makeUnitVector()
{
    double len = sqrt(e[0]*e[0] + e[1]*e[1] + e[2]*e[2]);
    double k = 1.0f / len;
    e[0] *= k; e[1] *= k; e[2] *= k;
    return len;
}

inline double Vector3::normalize()
{
    double len = sqrt(e[0]*e[0] + e[1]*e[1] + e[2]*e[2]);
    double k = 1.0f / len;
    e[0] *= k; e[1] *= k; e[2] *= k;
    return len;
}

inline Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
    return Vector3( v1.e[0] + v2.e[0], v1.e[1] + v2.e[1], v1.e[2] + v2.e[2]);
}

inline Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
    return Vector3( v1.e[0] - v2.e[0], v1.e[1] - v2.e[1], v1.e[2] - v2.e[2]);
}

inline Vector3 operator*(double t, const Vector3 &v) {
    return Vector3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline Vector3 operator*(const Vector3 &v, double t) {
    return Vector3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline Vector3 operator*(const Vector3 &v1, const Vector3& v2) {
  return Vector3(v1.e[0] * v2.e[0], v1.e[1] * v2.e[1], v1.e[2] * v2.e[2]);
}

inline Vector3 operator/(const Vector3 &v, double t) {
    return Vector3(v.e[0]/t, v.e[1]/t, v.e[2]/t);
}

inline Vector3 operator/(const Vector3 &v1, const Vector3& v2) {
    return Vector3(v1.e[0]/v2.e[0], v1.e[1]/v2.e[1], v1.e[2]/v2.e[2]);
}


inline double dot(const Vector3 &v1, const Vector3 &v2) {
    return v1.e[0] *v2.e[0] + v1.e[1] *v2.e[1]  + v1.e[2] *v2.e[2];
}

inline Vector3 cross(const Vector3 &v1, const Vector3 &v2) {
    return Vector3( (v1.e[1]*v2.e[2] - v1.e[2]*v2.e[1]),
                      (v1.e[2]*v2.e[0] - v1.e[0]*v2.e[2]),
                      (v1.e[0]*v2.e[1] - v1.e[1]*v2.e[0]));
}


inline int Vector3::indexOfMinAbsComponent() const
{
   if (fabs(e[0]) < fabs(e[1]) && fabs(e[0]) < fabs(e[2]))
      return 0;
   else if (fabs(e[1]) < fabs(e[2]))
      return 1;
   else
      return 2;
}

inline int Vector3::indexOfMaxAbsComponent() const
{
   if (fabs(e[0]) > fabs(e[1]) && fabs(e[0]) > fabs(e[2]))
      return 0;
   else if (fabs(e[1]) > fabs(e[2]))
      return 1;
   else
      return 2;
}

inline Vector3& Vector3::operator=(const Vector3 &v){
    e[0]  = v.e[0];
    e[1]  = v.e[1];
    e[2]  = v.e[2];
    return *this;
}

inline Vector3& Vector3::operator+=(const Vector3 &v){
    e[0]  += v.e[0];
    e[1]  += v.e[1];
    e[2]  += v.e[2];
    return *this;
}

inline Vector3& Vector3::operator-=(const Vector3& v) {
    e[0]  -= v.e[0];
    e[1]  -= v.e[1];
    e[2]  -= v.e[2];
    return *this;
}
inline Vector3& Vector3::operator*=(const Vector3& v) {
    e[0]  *= v.e[0];
    e[1]  *= v.e[1];
    e[2]  *= v.e[2];
    return *this;
}

inline Vector3& Vector3::operator*=(const double t) {
    e[0]  *= t;
    e[1]  *= t;
    e[2]  *= t;
    return *this;
}

inline Vector3& Vector3::operator/=(const double t) {
    e[0]  /= t;
    e[1]  /= t;
    e[2]  /= t;
    return *this;
}

inline Vector3& Vector3::operator/=(const Vector3& v2) {
    e[0] /= v2.e[0];
    e[1] /= v2.e[1];
    e[2] /= v2.e[2];
    return *this;
}

#endif
