#ifndef _SIMHWRT_CONVERSION_UNIT_H_
#define _SIMHWRT_CONVERSION_UNIT_H_

// Convert from float->int and int->float
#include "FunctionalUnit.h"
class SimpleRegisterFile;

class ConversionUnit : public FunctionalUnit {
 public:
  ConversionUnit(int latency, int width);

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

#endif // _SIMHWRT_CONVERSION_UNIT_H_
