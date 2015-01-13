#ifndef _SIMHWRT_LOAD_STORE_H_
#define _SIMHWRT_LOAD_STORE_H_

#include "FunctionalUnit.h"

class LoadStore : public FunctionalUnit {
 public:
  LoadStore();
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);
  virtual void ClockRise();
  virtual void ClockFall();
};

#endif // _SIMHWRT_LOAD_STORE_H_
