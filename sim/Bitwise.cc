#include "Bitwise.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

Bitwise::Bitwise(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width)
{
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool Bitwise::SupportsOp(Instruction::Opcode op) const
{
  if (op == Instruction::BITOR ||
      op == Instruction::BITXOR ||
      op == Instruction::BITAND ||
      op == Instruction::ANDN ||
      op == Instruction::sra ||
      op == Instruction::srl ||
      op == Instruction::sext8 ||
      op == Instruction::ORI ||
      op == Instruction::XORI ||
      op == Instruction::ANDI ||
      op == Instruction::ANDNI ||
      op == Instruction::BITSLEFT ||
      op == Instruction::BITSRIGHT ||
      op == Instruction::bslli ||
      op == Instruction::bsll ||
      op == Instruction::bsrli ||
      op == Instruction::bsrl ||
      op == Instruction::bsrai ||
      op == Instruction::bsra ||
      op == Instruction::bsra ||
      op == Instruction::andi ||
      op == Instruction::and_m ||
      op == Instruction::nor ||
      op == Instruction::not_m ||
      op == Instruction::or_m ||
      op == Instruction::ori ||
      op == Instruction::sll ||
      op == Instruction::sra_m ||
      op == Instruction::srl_m ||
      op == Instruction::srav ||
      op == Instruction::srlv ||
      op == Instruction::sllv ||
      op == Instruction::xor_m ||
      op == Instruction::xori
      )
    return true;
  else
    return false;
}

bool Bitwise::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  if (issued_this_cycle >= width) return false;

  reg_value arg1, arg2;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  Instruction::Opcode failop = Instruction::NOP;

  // Read the registers
  switch (ins.op)
  {
    // 2 register operands
    case Instruction::and_m:
    case Instruction::nor:
    case Instruction::or_m:
    case Instruction::xor_m:
    case Instruction::BITOR:
    case Instruction::BITXOR:
    case Instruction::BITAND:
    case Instruction::ANDN:
    case Instruction::BITSLEFT:
    case Instruction::BITSRIGHT:
    case Instruction::bsll:
    case Instruction::bsrl:
    case Instruction::bsra:
    case Instruction::srav:
    case Instruction::srlv:
    case Instruction::sllv:
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) ||
          !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // bad stuff happened
        printf("Bitwise unit: Error in Accepting instruction. Should have passed.\n");
      }
      break;

      // 1 register operand
    case Instruction::ori:
    case Instruction::not_m:
    case Instruction::andi:
    case Instruction::sra_m:
    case Instruction::srl_m:
    case Instruction::sra:
    case Instruction::sll:
    case Instruction::srl:
    case Instruction::sext8:
    case Instruction::bslli:
    case Instruction::bsrli:
    case Instruction::bsrai:
    case Instruction::ORI:
    case Instruction::XORI:
    case Instruction::xori:
    case Instruction::ANDI:
    case Instruction::ANDNI:
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("Bitwise unit: Error in Accepting instruction. Should have passed.\n");
      }
      break;

    default:
      fprintf(stderr, "ERROR Bitwise FOUND SOME OTHER OP\n");
      break;
  };

  // now get the result value
  reg_value result;
  switch (ins.op)
  {
    case Instruction::nor:
      result.udata = ~(arg1.udata | arg1.udata);
      break;

    case Instruction::not_m:
      result.udata = ~arg1.udata;
      break;

    case Instruction::BITOR:
    case Instruction::or_m:
      result.udata = arg1.udata | arg2.udata;
      break;

    case Instruction::sra_m:
      result.udata = arg1.idata >> ins.args[2];
      break;

    case Instruction::srl_m:
      result.udata = arg1.udata >> ins.args[2];
      break;

    case Instruction::BITXOR:
    case Instruction::xor_m:
      result.udata = arg1.udata ^ arg2.udata;
      break;

    case Instruction::BITAND:
    case Instruction::and_m:
      result.udata = arg1.udata & arg2.udata;
      break;

    case Instruction::ANDN:
      result.udata = arg1.udata & ~arg2.udata;
      break;

      // mips ori immediate is 16-bit 0-extended
    case Instruction::ori:
      result.udata = arg1.udata | (0x0000FFFF & ins.args[2]);
      break;

    case Instruction::ORI:
      result.udata = arg1.udata | ins.args[2];
      break;

    case Instruction::XORI:
    case Instruction::xori:
      result.udata = arg1.udata ^ ins.args[2];
      break;

      // MIPS andi immediate is 16-bit 0-extended
    case Instruction::andi:
      result.udata = arg1.udata & (0x0000FFFF & ins.args[2]);
      break;

    case Instruction::ANDI:
      result.udata = arg1.udata & ins.args[2];
      break;

    case Instruction::ANDNI:
      result.udata = arg1.udata & ~ins.args[2];
      break;

    case Instruction::sra:
      thread->carry_register = arg1.udata & 1;
      result.idata = arg1.idata >> 1;  // signed data for arithmetic
      break;

    case Instruction::srl:
      thread->carry_register = arg1.udata & 1;
      result.udata = arg1.udata >> 1;  // unsigned data for logical
      break;

    case Instruction::sext8:
      result.udata = ((arg1.udata << 24) >> 24);
      break;

    case Instruction::BITSLEFT:
      result.udata = arg1.udata << arg2.udata;
      break;

    case Instruction::BITSRIGHT:
      result.udata = arg1.udata >> arg2.udata;
      break;

    case Instruction::sll:
    case Instruction::bslli:
      result.udata = arg1.udata << ins.args[2];
      break;

    case Instruction::bsll:
    case Instruction::sllv:
      result.udata = arg1.udata << arg2.udata;
      break;

      // arithmetic
    case Instruction::bsrai:
      result.idata = arg1.idata >> ins.args[2];
      break;

      // arithmetic
    case Instruction::bsra:
    case Instruction::srav:
      result.idata = arg1.idata >> arg2.idata;
      break;

      // logical
    case Instruction::bsrli:
      result.udata = arg1.udata >> ins.args[2];
      break;

      // logical
    case Instruction::bsrl:
    case Instruction::srlv:
      result.udata = arg1.udata >> arg2.udata;
      break;

    default:
      fprintf(stderr, "ERROR Bitwise FOUND SOME OTHER OP\n");
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
void Bitwise::ClockRise()
{
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
}

void Bitwise::ClockFall()
{
  issued_this_cycle = 0;
}

void Bitwise::print()
{
  printf("%d instructions issued this cycle.", issued_this_cycle);
}
