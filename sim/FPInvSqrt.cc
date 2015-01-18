#include "FPInvSqrt.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <math.h>
#include <cassert>

FPInvSqrt::FPInvSqrt(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width)
{
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool FPInvSqrt::SupportsOp(Instruction::Opcode op) const
{
  if (op == Instruction::FPINVSQRT ||
      op == Instruction::FPSQRT)
    return true;
  else
    return false;
}

bool FPInvSqrt::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  if (issued_this_cycle >= width) return false;

  reg_value arg1;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  Instruction::Opcode failop = Instruction::NOP;

  // Read the registers
  if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
  {
    // bad stuff happened
    printf("FPInvSqrt unit: Error in Accepting instruction. Should have passed.\n");
  }

  // Compute the value
  reg_value result;
  switch (ins.op)
  {
    case Instruction::FPINVSQRT:
      result.fdata = 1.f / sqrtf(arg1.fdata);
      break;
    case Instruction::FPSQRT:
      result.fdata = sqrtf(arg1.fdata);
      break;
    default:
      fprintf(stderr, "ERROR FPINVSQRT FOUND SOME OTHER OP\n");
      break;
  };

  // Write the value
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins))
  {
    // pipeline hazzard
    return false;
  }

  issued_this_cycle++;
  return true;
}

// From HardwareModule
void FPInvSqrt::ClockRise()
{
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}

void FPInvSqrt::ClockFall()
{
}
void FPInvSqrt::print()
{
  printf("%d instructions issued this cycle.",issued_this_cycle);
}

double FPInvSqrt::Utilization()
{
  return static_cast<double>(issued_this_cycle) / static_cast<double>(width);
}
