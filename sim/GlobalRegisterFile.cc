#include "GlobalRegisterFile.h"
//#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <cassert>
#ifndef WIN32
#  include <sys/time.h>
#else
#  include <time.h>
#endif

GlobalRegisterFile::GlobalRegisterFile(int num_regs, unsigned int sys_threads, unsigned int _report_period) :
  FunctionalUnit(0),
  num_registers(num_regs),
  total_system_threads(sys_threads),
  report_period(_report_period)
{
  fdata = new float[num_regs];
  for (int i = 0; i < num_regs; ++i) {
    udata[i] = 0;
  }
  //current_cycle = 0;
  latency = 1;
  issued_this_cycle = 0;
  last_report_cycle = 0;
}

GlobalRegisterFile::~GlobalRegisterFile()
{
  delete[] fdata;
}

void GlobalRegisterFile::Reset()
{
  for (int i = 0; i < num_registers; ++i) {
    udata[i] = 0;
  }
  issued_this_cycle = 0;
  last_report_cycle = 0;
}

int GlobalRegisterFile::ReadInt(int which_reg) const {
  assert (which_reg >= 0 && which_reg < num_registers);
  return idata[which_reg];
}
unsigned int GlobalRegisterFile::ReadUint(int which_reg) const {
  assert (which_reg >= 0 && which_reg < num_registers);
  return udata[which_reg];
}
float GlobalRegisterFile::ReadFloat(int which_reg) const {
  assert (which_reg >= 0 && which_reg < num_registers);
  return fdata[which_reg];
}

void GlobalRegisterFile::WriteInt(int which_reg, int value) {
  assert (which_reg >= 0 && which_reg < num_registers);
  idata[which_reg] = value;
}
void GlobalRegisterFile::WriteUint(int which_reg, unsigned int value) {
  assert (which_reg >= 0 && which_reg < num_registers);
  udata[which_reg] = value;
}
void GlobalRegisterFile::WriteFloat(int which_reg, float value) {
  assert (which_reg >= 0 && which_reg < num_registers);
  fdata[which_reg] = value;
}

bool GlobalRegisterFile::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::ATOMIC_INC || op == Instruction::ATOMIC_ADD || 
      op == Instruction::INC_RESET || op == Instruction::BARRIER || op == Instruction::GLOBAL_READ)
    return true;
  return false;
}

bool GlobalRegisterFile::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  static bool first_time = true;
#ifndef WIN32
  static timeval start_time;
#else
  static time_t start_time;
#endif
  if (issued_this_cycle >= 1) return false;
  int write_reg = ins.args[0];
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg;
  Instruction::Opcode failop = Instruction::NOP;

  // now get the result value
  reg_value result;
  result.udata = 0;
  pthread_mutex_lock(&global_mutex);
  switch (ins.op) {
  case Instruction::ATOMIC_INC:
    // Instruction is ATOMIC_INC: Copy args[1]++ to args[0]
    // args[1] is a global register, args[0] is a thread register
    result.udata = ReadUint(ins.args[1]);
    //printf(">=Atomic Inc %d = %d\n", ins.args[0], result.idata);
    // postincrement
    
    // report if enough cycles have passed
    if (report_period > 0 && last_report_cycle + report_period < issuer->current_cycle) {
#ifndef WIN32
      timeval current_time;
      gettimeofday( &current_time, NULL );
      if (first_time) {
        start_time = current_time;
        first_time = false;
      }
      printf("On cycle %lld\tTime(ms)\t%ld\tRegs\t", issuer->current_cycle,
	     (current_time.tv_sec - start_time.tv_sec)*1000 + (current_time.tv_usec - start_time.tv_usec)/1000 );
      print();
#else
      time_t current_time = time( NULL );
      if (first_time) {
        start_time = current_time;
        first_time = false;
      }
      const double diffTimeSec = difftime(current_time, start_time);
      printf("On cycle %lld\tTime(ms)\t%ld\tRegs\t", issuer->current_cycle, static_cast<unsigned long long>(diffTimeSec*1000));
      print();
#endif
      fflush(stdout);
      last_report_cycle += report_period;
    }
    

    WriteUint(ins.args[1], result.udata+1);
    break;
  case Instruction::ATOMIC_ADD:
    // Instruction is ATOMIC_ADD: Copy args[1]+=args[2] to args[0]
    // args[1] is a global register, args[0] and args[2] are local to the thread
    // Read the register
    if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg, failop)) {
      // bad stuff happened
      printf("Error in GlobalRegisterFile. Should have passed.\n");
    }
    result.udata = ReadUint(ins.args[1]) + arg.udata;

    WriteUint(ins.args[1], result.udata);

    break;

  case Instruction::GLOBAL_READ:
    result.udata = ReadUint(ins.args[1]);
    break;

  case Instruction::INC_RESET:
    result.udata = ReadUint(ins.args[1]);
    if(result.udata == total_system_threads - 1)
      WriteUint(ins.args[1], 0);
    else
      WriteUint(ins.args[1], result.udata + 1);
    break;

  case Instruction::BARRIER:
    result.udata = ReadUint(ins.args[0]);
    if(result.udata != 0)
      {
	pthread_mutex_unlock(&global_mutex);
	return false;
      }
    break;

  default:
    fprintf(stderr, "ERROR GlobalRegisterFile FOUND SOME OTHER OP\n");
    break;
  };


  if(ins.op != Instruction::BARRIER) // barrier doesn't write anything
    {
      if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins)) {
	// pipeline hazzard, undo changes
	switch (ins.op) {
	case Instruction::ATOMIC_INC:
	  WriteUint(ins.args[1], result.udata);
	  break;
	case Instruction::ATOMIC_ADD:
	  result.udata = ReadUint(ins.args[1]) - arg.udata;
	  WriteUint(ins.args[1], result.udata);
	  break;
	case Instruction::INC_RESET:
	  WriteUint(ins.args[1], result.udata);
	  break;
	default:
	  break;
	};    
	pthread_mutex_unlock(&global_mutex);
	return false;
      }
    }

  pthread_mutex_unlock(&global_mutex);
  issued_this_cycle++;
  //printf("global reg returning true\n");
  return true;
}

// From HardwareModule
void GlobalRegisterFile::ClockRise() {}
void GlobalRegisterFile::ClockFall() {
  // needs fixing
  issued_this_cycle = 0;
}

void GlobalRegisterFile::print() {
  // print 8 regs per line?
//   printf("\n");
  int num_per_row = 8;
  for (int row = 0; row < num_registers / num_per_row; row++) {
    for (int col = 0; col < num_per_row; col++) {
      int which_reg = row * num_per_row + col;
      if (which_reg == num_registers)
        break;
      printf("[%02d]\t%u\t", which_reg, udata[which_reg]);
    }
    printf("\n");
  }
}
