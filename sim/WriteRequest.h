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
  Instruction* instr;
  int which_reg;
  bool isMSA;
  
  union {
    int idata;
    unsigned int udata;
    float fdata;
  };
  union {
    int idataMSA[3];
    unsigned int udataMSA[3];
    float fdataMSA[3];
  };
};

#endif // __SIMHWRT_WRITE_REQUEST_H__
