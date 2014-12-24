#include "WinCommonNixFcns.h"

#include "FPAddSub.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>
#include <math.h>
#include <cstdlib>

FPAddSub::FPAddSub(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool FPAddSub::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::FPADD ||
      op == Instruction::FPSUB ||
      op == Instruction::FPRSUB ||
      op == Instruction::RAND ||
      op == Instruction::COS ||
      op == Instruction::SIN ||
      op == Instruction::add_s ||
      op == Instruction::sub_s)
    return true;
  else
    return false;
}

bool FPAddSub::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg1, arg2;
  Instruction::Opcode failop = Instruction::NOP;
  // Read the registers
  if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) || 
      !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
    // bad stuff happened
    printf("Error in FPAddSub. Should have passed.\n");
  }

  // Compute results
  reg_value result;
  switch (ins.op) {
    case Instruction::add_s:
    case Instruction::FPADD:
      result.fdata = arg1.fdata + arg2.fdata;
      break;

    case Instruction::sub_s:
    case Instruction::FPSUB:
      result.fdata = arg1.fdata - arg2.fdata;
      break;
    case Instruction::FPRSUB:
      result.fdata = arg2.fdata - arg1.fdata;
      break;
    case Instruction::RAND:
      result.fdata = drand48();
      break;
    case Instruction::COS:
      result.fdata = cos(arg1.fdata);
      break;
    case Instruction::SIN:
      result.fdata = sin(arg1.fdata);
      break;
    default:
      fprintf(stderr, "ERROR FPADDSUB FOUND SOME OTHER OP\n");
      break;
  };

  // Write the value
  // Write the value
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins)) {
    // pipeline hazzard
    return false;
  }
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void FPAddSub::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}
void FPAddSub::ClockFall() {
}
void FPAddSub::print() {
  printf("%d instructions issued this cycle",issued_this_cycle);
}

double FPAddSub::Utilization() {
  return static_cast<double>(issued_this_cycle)/
      static_cast<double>(width);
}
