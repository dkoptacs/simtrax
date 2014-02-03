#include "IntAddSub.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>
#include <stdlib.h>

IntAddSub::IntAddSub(int _latency, int _width) :
  FunctionalUnit(_latency), width(_width){
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool IntAddSub::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::ADD ||
      op == Instruction::ADDI ||
      op == Instruction::ADDK ||
      op == Instruction::ADDKC ||
      op == Instruction::ADDIK ||
      op == Instruction::ADDIKC ||
      op == Instruction::SUB ||
      op == Instruction::CMP ||
      op == Instruction::CMPU || 
      op == Instruction::RSUB ||
      op == Instruction::RSUBI)
    return true;
  else
    return false;
}

bool IntAddSub::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg1, arg2;
  Instruction::Opcode failop = Instruction::NOP;
  // Read the registers
  if (ins.op == Instruction::ADDI ||
      ins.op == Instruction::RSUBI) {
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("IntAddSub unit: Error in Accepting instruction. Should have passed1.\n");
    }
  } else {
    // two register instructions
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) || 
	!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
      // bad stuff happened
      printf("IntAddSub unit: Error in Accepting instruction. Should have passed2.\n");
      printf("PC = %d, thread = %d, core = %d, cycle = %lld, reg_ready[%d] = %lld, reg_ready[%d] = %lld\n", 
	     ins.pc_address, thread->thread_id, (int)thread->core_id, issuer->current_cycle, 
	     ins.args[1], thread->register_ready[ins.args[1]],
	     ins.args[2], thread->register_ready[ins.args[2]]);
      exit(1);
    }
  }
  
  // now get the result value
  unsigned long long overflow;
  reg_value result;
  result.udata = 0;
  switch (ins.op) {
  case Instruction::ADD:
    result.idata = arg1.idata + arg2.idata;
    break;
  case Instruction::ADDK:
    thread->carry_register = 0;
  case Instruction::ADDKC:
    result.idata = arg1.idata + arg2.idata + thread->carry_register;
    overflow = (long long int)arg1.udata + (long long int)arg2.udata + (long long int)thread->carry_register;
    if(overflow & 0x100000000LL) // check 32nd bit for carry
      thread->carry_register = 1;
    else thread->carry_register = 0;
    break;
  case Instruction::ADDI:
    result.idata = arg1.idata + ins.args[2];
    break;
  case Instruction::ADDIK:
    thread->carry_register = 0;
  case Instruction::ADDIKC:
    result.idata = arg1.idata + ins.args[2] + thread->carry_register;
    overflow = (long long int)arg1.udata + (long long int)arg2.udata + (long long int)thread->carry_register;
    if(overflow & 0x100000000LL) // check 32nd bit for carry
      thread->carry_register = 1;
    else thread->carry_register = 0;
    break;
  case Instruction::SUB:
    result.idata = arg1.idata - arg2.idata;
    break;
  case Instruction::CMP:
    result.idata = arg2.idata - arg1.idata;
    break;
  case Instruction::CMPU:
    result.udata = arg2.udata - arg1.udata;
    if(arg1.udata > arg2.udata)
      result.udata |= 0x80000000u;
    else
      result.udata &= 0x7fffffffu;
    break;
  case Instruction::RSUB:
    result.idata = arg2.idata - arg1.idata;
    break;
  case Instruction::RSUBI:
    result.idata = ins.args[2] - arg1.idata;
    break;
  default:
    fprintf(stderr, "ERROR IntAddSub FOUND SOME OTHER OP\n");
    break;
  };
  
  // Write the value
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins)) {
    // pipeline hazard
    return false;
  }
  issued_this_cycle++;
  return true;
}

// From HardwareModule
void IntAddSub::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}

void IntAddSub::ClockFall() {
}

void IntAddSub::print() {
  printf("%d instructions in-flight",issued_this_cycle);
}

double IntAddSub::Utilization() {
  return static_cast<double>(issued_this_cycle)/
    static_cast<double>(width);
}
