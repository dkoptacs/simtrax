#ifndef TEXTURE_H
#define TEXTURE_H

#include "Triangle.h"
#include "BoundingVolumeHierarchy.h"
#include "Vector.h"
#include "Math.h"
#include "trax.hpp"

Color getTextureColor(const HitRecord &hr, const int &start_tex_coords, int texture_address);

inline int getColorAddress(int texture_address, int xres, int byteCount, int i, int j)
{
  return texture_address + 3 + (((i * xres) + j) * byteCount);
}

// Returns the texture color of a given hit point (defined by the hit record)
Color getTextureColor(const HitRecord &hr, const int &start_tex_coords, int texture_address)
{

  int tri_id = loadi(hr.getID(), 9);

  // texture coords
  Vector tc0 = loadVectorFromMemory(start_tex_coords + (9 * tri_id) + 0);
  Vector tc1 = loadVectorFromMemory(start_tex_coords + (9 * tri_id) + 3);
  Vector tc2 = loadVectorFromMemory(start_tex_coords + (9 * tri_id) + 6);

  // barycentric coords
  float a = hr.getB1();
  float b = hr.getB2();
  float c = 1.f - a - b;

  Vector uvw = (tc0 * a) + (tc1 * b) + (tc2 * c);

  // load texture header data
  int t_yres = loadi(texture_address, 0);
  int t_xres = loadi(texture_address, 1);
  int byteCount = loadi(texture_address, 2);

  float x = uvw.X;
  float y = uvw.Y;
  
  // handle wrapping textures
  if(x != (int)x)
    x -= (int)x;
  if(y != (int)y)
    y -= (int)y;
  
  // take care of negative texture coordinates
  if(x < 0)
    x = 1 - -x;
  if(y < 0)
    y = 1 - -y;
  

  // interpolate  
  x *= t_xres;
  y *= t_yres;
  
  if((int)x == t_xres)
    x = 0.f;
  if((int)y == t_yres)
    y = 0.f;

  int ix = Floor(x) % t_xres;
  if(ix<0)
    ix += t_xres;
  int ix1 = (ix+1) % t_xres;
  int iy = Floor(y) % t_yres;
  if(iy<0)
    iy += t_yres;
  int iy1 = (iy+1) % t_yres;
  float fx = x - ix;
  float fy = y - iy;

  Color c00 = loadTextureColorFromMemory(getColorAddress(texture_address, t_xres, byteCount, iy, ix), byteCount);
  Color c01 = loadTextureColorFromMemory(getColorAddress(texture_address, t_xres, byteCount, iy, ix1), byteCount);
  Color c10 = loadTextureColorFromMemory(getColorAddress(texture_address, t_xres, byteCount, iy1, ix), byteCount);
  Color c11 = loadTextureColorFromMemory(getColorAddress(texture_address, t_xres, byteCount, iy1, ix1), byteCount);

  Color cf = c00*(1.f-fx)*(1.f-fy) + c01*fx*(1.f-fy) + c10*(1.f-fx)*fy + c11*fx*fy;

  // Need to return a single color for alpha blending, but ideally we would blend before interpolating
  // average alpha values
  cf.A = (c00.A + c01.A + c10.A + c11.A) / 4.f; 

  return cf;
}

#endif
