#ifndef __SIMHWRT_OBJLOADER_H_
#define __SIMHWRT_OBJLOADER_H_

#include <vector>
class Triangle;
class Material;
class PPM;
struct FourByte;

class OBJLoader {
public:
  static void LoadModel(const char* filename, std::vector<Triangle*>* tris,
			std::vector<Material*>* matls, FourByte* mem, int max_mem,
			int& mem_loc, int matl_offset = 0);
};

#endif // __SIMHWRT_OBJLOADER_H_
