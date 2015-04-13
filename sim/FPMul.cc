#include "FPMul.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

FPMul::FPMul(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width)
{
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool FPMul::SupportsOp(Instruction::Opcode op) const
{
  if (op == Instruction::FPMUL || op == Instruction::mul_s ||
      op == Instruction::fmul_w)
    return true;
  else
    return false;
}

bool FPMul::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  if (issued_this_cycle >= width) return false;

  reg_value arg1, arg2;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  Instruction::Opcode failop = Instruction::NOP;

  bool isMSA = ins.op == Instruction::fmul_w;

  // Read the registers
  if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop, isMSA) ||
      !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop, isMSA))
  {
    // bad stuff happened
    printf("FPMul unit: Error in Accepting instruction. Should have passed.\n");
  }

  // now get the result value
  reg_value result;
  switch (ins.op)
  {
    case Instruction::mul_s:
    case Instruction::FPMUL:
      result.fdata = arg1.fdata * arg2.fdata;
      break;

  case Instruction::fmul_w:
    result.fdata = arg1.fdata * arg2.fdata;
    result.fdataMSA[0] = arg1.fdataMSA[0] * arg2.fdataMSA[0];
    result.fdataMSA[1] = arg1.fdataMSA[1] * arg2.fdataMSA[1];
    result.fdataMSA[2] = arg1.fdataMSA[2] * arg2.fdataMSA[2];
    break;


    default:
      fprintf(stderr, "ERROR FPMul FOUND SOME OTHER OP\n");
      break;
  };

  // Write the value
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins, isMSA)) {
    // pipeline hazard
    return false;
  }
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void FPMul::ClockRise()
{
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}

void FPMul::ClockFall()
{
}

void FPMul::print()
{
  printf("%d instructions in-flight",issued_this_cycle);
}

double FPMul::Utilization()
{
  return static_cast<double>(issued_this_cycle) / static_cast<double>(width);
}
