#ifndef _SIMHWRT_PPM_H_
#define _SIMHWRT_PPM_H_

#include "FourByte.h"

class PPM {
public:
  PPM(std::string& filename);

  void LoadIntoMemory(int& memory_position, int max_memory,
		      FourByte* memory);

  //data members...
  FourByte *data;
  int width, height;
  

};

#endif // _SIMHWRT_PPM_H_
