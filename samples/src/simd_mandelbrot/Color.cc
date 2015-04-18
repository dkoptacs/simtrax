#include "Color.h"

Color::Color() 
{
  R = 1.f;
  G = 1.f;
  B = 1.f;
}

Color::Color(const Color& copy) 
{
  R = copy.R;
  G = copy.G;
  B = copy.B;
}

Color& Color::operator=(const Color& copy) 
{
  R = copy.R;
  G = copy.G;
  B = copy.B;
  return *this;
}

float Color::r() const 
{
  return R;
}
float Color::g() const 
{
  return G;
}
float Color::b() const 
{
  return B;
}

Color::Color(float r, float g, float b) 
{
  R = r;
  G = g;
  B = b;
}
