#ifndef _SIMHWRT_INTADDSUB_H_
#define _SIMHWRT_INTADDSUB_H_

#include "FunctionalUnit.h"
#include <queue>

class IntAddSub : public FunctionalUnit {
 public:
  IntAddSub(int latency, int width);

  // From FunctionalUnit
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  virtual double Utilization();


  int width;
  int issued_this_cycle;
};

#endif // _SIMHWRT_INTADDSUB_H_
