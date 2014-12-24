#include "FPDiv.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <math.h>
#include <cassert>

FPDiv::FPDiv(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool FPDiv::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::FPDIV ||
      op == Instruction::DIV ||
      op == Instruction::FPINV ||
      op == Instruction::div ||
      op == Instruction::div_s)
    return true;
  else
    return false;
}

bool FPDiv::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg1, arg2;
  Instruction::Opcode failop = Instruction::NOP;
  
  // Read the registers

  if (ins.op == Instruction::div)
  {
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) ||
        !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
    {
      printf("FPDiv unit: Error in Accepting MIPS instruction.\n");
    }
  }
  else if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
  {
    // bad stuff happened
    printf("FPDiv unit: Error in Accepting instruction. Should have passed.\n");
  }

  // DIV needs a second register
  if (ins.op == Instruction::FPDIV ||
      ins.op == Instruction::DIV ||
      ins.op == Instruction::div_s) {
    if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
      // bad stuff happened
      printf("FPDiv unit: Error in Accepting instruction. Should have passed.\n");
    }
  }

  // Compute the value
  reg_value result;
  switch (ins.op) {

    case Instruction::div:
      if (arg2.idata == 0)
      {
        printf("dividing by zero!\n");
        thread->LO_register = 0x7FFFFFFF;
        thread->HI_register = 0x7FFFFFFF;
      }
      else
      {
        thread->LO_register = arg1.idata / arg2.idata;
        thread->HI_register = arg1.idata % arg2.idata;
      }
      
      break;
    case Instruction::FPDIV:
    case Instruction::div_s:
      result.fdata = arg1.fdata/arg2.fdata;
      break;
    case Instruction::DIV:
      if (arg1.idata == 0) {
        printf("dividing by zero!\n");
        result.idata = 0x7FFFFFFF;
      } else {
        // This is actually a reverse divide
        result.idata = arg2.idata/arg1.idata;
      }
      break;
    case Instruction::FPINV:
      result.fdata = 1.f/arg1.fdata;
      break;
    default:
      fprintf(stderr, "ERROR FPINVSQRT FOUND SOME OTHER OP\n");
      break;
  };

  // Write the value
  if (ins.op == Instruction::div);
  else if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins)) {
    // pipeline hazzard
    return false;
  }
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void FPDiv::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}
void FPDiv::ClockFall() {
}
void FPDiv::print() {
  printf("%d instructions issued this cycle.",issued_this_cycle);
}

double FPDiv::Utilization() {
  return static_cast<double>(issued_this_cycle)/
      static_cast<double>(width);
}
