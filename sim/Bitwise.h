#ifndef _SIMHWRT_BITWISE_H_
#define _SIMHWRT_BITWISE_H_

#include "FunctionalUnit.h"
#include <queue>

class FunctionalUnit;

class Bitwise : public FunctionalUnit {
 public:
  Bitwise(int latency, int width);

  // From FunctionalUnit
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();

  int width;
  int issued_this_cycle;
};

#endif // _SIMHWRT_BITWISE_H_
