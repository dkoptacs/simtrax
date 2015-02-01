#include "FPCompare.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>
#include <math.h>

FPCompare::FPCompare(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool FPCompare::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::FPCMPLT ||
      op == Instruction::FPEQ ||
      op == Instruction::FPNE ||
      op == Instruction::FPLT ||
      op == Instruction::FPLE ||
      op == Instruction::FPGT ||
      op == Instruction::FPGE ||
      op == Instruction::FPUN ||
      op == Instruction::FPNEG ||
      op == Instruction::neg_s ||
      op == Instruction::EQ ||
      op == Instruction::NE ||
      op == Instruction::LT ||
      op == Instruction::LE ||
      op == Instruction::c_eq_s ||
      op == Instruction::c_ole_s ||
      op == Instruction::c_olt_s ||
      op == Instruction::c_ule_s ||
      op == Instruction::c_ult_s
      )
    return true;
  else
    return false;
}

bool FPCompare::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;

  reg_value arg0, arg1, arg2;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  Instruction::Opcode failop = Instruction::NOP;
  bool mips_compare = false;

  // Read the registers
  if (ins.op == Instruction::FPNEG || ins.op == Instruction::neg_s)
  {
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
    {
      // bad stuff happened
      printf("FPCompare unit: Error in Accepting instruction. Should have passed.\n");
    }
  }

  else if (ins.op == Instruction::c_eq_s ||
           ins.op == Instruction::c_ole_s ||
           ins.op == Instruction::c_olt_s ||
           ins.op == Instruction::c_ule_s ||
           ins.op == Instruction::c_ult_s)
  {
    mips_compare = true;
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop) ||
        !thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("FPCompare unit: Error in Accepting instruction.\n");
    }
  }

  else
  {
    // all other instructions read 2 registers
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) ||
        !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
      // bad stuff happened
      printf("FPCompare unit: Error in Accepting instruction. Should have passed.\n");
    }
  }

  // Compute result
  reg_value result;
  switch (ins.op)
  {

    case Instruction::c_eq_s:
      if (isnan(arg0.fdata) || isnan(arg1.fdata))
        thread->compare_register = 0;
      else if (arg0.fdata == arg1.fdata)
        thread->compare_register = 1;
      else
        thread->compare_register = 0;
      break;

    case Instruction::c_olt_s:
      if (isnan(arg0.fdata) || isnan(arg1.fdata))
      {
        thread->compare_register = 0;
      }
      else if (arg0.fdata < arg1.fdata)
        thread->compare_register = 1;
      else
        thread->compare_register = 0;
      break;

    case Instruction::c_ole_s:
      if (isnan(arg0.fdata) || isnan(arg1.fdata))
      {
        thread->compare_register = 0;
      }
      else if (arg0.fdata <= arg1.fdata)
        thread->compare_register = 1;
      else
        thread->compare_register = 0;
      break;

    case Instruction::c_ult_s:
      if (isnan(arg0.fdata) || isnan(arg1.fdata))
      {
        thread->compare_register = 1;
      }
      else if (arg0.fdata < arg1.fdata)
        thread->compare_register = 1;
      else
        thread->compare_register = 0;
      break;

    case Instruction::c_ule_s:
      if (isnan(arg0.fdata) || isnan(arg1.fdata))
      {
        thread->compare_register = 1;
      }
      else if (arg0.fdata <= arg1.fdata)
        thread->compare_register = 1;
      else
        thread->compare_register = 0;
      break;

    case Instruction::FPCMPLT:
      // old style compare
      result.fdata = (arg1.fdata < arg2.fdata) ? 0xffffffff : 0;
      break;

    case Instruction::FPEQ:
      result.idata = (arg1.fdata == arg2.fdata) ? 1 : 0;
      break;

    case Instruction::FPNE:
      result.idata = (arg1.fdata != arg2.fdata) ? 1 : 0;
      break;

    case Instruction::FPLT:
      result.idata = (arg1.fdata < arg2.fdata) ? 1 : 0;
      break;

    case Instruction::FPLE:
      result.idata = (arg1.fdata <= arg2.fdata) ? 1 : 0;
      break;

    case Instruction::FPGT:
      result.idata = (arg1.fdata > arg2.fdata) ? 1 : 0;
      break;

    case Instruction::FPGE:
      result.idata = (arg1.fdata >= arg2.fdata) ? 1 : 0;
      break;

    // FPUN (FP "un"defined), checks for NaN, relies on IEEE defining NaN != NaN
    case Instruction::FPUN:
      result.idata = (arg1.fdata != arg1.fdata || arg2.fdata != arg2.fdata) ? 1 : 0;
      break;

    case Instruction::FPNEG:
    case Instruction::neg_s:
      result.fdata = -arg1.fdata;
      break;

    // These comparisons might not be used any more
    case Instruction::EQ:
      result.idata = (arg1.idata == arg2.idata) ? 0xffffffff : 0;
      break;

    case Instruction::NE:
      result.idata = (arg1.idata != arg2.idata) ? 0xffffffff : 0;
      break;

    case Instruction::LT:
      result.idata = (arg1.idata < arg2.idata) ? 0xffffffff : 0;
      break;

    case Instruction::LE:
      result.idata = (arg1.idata <= arg2.idata) ? 0xffffffff : 0;
      break;

    default:
      fprintf(stderr, "ERROR FPCompare OP NOT RECOGNIZED\n");
      break;
  };

  // Write the value
  if (!mips_compare && !thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins))
  {
    // pipeline hazzard
    return false;
  }

  issued_this_cycle++;
  return true;
}

// From HardwareModule
void FPCompare::ClockRise()
{
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}

void FPCompare::ClockFall()
{
}

void FPCompare::print()
{
  printf("%d instructions issued this cycle.",issued_this_cycle);
}

double FPCompare::Utilization()
{
  return static_cast<double>(issued_this_cycle) / static_cast<double>(width);
}
