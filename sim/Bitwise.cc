#include "Bitwise.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

Bitwise::Bitwise(int _latency, int _width) :
  FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool Bitwise::SupportsOp(Instruction::Opcode op) const {
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
      op == Instruction::bsrli ||
      op == Instruction::bsrai
      )
    return true;
  else
    return false;
}

bool Bitwise::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg1, arg2;
  Instruction::Opcode failop = Instruction::NOP;
  // Read the registers
  switch (ins.op) {
  case Instruction::BITOR:
  case Instruction::BITXOR:
  case Instruction::BITAND:
  case Instruction::ANDN:
  case Instruction::BITSLEFT:
  case Instruction::BITSRIGHT:
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) || 
	!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
      // bad stuff happened
      printf("Bitwise unit: Error in Accepting instruction. Should have passed.\n");
    }
    break;
  case Instruction::sra:
  case Instruction::srl:
  case Instruction::sext8:
  case Instruction::bslli:
  case Instruction::bsrli:
  case Instruction::bsrai:
  case Instruction::ORI:
  case Instruction::XORI:
  case Instruction::ANDI:
  case Instruction::ANDNI:
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
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
  switch (ins.op) {
  case Instruction::BITOR:
    result.udata = arg1.udata | arg2.udata;
    break;
  case Instruction::BITXOR:
    result.udata = arg1.udata ^ arg2.udata;
    break;
  case Instruction::BITAND:
    result.udata = arg1.udata & arg2.udata;
    break;
  case Instruction::ANDN:
    result.udata = arg1.udata & ~arg2.udata;
    break;
  case Instruction::ORI:
    result.udata = arg1.udata | ins.args[2];
    break;
  case Instruction::XORI:
    result.udata = arg1.udata ^ ins.args[2];
    break;
  case Instruction::ANDI:
    result.udata = arg1.udata & ins.args[2];
    break;
  case Instruction::ANDNI:
    result.udata = arg1.udata & ~ins.args[2];
    break;
  case Instruction::sra: 
    thread->carry_register = arg1.udata & 1;
    result.idata = arg1.idata >> 1; // signed data for arithmetic
    break;
  case Instruction::srl: 
    thread->carry_register = arg1.udata & 1;
    result.udata = arg1.udata >> 1; // unsigned data for logical
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
  case Instruction::bslli:
    result.udata = arg1.udata << ins.args[2];
    break;
  case Instruction::bsrai: // arithmetic
    result.idata = arg1.idata >> ins.args[2];
    break;
  case Instruction::bsrli: // logical
    result.udata = arg1.udata >> ins.args[2];
    break;
  default:
    fprintf(stderr, "ERROR Bitwise FOUND SOME OTHER OP\n");
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
void Bitwise::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
}

void Bitwise::ClockFall() {
  issued_this_cycle = 0;
}

void Bitwise::print() {
  printf("%d instructions issued this cycle.", issued_this_cycle);
}
