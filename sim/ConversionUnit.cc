#include "ConversionUnit.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

ConversionUnit::ConversionUnit(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width)
{
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool ConversionUnit::SupportsOp(Instruction::Opcode op) const
{
  if (op == Instruction::FPCONV ||
      op == Instruction::FTOD ||
      op == Instruction::INTCONV ||
      op == Instruction::cvt_s_w ||
      op == Instruction::trunc_w_s)
    return true;
  else
    return false;
}

bool ConversionUnit::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  if (issued_this_cycle >= width) return false;

  reg_value arg1, arg2;
  int write_reg = ins.args[0];
  int write_reg2 = ins.args[1];
  long long int write_cycle = issuer->current_cycle + latency;
  Instruction::Opcode failop = Instruction::NOP;

  // Read the register
  if(ins.op == Instruction::FTOD)
  {
    // Check for unexpected error
    if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      printf("Error in conversionUnit.\n");
  }
  else
  {
    // Check for unexpected error
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      printf("Error in conversionUnit.\n");
  }

  // Get result value
  reg_value result, result2;
  double d;
  switch (ins.op)
  {
    case Instruction::cvt_s_w:
    case Instruction::FPCONV:
      result.fdata = static_cast<float>(arg1.idata);
      break;

    case Instruction::trunc_w_s:
    case Instruction::INTCONV:
      result.idata = static_cast<int>(arg1.fdata);
      break;
      
    case Instruction::FTOD: // float to double (destination is 2 registers)
      d = static_cast<double>(arg2.fdata);
      // no good way to cast from double to long long without changing bits
      memcpy(&result.udata, (char*)&d, 4);
      memcpy(&result2.udata, ((char*)&d) + 4, 4);
      break;
      
    default:
      fprintf(stderr, "ERROR ConversionUnit FOUND SOME OTHER OP\n");
      break;
  };

  // Write the value(s)
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins))
  {
    // pipeline hazzard
    return false;
  }
  if(ins.op == Instruction::FTOD)
  {
    if (!thread->QueueWrite(write_reg2, result2, write_cycle, ins.op, &ins))
    {
      // pipeline hazzard
      return false;
    }
  }

  issued_this_cycle++;
  return true;
}

// From HardwareModule
void ConversionUnit::ClockRise()
{
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
}
void ConversionUnit::ClockFall()
{
  issued_this_cycle = 0;
}

void ConversionUnit::print()
{
  printf("%d instructions issued this cycle.",issued_this_cycle);
}

double ConversionUnit::Utilization()
{
  return static_cast<double>(issued_this_cycle) / static_cast<double>(width);
}
