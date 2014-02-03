#include "ConversionUnit.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

ConversionUnit::ConversionUnit(int _latency, int _width) :
  FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool ConversionUnit::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::FPCONV)
    return true;
  else if (op == Instruction::INTCONV)
    return true;
  else return false;
}

bool ConversionUnit::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg1;
  Instruction::Opcode failop = Instruction::NOP;
  // Read the register
  if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
    // should never happen
    printf("Error in conversionUnit.\n");
  }

  // Get result value
  reg_value result;
  switch (ins.op) {
  case Instruction::FPCONV:
    result.fdata = static_cast<float>(arg1.idata);
    break;
  case Instruction::INTCONV:
    result.idata = static_cast<int>(arg1.fdata);
    break;
  default:
    fprintf(stderr, "ERROR ConversionUnit FOUND SOME OTHER OP\n");
    break;
  };

  // Write the value
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins)) {
    // pipeline hazzard
    return false;
  }
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void ConversionUnit::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
}
void ConversionUnit::ClockFall() {
  issued_this_cycle = 0;
}

void ConversionUnit::print() {
  printf("%d instructions issued this cycle.",issued_this_cycle);
}

double ConversionUnit::Utilization() {
  return static_cast<double>(issued_this_cycle)/
    static_cast<double>(width);
}
