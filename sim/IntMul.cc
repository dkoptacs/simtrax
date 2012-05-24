#include "IntMul.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

IntMul::IntMul(int _latency, int _width) :
  FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool IntMul::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::MUL ||
      op == Instruction::MULI)
    return true;
  else
    return false;
}

bool IntMul::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg1, arg2;
  Instruction::Opcode failop = Instruction::NOP;
  // Read the registers
  switch (ins.op) {
  case Instruction::MUL:
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) || 
	!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
      // bad stuff happened
      printf("IntMul unit: Error in Accepting instruction. Should have passed.\n");
      ins.print();
      printf("\n");
    }
    break;
  case Instruction::MULI:
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("IntMul unit: Error in Accepting instruction. Should have passed.\n");
      ins.print();
      printf("\n");
    }
    break;
  default:
    fprintf(stderr, "ERROR IntMul FOUND SOME OTHER OP\n");
    break;
  };

  // now get the result value
  reg_value result;
  switch (ins.op) {
  case Instruction::MUL:
    result.idata = arg1.idata * arg2.idata;
    break;
  case Instruction::MULI:
    result.idata = arg1.idata * ins.args[2];
    break;
  default:
    fprintf(stderr, "ERROR IntMul FOUND SOME OTHER OP\n");
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
void IntMul::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}

void IntMul::ClockFall() {
}

void IntMul::print() {
  printf("%d instructions in-flight",issued_this_cycle);
}

double IntMul::Utilization() {
  return static_cast<double>(issued_this_cycle)/
    static_cast<double>(width);
}
