#include "Camera.h"
#include <iostream>

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

  std::cout << "Center is " << center << std::endl;
  std::cout << "Corner is " << corner << std::endl;
  std::cout << "Across is " << across << std::endl;
  std::cout << "Up is     " << up << std::endl;
  std::cout << "gaze is   " << unitgaze << std::endl;
  std::cout << "U is      " << u << std::endl;
  std::cout << "V is      " << v << std::endl;
  std::cout << "Unorm is  " << unitVector(u) << std::endl;
  std::cout << "Vnorm is  " << unitVector(v) << std::endl;
  std::cout << "radius is " << radius << std::endl;
  std::cout << "left is   " << left << std::endl;
  std::cout << "right is  " << right << std::endl;
  std::cout << "bottom is " << bottom << std::endl;
  std::cout << "top is    " << top << std::endl;
  std::cout << "near is   " << distance << std::endl;
  std::cout << "far is    " << far << std::endl;
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

/*
  // From uber.gr
#if USE_SPHERES || USE_BOXES
  default_camera.center[0] = -5.f;
  default_camera.center[1] =  1.f;
  default_camera.center[2] =  3.f;

  default_camera.corner[0] = -2.341772;
  default_camera.corner[1] =  2.22045e-16;
  default_camera.corner[2] = -0.344222;

  default_camera.across[0] = 1.54349;
  default_camera.across[1] = -0;
  default_camera.across[2] = 2.57248;

  default_camera.up[0] = 0;
  default_camera.up[1] = 2.f;
  default_camera.up[2] = 0.f;

  default_camera.u[0] = 0.514496;
  default_camera.u[1] = -0.f;
  default_camera.u[2] = 0.857493;

  default_camera.v[0] = 0.f;
  default_camera.v[1] = 1.f;
  default_camera.v[2] = 0.f;
#else
  default_camera.center[0] = 278.f;
  default_camera.center[1] = 273.f;
  default_camera.center[2] = -550.f;

  default_camera.corner[0] = 279.f;
  default_camera.corner[1] = 272.f;
  default_camera.corner[2] = -548.f;

  default_camera.across[0] = -2.f;
  default_camera.across[1] =  0.f;
  default_camera.across[2] =  0.f;

  default_camera.up[0] = 0;
  default_camera.up[1] = 2.f;
  default_camera.up[2] = 0.f;

  default_camera.u[0] = -1.f;
  default_camera.u[1] = 0.f;
  default_camera.u[2] = 0.f;

  default_camera.v[0] = 0.f;
  default_camera.v[1] = 1.f;
  default_camera.v[2] = 0.f;
#endif

  default_camera.radius = 0.f;

  for (int i = 0; i < camera_size; i++) {
    mem[start_camera + i].fvalue = camera_params[i];
  }
*/
