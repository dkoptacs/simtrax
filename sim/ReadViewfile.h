#ifndef _SIMHWRT_READ_VIEWFILE_H_
#define _SIMHWRT_READ_VIEWFILE_H_

class Camera;

class ReadViewfile {
public:
  static Camera* LoadFile(const char* filename, float far);
};

#endif // SIMHWRT_READ_VIEWFILE
