#include "IntMul.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

IntMul::IntMul(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width)
{
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool IntMul::SupportsOp(Instruction::Opcode op) const
{
  if (op == Instruction::MUL ||
      op == Instruction::MULI ||
      op == Instruction::mul || 
      op == Instruction::mult ||
      op == Instruction::multu ||
      op == Instruction::mulv_w)
    return true;
  else
    return false;
}

bool IntMul::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg0, arg1, arg2;
  // Special results for mult, multu
  long long int product = 0;
  unsigned long long uproduct = 0;
  reg_value resultHI;
  reg_value resultLO;
  Instruction::Opcode failop = Instruction::NOP;
  bool isMSA = ins.op == Instruction::mulv_w;

  // Read the registers
  switch (ins.op)
  {
    case Instruction::MUL:
    case Instruction::mul:
    case Instruction::mulv_w:
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop, isMSA) ||
          !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop, isMSA))
      {
        // bad stuff happened
        printf("IntMul unit: Error in Accepting instruction. Should have passed.\n");
        ins.print();
        printf("\n");
      }

      break;

    case Instruction::MULI:
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("IntMul unit: Error in Accepting instruction. Should have passed.\n");
        ins.print();
        printf("\n");
      }

      break;

  case Instruction::mult:
  case Instruction::multu:
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop) ||
	!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      printf("IntMul unit: Error in Accepting instruction. Should have passed.\n");
    break;
    
  default:
    fprintf(stderr, "ERROR IntMul FOUND SOME OTHER OP\n");
    break;
  };

  // now get the result value
  reg_value result;
  switch (ins.op)
  {
    case Instruction::MUL:
    case Instruction::mul:
      result.idata = arg1.idata * arg2.idata;
      break;

    case Instruction::mulv_w:
      result.idata = arg1.idata * arg2.idata;
      result.idataMSA[0] = arg1.idataMSA[0] * arg2.idataMSA[0];
      result.idataMSA[1] = arg1.idataMSA[1] * arg2.idataMSA[1];
      result.idataMSA[2] = arg1.idataMSA[2] * arg2.idataMSA[2];
      break;

    case Instruction::MULI:
      result.idata = arg1.idata * ins.args[2];
      break;

  case Instruction::mult:
    product = (long long int)arg0.idata * (long long int)arg1.idata;
    resultLO.idata = product & 0xFFFFFFFF;
    resultHI.idata = (product & 0xFFFFFFFF00000000l) >> 32;
    break;

  case Instruction::multu:
    uproduct = (unsigned long long)arg0.udata * (unsigned long long)arg1.udata;
    resultLO.udata = uproduct & 0xFFFFFFFF;
    resultHI.udata = (uproduct & 0xFFFFFFFF00000000l) >> 32;
    break;

    default:
      fprintf(stderr, "ERROR IntMul FOUND SOME OTHER OP\n");
      break;
  };

  // Write the value
  if (ins.op == Instruction::mult || ins.op == Instruction::multu)
  {
    if (!thread->QueueWrite(HI_REG, resultHI, write_cycle, ins.op, &ins) ||
        !thread->QueueWrite(LO_REG, resultLO, write_cycle, ins.op, &ins))
    {
      // pipeline hazzard
      return false;
    }
  }
  else if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins, isMSA))
  {
    // pipeline hazzard
    return false;
  }
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void IntMul::ClockRise()
{
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}

void IntMul::ClockFall()
{
}

void IntMul::print()
{
  printf("%d instructions in-flight",issued_this_cycle);
}

double IntMul::Utilization()
{
  return static_cast<double>(issued_this_cycle) / static_cast<double>(width);
}
