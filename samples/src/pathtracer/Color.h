
#ifndef Color_h
#define Color_h

#include "trax.hpp"

class Color {
 public:
  Color() {
    R = 1.f;
    G = 1.f;
    B = 1.f;
    A = 1.f;
  }

  Color(const Color& copy) {
    R = copy.R;
    G = copy.G;
    B = copy.B;
    A = copy.A;
  }

  Color& operator=(const Color& copy) {
    R = copy.R;
    G = copy.G;
    B = copy.B;
    A = copy.A;
    return *this;
  }

  float r() const {
    return R;
  }
  float g() const {
    return G;
  }
  float b() const {
    return B;
  }
  float a() const {
    return A;
  }

  Color(float r, float g, float b) {
    R = r;
    G = g;
    B = b;
    A = 1.f;
  }
  Color(float r, float g, float b, float a) {
    R = r;
    G = g;
    B = b;
    A = a;
  }

  Color operator+(const Color& c) {
    return Color(R + c.R, G+c.G, B+c.B);
  }
  Color& operator+=(const Color& c) {
    R += c.R; G += c.G; B += c.B;
    return *this;
  }
  Color operator-(const Color& c) {
    return Color(R - c.R, G-c.G, B-c.B);
  }
  Color& operator-=(const Color& c) {
    R -= c.R; G -= c.G; B -= c.B;
    return *this;
  }
  Color operator*(const Color& c) {
    return Color(R * c.R, G*c.G, B*c.B);
  }
  Color& operator*=(const Color& c) {
    R *= c.R; G *= c.G; B *= c.B;
    return *this;
  }
  Color operator*(float s) const{
    return Color(R*s, G*s, B*s);
  }
  Color operator/(float s) const{
    return Color(R/s, G/s, B/s);
  }
  Color& operator*=(float s) {
    R *= s; G *= s; B *= s;
    return *this;
  }
  Color& operator/=(float s) {
    R /= s; G /= s; B /= s;
    return *this;
  }

 inline Color blend(Color dst)
  {
    float out_a = this->a() + dst.a() * (1.f-this->a());
    if(out_a == 0.f)
      {
	return Color(0.f, 0.f, 0.f, 0.f);
      }
    Color out = (*this * this->a() + dst * dst.a() * (1.f - this->a())) / out_a;
    out.A = out_a;
    return out;
  }

 inline float luminance()
 {
   return 0.299f * R + 0.587f * G + 0.114f * B;
 }
 
 float R, G, B, A;
};

// load the Kd term from the material
inline Color loadColorFromMemory(int addr)
{
  return Color(loadf(addr, 4), loadf(addr, 5), loadf(addr, 6));
}

// load the Kd term from the material
inline Color loadTextureColorFromMemory(int addr, int byteCount)
{ 
  if(byteCount == 1) // grayscale
    {
      float gray = loadi(addr, 0) / 255.f;
      return Color(gray, gray, gray);
    }
  else if(byteCount == 3) // rgb
    return Color(loadi(addr, 0) / 255.f, loadi(addr, 1) / 255.f, loadi(addr, 2) / 255.f);
  else if (byteCount == 4) // rgba
    return Color(loadi(addr, 1) / 255.f, loadi(addr, 2) / 255.f, loadi(addr, 3) / 255.f, loadi(addr, 0) / 255.f);
  else 
    return Color(0.f, 0.f, 0.f);
}

#endif
