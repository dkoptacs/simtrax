#ifndef _SIMHWRT_SYNCHRONIZE_H_
#define _SIMHWRT_SYNCHRONIZE_H_

#include "FunctionalUnit.h"
#include <queue>

class FunctionalUnit;

class Synchronize : public FunctionalUnit {
public:
  Synchronize(int latency, int width, int simd_width, int num_threads);

  // From FunctionalUnit
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();

  int width;
  int issued_this_cycle;

  int simd_width;
  int num_threads;
  int *locked;
  int *at_lock;
};

#endif // _SIMHWRT_SYNCHRONIZE_H_
