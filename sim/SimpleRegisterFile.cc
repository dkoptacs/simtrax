#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <cassert>

SimpleRegisterFile::SimpleRegisterFile(int num_regs, int _thread_id) :
    FunctionalUnit(0),
    num_registers(num_regs)
{

  // Hard-code area and energy since they are unlikely to change
  area = 0.0147665;
  energy = 0.00789613;

  fdata = new float[num_regs];
  buffer = NULL;
  
  // this should be the actual thread id
  thread_id = _thread_id;
  current_cycle = 0;
  issued_this_cycle = 0;
  latency = 1;
  width = 1;
}

SimpleRegisterFile::~SimpleRegisterFile()
{
  if (buffer)
  {
    delete buffer;
    buffer = NULL;
  }
}

void SimpleRegisterFile::EnableDump(char *filename)
{
  buffer = new DiskBuffer<RegisterTrace, unsigned>(filename, 4096, RegisterTrace::version);
}

int SimpleRegisterFile::ReadInt(int which_reg, long long int which_cycle) const
{
  if (buffer)
  {
    //RegisterTrace temp = {which_cycle, RegisterTrace::Read, which_reg, thread_id};
    //buffer->AddItem(temp);
  }

  assert (which_reg >= 0 && which_reg < num_registers);
  return idata[which_reg];
}
unsigned int SimpleRegisterFile::ReadUint(int which_reg, long long int which_cycle) const
{
  if (buffer)
  {
    //RegisterTrace temp = {which_cycle, RegisterTrace::Read, which_reg, thread_id};
    //buffer->AddItem(temp);
  }

  assert (which_reg >= 0 && which_reg < num_registers);
  return udata[which_reg];
}
float SimpleRegisterFile::ReadFloat(int which_reg, long long int which_cycle) const
{
  if (buffer)
  {
    //RegisterTrace temp = {which_cycle, RegisterTrace::Read, which_reg, thread_id};
    //buffer->AddItem(temp);
  }

  assert (which_reg >= 0 && which_reg < num_registers);
  return fdata[which_reg];
}
void SimpleRegisterFile::ReadForwarded(int which_reg, long long int which_cycle) const
{
  if (buffer)
  {
    //RegisterTrace temp = {which_cycle, RegisterTrace::Read, which_reg, thread_id};
    //buffer->AddItem(temp);
  }

  assert (which_reg >= 0 && which_reg < num_registers);
}

void SimpleRegisterFile::WriteInt(int which_reg, int value, long long int which_cycle)
{
  if (buffer)
  {
    //RegisterTrace temp = {which_cycle, RegisterTrace::Write, which_reg, thread_id};
    //buffer->AddItem(temp);
  }
  assert (which_reg >= 0 && which_reg < num_registers);
  idata[which_reg] = value;
}

void SimpleRegisterFile::WriteUint(int which_reg, unsigned int value, long long int which_cycle)
{
  if (buffer)
  {
    //RegisterTrace temp = {which_cycle, RegisterTrace::Write, which_reg, thread_id};
    //buffer->AddItem(temp);
  }

  assert (which_reg >= 0 && which_reg < num_registers);
  udata[which_reg] = value;
}

void SimpleRegisterFile::WriteFloat(int which_reg, float value, long long int which_cycle)
{
  if (buffer)
  {
    //RegisterTrace temp = {which_cycle, RegisterTrace::Write, which_reg, thread_id};
    //buffer->AddItem(temp);
  }

  assert (which_reg >= 0 && which_reg < num_registers);
  fdata[which_reg] = value;
}


// MIPS NOTE: move lui to LocalStore
bool SimpleRegisterFile::SupportsOp(Instruction::Opcode op) const
{
  if (op == Instruction::MOV ||
      op == Instruction::LOADIMM ||
      op == Instruction::MOVINDRD ||
      op == Instruction::MOVINDWR ||
      op == Instruction::lui ||
      op == Instruction::mfc1 ||
      op == Instruction::mfhi ||
      op == Instruction::mflo ||
      op == Instruction::movf ||
      op == Instruction::movn ||
      op == Instruction::movt ||
      op == Instruction::mov_s ||
      op == Instruction::movn_s ||
      op == Instruction::movz_s ||
      op == Instruction::movt_s ||
      op == Instruction::move ||
      op == Instruction::mtc1)
    return true;

  return false;
}

