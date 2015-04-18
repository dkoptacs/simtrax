
#ifndef Color_h
#define Color_h

#include "trax.hpp"

// A barebones Color class for the basic samples
class Color {
 public:
  Color();
  Color(const Color& copy);
  Color& operator=(const Color& copy);

  float r() const;
  float g() const;
  float b() const;

  Color(float r, float g, float b);

  
  float R, G, B;
};




#endif
