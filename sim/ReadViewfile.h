#ifndef _SIMHWRT_READ_VIEWFILE_H_
#define _SIMHWRT_READ_VIEWFILE_H_

namespace simtrax {
  class Camera;
}

class ReadViewfile {
public:
  static simtrax::Camera* LoadFile(const char* filename, float far);
};

#endif // SIMHWRT_READ_VIEWFILE
