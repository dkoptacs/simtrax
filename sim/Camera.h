#ifndef _SIMHWRT_CAMERA_H_
#define _SIMHWRT_CAMERA_H_

#include "Vector3.h"
#include "FourByte.h"

namespace simtrax {

class Camera {
public:
  Camera(const Vector3& c,
         const Vector3& gaze,
         const Vector3& vup,
         float aperture,
         float left, float right,
         float bottom, float top, float distance, float far);
  // A ThinLens camera is a center (3 floats), corner (3 floats),
  // across (3 floats), up (3 floats), u (3 floats), v (3 floats) and
  // radius (1 float): total of 19 floats (though across and up are scaled versions of u,v which are normalized)
  void LoadIntoMemory(int& memory_position, int max_memory,
                      FourByte* memory);

  Vector3 center;
  Vector3 corner;
  Vector3 across;
  Vector3 up;
  Vector3 u;
  Vector3 v;
  Vector3 unitgaze;
  float radius;

  float left, right, bottom, top, near, far;

};

} // simtrax

#endif // _SIMHWRT_CAMERA_H_