bool SimpleRegisterFile::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  if (issued_this_cycle >= width) return false;

  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;

  // now get the result value
  reg_value result;
  reg_value arg1, arg2;
  Instruction::Opcode failop = Instruction::NOP;

  switch (ins.op)
  {
    // MIPS!
    case Instruction::lui:
      result.udata = ins.args[1] << 16;
      break;

    case Instruction::movf:
      // Read the register
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error with MIPS movf.\n");
      }

      // Only move if CC is equal to 0.
      if (thread->compare_register)
      {
        issued_this_cycle++;
        return true;
      }

      result.idata = arg1.idata;
      break;

    case Instruction::movt:
      // Read the register
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error with MIPS movt.\n");
      }

      // Only move if CC is not equal to 0.
      if (!thread->compare_register)
      {
        issued_this_cycle++;
        return true;
      }

      result.idata = arg1.idata;
      break;

    case Instruction::movn:
    case Instruction::movn_s:
      // Read the registers
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)
          ||
          !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error with MIPS movn.\n");
      }

      // Only move if rt is not equal to 0.
      if (arg2.idata == 0)
      {
        issued_this_cycle++;
        return true;
      }

      result.udata = arg1.udata;
      break;

    case Instruction::movz_s:
      // Read the registers
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)
          ||
          !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error with MIPS movz_s.\n");
      }

      // Only move if rt is equal to 0.
      if (arg2.idata != 0)
      {
        issued_this_cycle++;
        return true;
      }

      result.fdata = arg1.fdata;
      break;

    case Instruction::movt_s:
      // Read the registers
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)
          ||
          !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error with MIPS movt_s.\n");
      }

      // Only move if rt is equal to 1.
      if (arg2.idata != 1)
      {
        issued_this_cycle++;
        return true;
      }

      result.fdata = arg1.fdata;
      break;


      // MFHI and MFLO don't read normal registers
    case Instruction::mfhi:
      // Read the register
      if (!thread->ReadRegister(HI_REG, issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction (mfhi). Should have passed.\n");
      }
      
      result.udata = arg1.udata;
      write_reg = ins.args[0];
      break;

    case Instruction::mflo:
      // Read the register
      if (!thread->ReadRegister(LO_REG, issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction (mflo). Should have passed.\n");
      }
      
      result.udata = arg1.udata;
      write_reg = ins.args[0];
      break;

    case Instruction::mfc1:
      // Read the register
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction. Should have passed.\n");
      }
      
      result.udata = arg1.udata;
      write_reg = ins.args[0];
      break;

    case Instruction::mtc1:
      // Read the register
      if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction. Should have passed.\n");
      }
      
      result.udata = arg1.udata;
      write_reg = ins.args[1];
      break;

    case Instruction::mov_s:
    case Instruction::MOV:
    case Instruction::move:
      // Read the register
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction. Should have passed.\n");
      }
      
      result.udata = arg1.udata;
      break;

    case Instruction::LOADIMM:
      result.idata = ins.args[1];
      break;

    case Instruction::MOVINDRD:
      // indirect move
      // Read the registers
      if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction. Should have passed.\n");
      }
      //printf("ins.args[2] = %d, arg2.idata = %d, fdata = %f\n", ins.args[2], arg2.idata, arg2.fdata);
      if (!thread->ReadRegister(ins.args[1] + arg2.idata, issuer->current_cycle, arg1, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction. Should have passed.\n");
      }
      
      result.udata = arg1.udata;
      break;

    case Instruction::MOVINDWR:
      // indirect move
      // Read the registers
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop) ||
          !thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // bad stuff happened
        printf("SimpleRegisterFile unit: Error in Accepting instruction. Should have passed.\n");
      }
      
      write_reg += arg2.idata;
      result.udata = arg1.udata;
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
void SimpleRegisterFile::ClockRise() {}

void SimpleRegisterFile::ClockFall()
{
  issued_this_cycle = 0;
  current_cycle++;
}

void SimpleRegisterFile::print()
{
  // print 8 regs per line?
  printf("\n");
  int num_per_row = 8;

  for (int row = 0; row < num_registers / num_per_row; row++)
  {
    for (int col = 0; col < num_per_row; col++)
    {
      int which_reg = row * num_per_row + col;

      if (which_reg == num_registers)
        break;

      printf("%02d: %u  ", which_reg, udata[which_reg]);
    }
    printf("\n");
  }
}
