
#ifndef Vector_h
#define Vector_h

#include "trax.hpp"
#include "Math.h"
class Vector {
 public:
  float X, Y, Z;
  Vector() {
  }
  Vector(const float &x, const float &y, const float &z) {
    X = x; Y = y; Z = z;
  }

  Vector(const float &s) {
    X = s; Y = s; Z = s;
  }

  Vector(const Vector& copy) {
    X = copy.X;
    Y = copy.Y;
    Z = copy.Z;
  }

  
  Vector& operator=(const Vector& copy) {
    X = copy.X;
    Y = copy.Y;
    Z = copy.Z;
    return *this;
  }


  float x() const {
    return X;
  }
  float y() const {
    return Y;
  }
  float z() const {
    return Z;
  }

  Vector operator+(const Vector& v) const {
    return Vector(X+v.X, Y+v.Y, Z+v.Z);
  }
  Vector operator-(const Vector& v) const {
    return Vector(X-v.X, Y-v.Y, Z-v.Z);
  }
  Vector operator-() const {
    return Vector(-X, -Y, -Z);
  }
  Vector& operator+=(const Vector& v) {
    X+=v.X; Y+=v.Y; Z+=v.Z;
    return *this;
  }
  Vector& operator-=(const Vector& v) {
    X-=v.X; Y-=v.Y; Z-=v.Z;
    return *this;
  }

  Vector operator*(const Vector& v) const {
    return Vector(X*v.X, Y*v.Y, Z*v.Z);
  }
  Vector operator*(float s) const {
    return Vector(X*s, Y*s, Z*s);
  }
  Vector& operator*=(const Vector& v) {
    X*=v.X; Y*=v.Y; Z*=v.Z;
    return *this;
  }
  Vector& operator*=(float s) {
    X*=s; Y*=s; Z*=s;
    return *this;
  }
  Vector operator/(float s) const {
    float inv_s = 1.f/s;
    return Vector(X*inv_s, Y*inv_s, Z*inv_s);
  }

  inline Vector operator/( Vector const &right ) const {
    return Vector( X / right.X, Y / right.Y, Z / right.Z );
  }

  Vector& operator/=(float s) {
    float inv_s = 1.f/s;
    X*=inv_s; Y*=inv_s; Z*=inv_s;
    return *this;
  }

  inline Vector vecMin( Vector const &right ) const {
    return Vector( min( X, right.X ), min( Y, right.Y ), min( Z, right.Z ) );
  }
  
  inline Vector vecMax( Vector const &right ) const {
    return Vector( max( X, right.X ), max( Y, right.Y ), max( Z, right.Z ) );
  }
  
  float length() const {
    return sqrt(X*X+Y*Y+Z*Z);
  }

  // length squared (faster if you just need <, > comparison)
  float length2() const { 
    return X*X+Y*Y+Z*Z;
  }

  float normalize() {
    float l = length();
    float scale = 1.f/l;
    *this *= scale;
    return l;
  }

  static Vector zero() {
    return Vector(0.f,0.f,0.f);
  }
  static Vector one() {
    return Vector(1.f,1.f,1.f);
  }


};

inline Vector loadVectorFromMemory(const int &address)
{
  return Vector(loadf(address, 0), loadf(address, 1), loadf(address, 2));
}

inline Vector operator*(float s, const Vector& v) {
  return Vector(s*v.x(), s*v.y(), s*v.z());
}

inline float Dot(const Vector& v1, const Vector& v2)
{
  return v1.x()*v2.x() + v1.y()*v2.y() + v1.z()*v2.z();
}

inline Vector Cross(const Vector& v1, const Vector& v2)
{
  return Vector(v1.y()*v2.z() - v1.z()*v2.y(),
                v1.z()*v2.x() - v1.x()*v2.z(),
                v1.x()*v2.y() - v1.y()*v2.x());
}


// Returns a vector that is orthogonal to v
Vector getOrtho(Vector const &v)
{
  Vector axis(0.0f, 0.0f, 0.0f);
  float ax = fabs(v.X);
  float ay = fabs(v.Y);
  float az = fabs(v.Z);

  if(ax < ay && ax < az)
    { axis = Vector(1.0f, 0.0f, 0.0f); }
  else if (ay < az)
    { axis = Vector(0.0f, 1.0f, 0.0f); }
  else
    { axis = Vector(0.0f, 0.0f, 1.0f); }
  return Cross(v, axis);
}

// Returns a a random vector in the hemisphere defined by primary axis v
// Random direction is weighted by the cosine of the angle between v and the returned vector
Vector randomReflection(const Vector &v)
{

  Vector refDir(0.0f, 0.0f, 0.0f);
  // pick random point on disk [-1, 1]
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float x_2 = 0.0f; // squares
  float y_2 = 0.0f;
  do 
    {
      x = trax_rand(); 
      y = trax_rand();
      x = x * 2.0f;
      x = x - 1.0f;
      y = y * 2.0f;
      y = y - 1.0f;
      x_2 = x * x;
      y_2 = y * y;
    }
  while((x_2 + y_2) >= 1.0f); // cut out points outside the disk
  
  z = sqrt(1.0f - x_2 - y_2); 
  Vector X = getOrtho(v);
  Vector Y = Cross(v, X);

  refDir = (X * x) + (Y * y) + (v * z);
  refDir.normalize();
  
  return refDir;
}

// Returns a a random vector in the cone defined by primary axis v and angle theta (input is sine of theta).
// Random direction is weighted by the cosine of the angle between v and the returned vector
Vector randomReflectionCone(const Vector &v, const float sinTheta)
{
  Vector refDir(0.0f, 0.0f, 0.0f);
  // pick random point on disk [-1, 1]
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float x_2 = 0.0f; // squares
  float y_2 = 0.0f;
  do 
    {
      x = trax_rand(); 
      y = trax_rand();
      x = x * 2.0f;
      x = x - 1.0f;
      y = y * 2.0f;
      y = y - 1.0f;
      x_2 = x * x;
      y_2 = y * y;
    }
  while((x_2 + y_2) >= 1.0f); // cut out points outside the disk
  
  x *= sinTheta;
  y *= sinTheta;

  z = sqrt(1.0f - x_2 - y_2); 
  Vector X = getOrtho(v);
  Vector Y = Cross(v, X);

  refDir = (X * x) + (Y * y) + (v * z);
  refDir.normalize();
  
  return refDir;
}


#endif
