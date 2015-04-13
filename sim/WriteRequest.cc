#include "WriteRequest.h"
#include <float.h>

WriteRequest::WriteRequest() {
  ready_cycle = 0; // set later
  op = Instruction::NOP;
  instr = NULL;
  which_reg = -1;
  isMSA = false;
  // set good debugging value...
  fdata = FLT_MAX;
}

WriteRequest::WriteRequest(long long int which_cycle) :
  ready_cycle(which_cycle) {
  // set good debugging value...
  fdata = FLT_MAX;
  instr = NULL;
  isMSA = false;
}

bool WriteRequest::IsReady(long long int current_cycle) const {
  //printf("**%lld: ",current_cycle);
  //printf("%s:%d = %d at %lld\n",Instruction::Opnames[op].c_str(), which_reg, idata, ready_cycle);
  return current_cycle >= ready_cycle;
}

void WriteRequest::SetInt(int new_int) {
  idata = new_int;
}
void WriteRequest::SetUint(unsigned int new_uint) {
  udata = new_uint;
}
void WriteRequest::SetFloat(float new_float) {
  fdata = new_float;
}

void WriteRequest::print() {
  printf("%s:%d = %d at %lld\n",Instruction::Opnames[op].c_str(), which_reg, idata, ready_cycle);
}
