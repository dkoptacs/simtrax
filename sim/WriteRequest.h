#ifndef __SIMHWRT_WRITE_REQUEST_H_
#define __SIMHWRT_WRITE_REQUEST_H_

#include "Instruction.h"

class WriteRequest {
public:
  long long int ready_cycle;
  WriteRequest();
  WriteRequest(long long int ready_cycle);
  bool IsReady(long long int current_cycle) const;
  void SetInt(int new_int);
  void SetUint(unsigned int new_uint);
  void SetFloat(float new_float);
  void print();

  Instruction::Opcode op;
  int which_reg;
  
  union {
    int idata;
    unsigned int udata;
    float fdata;
  };
};

#endif // __SIMHWRT_WRITE_REQUEST_H__
