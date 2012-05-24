#include "FPMinMax.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

FPMinMax::FPMinMax(int _latency, int _width) :
  FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool FPMinMax::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::FPMIN ||
      op == Instruction::FPMAX)
    return true;
  else
    return false;
}

bool FPMinMax::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg1, arg2;
  Instruction::Opcode failop = Instruction::NOP;
  // Read the registers
  if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) || 
      !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
    // bad stuff happened
    printf("FPMinMax unit: Error in Accepting instruction. Should have passed.\n");
  }

  // Compute
  reg_value result;
  switch (ins.op) {
  case Instruction::FPMIN:
    if (arg1.fdata < arg2.fdata)
      result.fdata = arg1.fdata;
    else
      result.fdata = arg2.fdata;
    break;
  case Instruction::FPMAX:
    if (arg1.fdata > arg2.fdata)
      result.fdata = arg1.fdata;
    else
      result.fdata = arg2.fdata;
    break;
  default:
    fprintf(stderr, "ERROR FPMINMAX FOUND SOME OTHER OP\n");
    break;
  };

  // Write the value
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op)) {
    // pipeline hazzard
    return false;
  }
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void FPMinMax::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}
void FPMinMax::ClockFall() {
}
void FPMinMax::print() {
  printf("%d instructions issued.", issued_this_cycle);
}

double FPMinMax::Utilization() {
  return static_cast<double>(issued_this_cycle)/
    static_cast<double>(width);
}

bool FPMinMax::ReportUtilization(int& processed, int& max_process) {
  processed = issued_this_cycle;
  max_process = width;
  return true;
}
