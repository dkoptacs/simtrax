#include "Camera.h"
#include <iostream>

using namespace simtrax;

Camera::Camera(const Vector3& c,
               const Vector3& gaze,
               const Vector3& vup,
               float aperture,
               float _left, float _right,
               float _bottom, float _top, float distance, float _far) :
  left(_left), right(_right), bottom(_bottom), top(_top), near(distance), far(_far)
{
  unitgaze = unitVector(gaze);
  center = c;
  Vector3 uvw_v = unitVector(vup);
  Vector3 uvw_w = unitVector(-gaze);
  Vector3 uvw_u = unitVector(cross(uvw_v, uvw_w));
  uvw_v = unitVector(cross(uvw_w, uvw_u));

  corner = center + left * uvw_u + bottom * uvw_v + distance * unitVector(gaze);
  u = uvw_u / distance;
  //v = uvw_v;
  v = unitVector(cross(u, gaze));
  float aspect = (right - left) / (top - bottom);
  v = v / distance / aspect;
  across = uvw_u * (right - left);
  up = uvw_v * (top - bottom);
  radius = aperture;
}

void LoadVector(int& memory_position, int max_memory, FourByte* memory,
                const Vector3& vec) {
  for (int i = 0; i < 3; i++) {
    memory[memory_position].fvalue = static_cast<float>( vec[i] );
    memory_position++;
  }
}

void Camera::LoadIntoMemory(int& memory_position, int max_memory,
                            FourByte* memory) {
  LoadVector(memory_position, max_memory, memory, center);
  LoadVector(memory_position, max_memory, memory, corner);//+3
  LoadVector(memory_position, max_memory, memory, across);//+6
  LoadVector(memory_position, max_memory, memory, up);//+9
  LoadVector(memory_position, max_memory, memory, unitgaze);//+12
  LoadVector(memory_position, max_memory, memory, u);//+15
  LoadVector(memory_position, max_memory, memory, v);//+18
  LoadVector(memory_position, max_memory, memory, unitVector(u));//+21
  LoadVector(memory_position, max_memory, memory, unitVector(v));//+24
  memory[memory_position++].fvalue = radius; //+27
  memory[memory_position++].fvalue = left;//+28
  memory[memory_position++].fvalue = right;//+29
  memory[memory_position++].fvalue = bottom;//+30
  memory[memory_position++].fvalue = top;//+31
  memory[memory_position++].fvalue = near;//+32
  memory[memory_position++].fvalue = far;//+33
}
