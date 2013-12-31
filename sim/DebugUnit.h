#ifndef _SIMHWRT_DEBUG_UNIT_H_
#define _SIMHWRT_DEBUG_UNIT_H_

#include "FunctionalUnit.h"
#include "Assembler.h"
#include "LocalStore.h"

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
  void PrintFormatString(int format_addr, ThreadState* thread);
  void setRegisters(const std::vector<symbol*> *regs)
  {
    registers = regs;
  }
  void setLocalStore(const LocalStore* ls)
  {
    ls_unit = ls;
  }

  const std::vector<symbol*> *registers;
  
  //TODO: This won't work with hyperthreading.
  //      Will need a separate ls_unit pointer for each thread.
  const LocalStore* ls_unit;
};

#endif // _SIMHWRT_FPMUL_H_
