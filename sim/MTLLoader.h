#ifndef _HWRT_MTLLOADER_H_
#define _HWRT_MTLLOADER_H_

#include <vector>
#include <string>
#include "TGALoader.h"

namespace simtrax {
  class Material;
}
class PPM;
struct FourByte;

class MTLLoader {
 public:
  static void LoadMTL(const char* filename, std::vector<simtrax::Material*>* matls,
		      std::vector<std::string>& matl_ids, FourByte* mem, int max_mem,
		      int &mem_loc);
  static STGA* ReadTexture(const char* filename, char* tex_buf, std::vector<simtrax::Material*>* matls);
};

#endif //  _HWRT_MTLLOADER_H_
