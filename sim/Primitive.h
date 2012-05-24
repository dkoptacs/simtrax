#ifndef __SIMHWRT_PRIMITIVE_H__
#define __SIMHWRT_PRIMITIVE_H__

#include "FourByte.h"

class Primitive {
public:
  virtual ~Primitive() {}
  virtual void LoadIntoMemory(int& memory_position, int max_memory,
                              FourByte* memory) = 0;

  enum PrimitiveTypes {
    TRIANGLE_TYPE = 0,
    SPHERE_TYPE,
    LAST_TYPE
  };
};

#endif // __SIMHWRT_PRIMITIVE_H__
