#include "IntAddSub.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>
#include <stdlib.h>

extern std::vector<std::string> source_names;

bool OverflowOnAdd(int a, int b);
int Extend16(int a);

IntAddSub::IntAddSub(int _latency, int _width) :
    FunctionalUnit(_latency), width(_width)
{
  issued_this_cycle = 0;
}

// From FunctionalUnit
bool IntAddSub::SupportsOp(Instruction::Opcode op) const
{
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
      op == Instruction::RSUBI ||

      // MIPS
      op == Instruction::addi ||
      op == Instruction::addiu ||
      op == Instruction::addu ||
      op == Instruction::slt ||
      op == Instruction::slti ||
      op == Instruction::sltu ||
      op == Instruction::sltiu ||
      op == Instruction::subu ||
      op == Instruction::negu ||
      op == Instruction::teq )
    return true;
  else
    return false;
}

bool IntAddSub::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  if (issued_this_cycle >= width) return false;

  reg_value arg0, arg1, arg2;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  Instruction::Opcode failop = Instruction::NOP;

  // Read the registers
  // Single register operands
  if (ins.op == Instruction::ADDI  ||
      ins.op == Instruction::RSUBI ||
      ins.op == Instruction::addi  ||
      ins.op == Instruction::addiu ||
      ins.op == Instruction::slti ||
      ins.op == Instruction::sltiu ||
      ins.op == Instruction::negu)
  {
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
    {
      // bad stuff happened
      printf("IntAddSub unit: Error in Accepting instruction. Should have passed.\n");
      exit(1);
    }
  }

  // Two register operands (0, 1)
  else if (ins.op == Instruction::teq)
  {
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop) ||
        !thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
    {
      printf("IntAddSub unit: Error in Accepting instruction. Should have passed.\n");
      exit(1);
    }
  }

  else
  {
    // Two register operands (1, 2)
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) ||
        !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
    {
      // bad stuff happened
      printf("IntAddSub unit: Error in Accepting instruction. Should have passed.\n");
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
  switch (ins.op)
  {

    case Instruction::addu:
      result.idata = arg1.idata + arg2.idata;
      break;

    case Instruction::subu:
      result.idata = arg1.idata - arg2.idata;
      break;

    case Instruction::negu:
      result.idata = -arg1.idata;
      break;

    case Instruction::slt:
      result.idata = (arg1.idata < arg2.idata);
      break;

    case Instruction::sltu:
      result.udata = (arg1.udata < arg2.udata);
      break;

    case Instruction::slti:
      result.idata = (arg1.idata < ins.args[2]);
      break;

    case Instruction::sltiu:
      result.idata = (arg1.udata < (unsigned)ins.args[2]);
      break;

    case Instruction::teq:
      if(arg0.udata == arg1.udata)
      {
        printf("TRAP (teq): PC = %d, thread = %d, core = %d, cycle = %lld\n",
               ins.pc_address, thread->thread_id, (int)thread->core_id, issuer->current_cycle);
        if(ins.srcInfo.fileNum >= 0)
          printf("%s: %d\n", source_names[ins.srcInfo.fileNum].c_str(), ins.srcInfo.lineNum);
        else
          printf("Compile your TRaX program with -g for more info\n");
        exit(1);
      }
      break;

    case Instruction::ADD:
      result.idata = arg1.idata + arg2.idata;
      break;

    case Instruction::ADDK:
      thread->carry_register = 0;
    case Instruction::ADDKC:
      result.idata = arg1.idata + arg2.idata + thread->carry_register;

      overflow =
          (long long int)arg1.udata +
          (long long int)arg2.udata +
          (long long int)thread->carry_register;

      // check 32nd bit for carry
      if(overflow & 0x100000000LL)
        thread->carry_register = 1;
      else
        thread->carry_register = 0;

      break;

    case Instruction::ADDI:
      result.idata = arg1.idata + ins.args[2];
      break;

    case Instruction::addi:
      if(OverflowOnAdd(arg1.idata, ins.args[2]))
      {
        printf("TRAP: Overflow occured when executing instruction %d with operands %d, %d\n",
               ins.pc_address, arg1.idata, ins.args[2]);
        exit(1);
      }

      result.idata = arg1.idata + Extend16(ins.args[2]);
      break;

    case Instruction::addiu:
      result.idata = arg1.idata + Extend16(ins.args[2]);
      break;

    case Instruction::ADDIK:
      thread->carry_register = 0;
    case Instruction::ADDIKC:
      result.idata = arg1.idata + ins.args[2] + thread->carry_register;

      overflow =
          (long long int)arg1.udata +
          (long long int)arg2.udata +
          (long long int)thread->carry_register;

      // check 32nd bit for carry
      if(overflow & 0x100000000LL)
        thread->carry_register = 1;
      else
        thread->carry_register = 0;

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
  if(ins.op != Instruction::teq) // trap doesn't write anything
  {
    if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins))
    {
      // pipeline hazard
      return false;
    }
  }

  issued_this_cycle++;
  return true;
}

// From HardwareModule
void IntAddSub::ClockRise()
{
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
  issued_this_cycle = 0;
}

void IntAddSub::ClockFall()
{
}

void IntAddSub::print()
{
  printf("%d instructions in-flight",issued_this_cycle);
}

double IntAddSub::Utilization()
{
  return static_cast<double>(issued_this_cycle) / static_cast<double>(width);
}

// some mips immediates need to be sign extended
int Extend16(int a)
{
  return (a << 16) >> 16;
}

bool OverflowOnAdd(int a, int b)
{
  int sum = a + b;

  // Opposite signs on inputs can't overflow
  if((a & 0x80000000u) != (b & 0x80000000u))
    return false;

  if((a & 0x80000000u) != (sum & 0x80000000u))
    return true;

  return false;
}
