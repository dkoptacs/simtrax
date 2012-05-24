#ifndef __SIMHWRT_OBJLISTLOADER_H_
#define __SIMHWRT_OBJLISTLOADER_H_

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "OBJLoader.h"
class Triangle;
class Material;
class PPM;
struct FourByte;

class OBJListLoader {
public:
  static void LoadModel(const char* filename, std::vector<Triangle*>* tris,
			std::vector<Material*>* matls, FourByte* mem, int max_mem,
			int& mem_loc);
};

#endif // __SIMHWRT_OBJLISTLOADER_H_
