
//TODO: REMEMBER TO UPDATE THE LLVM_Trax MAKEFILE WHEN COMMITTING TO SVN

#ifndef _SIMHWRT_MATERIAL_H_
#define _SIMHWRT_MATERIAL_H_

#include "Primitive.h"
#include "Vector3.h"
#include "TGALoader.h"

namespace simtrax {

class Material : public Primitive {
public:
  Material(const Vector3& c0, const Vector3& c1, const Vector3& c2, const int& mtl_type = 0);

  virtual void LoadIntoMemory(int& memory_position, int max_memory, FourByte* memory);
  void LoadTextureIntoMemory(int& memory_position, int max_memory, FourByte* memory);

  int mat_type;
  Vector3 c0;
  Vector3 c1;
  Vector3 c2;
  int i0;
  int i1;
  int i2;
  float sa0, sa1, oa0, oa1;
  float sd0, sd1, od0, od1;
  float ss0, ss1, os0, os1;
  STGA* Ka_tex;
  STGA* Kd_tex;
  STGA* Ks_tex;
};

} // simtrax

#endif // _SIMHWRT_MATERIAL_H_
