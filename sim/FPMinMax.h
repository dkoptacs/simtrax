#ifndef _SIMHWRT_FPMINMAX_H_
#define _SIMHWRT_FPMINMAX_H_

#include "FunctionalUnit.h"

class SimpleRegisterFile;

class FPMinMax : public FunctionalUnit {
 public:
  FPMinMax(int latency, int width);

  // From FunctionalUnit
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  virtual double Utilization();

  virtual bool ReportUtilization(int& processed, int& max_process);

  int width;
  int issued_this_cycle;
};

#endif // _SIMHWRT_FPADDSUB_H_
