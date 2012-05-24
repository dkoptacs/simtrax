#ifndef _SIMHWRT_DEBUG_UNIT_H_
#define _SIMHWRT_DEBUG_UNIT_H_

#include "FunctionalUnit.h"
#include "Assembler.h"

class DebugUnit : public FunctionalUnit {
public:
  DebugUnit(int latency);

  // From FunctionalUnit
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  void setRegisters(const std::vector<symbol*> *regs)
  {
    registers = regs;
  }
  const std::vector<symbol*> *registers;
};

#endif // _SIMHWRT_FPMUL_H_
