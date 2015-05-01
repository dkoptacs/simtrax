#ifndef _SIMHWRT_INTMUL_H_
#define _SIMHWRT_INTMUL_H_

#include "FunctionalUnit.h"
#include "IntAddSub.h"

class SimpleRegisterFile;

class IntMul : public FunctionalUnit {
 public:
  IntMul(int latency, int width);

  // From FunctionalUnit
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  virtual double Utilization();

  void SetAdder(IntAddSub* adder);
  IntAddSub* add_unit;

  int width;
  int issued_this_cycle;
};

#endif // _SIMHWRT_INTMUL_H_
