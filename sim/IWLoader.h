#ifndef __SIMHWRT_IWLOADER_H_
#define __SIMHWRT_IWLOADER_H_

#include <vector>
class Triangle;
class Material;

class IWLoader {
public:
  static void LoadModel(const char* filename, std::vector<Triangle*>* tris,
			std::vector<Material*>* matls);
};

#endif // __SIMHWRT_OBJLOADER_H_
