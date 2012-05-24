#ifndef _SIMHWRT_READ_LIGHTFILE_H_
#define _SIMHWRT_READ_LIGHTFILE_H_

class ReadLightfile {
public:
  static void LoadFile(const char* filename, float* light_pos);
};

#endif // SIMHWRT_READ_LIGHTFILE
