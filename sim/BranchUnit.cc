#include "BranchUnit.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>

BranchUnit::BranchUnit(int _latency, int _width) :
  FunctionalUnit(_latency), width(_width) {
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool BranchUnit::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::BLT || op == Instruction::JMP ||
      op == Instruction::JMPREG || op == Instruction::JAL ||
      op == Instruction::BET || op == Instruction::BNZ ||
      op == Instruction::rtsd ||
      op == Instruction::beqd ||
      op == Instruction::beqid ||
      op == Instruction::bged ||
      op == Instruction::bgeid ||
      op == Instruction::bgtd ||
      op == Instruction::bgtid ||
      op == Instruction::bled ||
      op == Instruction::bleid ||
      op == Instruction::bltd ||
      op == Instruction::bltid ||
      op == Instruction::bned ||
      op == Instruction::bneid ||
      op == Instruction::brd ||
      op == Instruction::brad ||
      op == Instruction::brld ||
      op == Instruction::brald ||
      op == Instruction::brid ||
      op == Instruction::braid ||
      op == Instruction::brlid ||
      op == Instruction::bralid ||
      op == Instruction::brk ||
      op == Instruction::brki
      )
    return true;
  else
    return false;
}

bool BranchUnit::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg, arg0, arg1;
  Instruction::Opcode failop = Instruction::NOP;

  // Read registers
  switch(ins.op) {
  case Instruction::JMP:
  case Instruction::brid:
  case Instruction::braid:
    break;
  case Instruction::JMPREG:
  case Instruction::brd:
  case Instruction::brad:
  case Instruction::rtsd:
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg, failop)) {
      // bad, shouldn't happen
      printf("Error in BranchUnit.\n");
    }
    break;
  case Instruction::JAL:
  case Instruction::brlid:
  case Instruction::bralid:
  case Instruction::brki:
    arg.idata = thread->program_counter;
    // write the value
    if (!thread->QueueWrite(write_reg, arg, write_cycle, ins.op, &ins)) {
      // pipeline hazzard
      return false;
    }
    break;
  case Instruction::brld:
  case Instruction::brald:
  case Instruction::brk:
    // Second arg is register with address to branch to
    arg.idata = thread->program_counter;
    // write the value
    if (!thread->QueueWrite(write_reg, arg, write_cycle, ins.op, &ins)) {
      // pipeline hazzard
      return false;
    }
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg, failop)) {
      // bad stuff happened
      printf("Error in BranchUnit. Should have passed.\n");
    }
    break;
  case Instruction::beqd:
  case Instruction::bged:
  case Instruction::bgtd:
  case Instruction::bled:
  case Instruction::bltd:
  case Instruction::bned:
    // read two registers. Branch to value in second after testing value in first
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop) || 
	!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("Error in BranchUnit. Should have passed.\n");
    }
    arg.idata = arg1.idata;
    // doing another check, okay to break
    break;
  case Instruction::beqid:
  case Instruction::bgeid:
  case Instruction::bgtid:
  case Instruction::bleid:
  case Instruction::bltid:
  case Instruction::bneid:
    // read 1 register. Test its value and branch to immediate value if true
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop)) {
      // bad stuff happened
      printf("Error in BranchUnit. Should have passed.\n");
    }
    arg.idata = ins.args[1];
    // doing another check, okay to break
    break;
  default:
    // Read the registers
    // this is for BLT, BET and BNZ
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop) || 
	!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("Error in BranchUnit. Should have passed.\n");
    }
    // doing another check, okay to break
    break;
  };

  // perform jump
  switch (ins.op) {
  case Instruction::JMP:
  case Instruction::JAL:
    // no delay
    thread->program_counter = ins.args[2];
    thread->next_program_counter = thread->program_counter + 1;
    break;
  case Instruction::braid:
  case Instruction::bralid:
  case Instruction::brki:
  case Instruction::brlid:
    thread->next_program_counter = ins.args[1];
    break;
  case Instruction::rtsd:
    thread->next_program_counter = arg.idata + ins.args[1] / 4 - 1; // make up for byte addressing
    break;
  case Instruction::brid:
    // all non-a's are offsets FIXME when we have offset mode in the assembler
    thread->next_program_counter = ins.args[0];// + thread->program_counter;
    break;
  case Instruction::JMPREG:
  case Instruction::brad:
  case Instruction::brald:
  case Instruction::brk:
    thread->next_program_counter = arg.idata;
    break;
  case Instruction::brd:
  case Instruction::brld:
    thread->next_program_counter = arg.idata + thread->program_counter;
    break;
  case Instruction::BLT:
    if (arg0.idata < arg1.idata)
      thread->next_program_counter = ins.args[2];
    break;
  case Instruction::BET:
    if (arg0.idata == arg1.idata)
      thread->next_program_counter = ins.args[2];
    break;
  case Instruction::BNZ:
    if (arg0.idata != 0) {
      thread->program_counter = ins.args[2];
      thread->next_program_counter = thread->program_counter + 1;
    }
    break;
  case Instruction::beqd:
  case Instruction::beqid:
    if (arg0.idata == 0)
      thread->next_program_counter = arg.idata;
    break;
    // These comparisons might be backwards
  case Instruction::bged:
  case Instruction::bgeid:
     if (arg0.idata >= 0)
//     if (arg0.idata < 0)
      thread->next_program_counter = arg.idata;
    break;
  case Instruction::bgtd:
  case Instruction::bgtid:
     if (arg0.idata > 0)
//     if (arg0.idata <= 0)
      thread->next_program_counter = arg.idata;
    break;
  case Instruction::bled:
  case Instruction::bleid:
     if (arg0.idata <= 0)
//     if (arg0.idata > 0)
      thread->next_program_counter = arg.idata;
    break;
  case Instruction::bltd:
  case Instruction::bltid:
     if (arg0.idata < 0)
//     if (arg0.idata >= 0)
      thread->next_program_counter = arg.idata;
    break;
  case Instruction::bned:
  case Instruction::bneid:
    if (arg0.idata != 0)
      {
	
	thread->next_program_counter = arg.idata;
	
      }
    break;
  default:
    fprintf(stderr, "ERROR BranchUnit FOUND SOME OTHER OP\n");
    break;
  };
  
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void BranchUnit::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)

  // TODO: We need to execute instructions in branch delay spots.
}

void BranchUnit::ClockFall() {
  issued_this_cycle = 0;
}

void BranchUnit::print() {
  printf("%d instructions issued this cycle.", issued_this_cycle);
}
