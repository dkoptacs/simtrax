#ifndef LIGHT_H
#define LIGHT_H

#include "Color.h"

#define POINTLIGHT 0
#define DIRECTIONALLIGHT 1
#define DIRECTIONALCONELIGHT 2

class Light{
 public:
  Light();
  Light(const Vector& _position, const Color& color, const int type);
  Light(const Vector& _position, const Color& color, const float _sinTheta);

  float getLight(Color& light_color, Vector& light_direction,
		 const Vector& pos) const;
  
  Vector position;
  Color color;
  int type; 
  float sinTheta; // used to simulate a distant area light
};

Light::Light()
{
  type = POINTLIGHT;
}

Light::Light(const Vector& _position, const Color& color, const int _type)
: position(_position), color(color), type(_type)
{
  if(type == DIRECTIONALLIGHT)
    position.normalize();
}

// Create a directional light that samples on the area of a cone with primary axis _dir
Light::Light(const Vector& _dir, const Color& color, const float _sinTheta)
: position(_dir), color(color), type(DIRECTIONALCONELIGHT)
{
  sinTheta = _sinTheta; 
  position.normalize();
}

float Light::getLight(Color& light_color, Vector& light_direction,
			   const Vector& hitpos) const
{
  if(type == POINTLIGHT)
    {
      light_color = color;
      Vector dir = position-hitpos;
      float len = dir.normalize();
      light_direction = dir;
      return len;
    }
  else if(type == DIRECTIONALCONELIGHT)
    {
      light_color = color;
      light_direction = randomReflectionCone(position, sinTheta); // position is actually a direction in this case
      return 100000.f;
    }
  else if (type == DIRECTIONALLIGHT)
    {
      light_color = color;
      light_direction = position; // position is actually a direction in this case
      return 100000.f;
    }

  // To appease the compiler, should never get here
  return -1.f;
}

inline Light loadLightFromMemory(int addr)
{
  return Light(Vector(loadf(addr, 0), loadf(addr, 1), loadf(addr, 2)), Color(1.f, 1.f, 1.f), POINTLIGHT);
}

#endif

