#ifndef __SIMHWRT_FOUR_BYTE_H__
#define __SIMHWRT_FOUR_BYTE_H__

#include <iostream>

struct FourByte {
  union {
    int ivalue;
    unsigned int uvalue;
    float fvalue;
  };
};

#endif // __SIMHWRT_FOUR_BYTE_H__
