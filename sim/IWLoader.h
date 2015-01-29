#ifndef __SIMHWRT_IWLOADER_H_
#define __SIMHWRT_IWLOADER_H_

#include <vector>
namespace simtrax {
  class Triangle;
  class Material;
}

class IWLoader {
public:
  static void LoadModel(const char* filename, std::vector<simtrax::Triangle*>* tris, std::vector<simtrax::Material*>* matls);
};

#endif // __SIMHWRT_OBJLOADER_H_
