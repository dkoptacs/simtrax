#ifndef _SIMHWRT_GLOBAL_REGISTER_FILE_H_
#define _SIMHWRT_GLOBAL_REGISTER_FILE_H_

#include <pthread.h>

// Global int registers with adders and counters built in
// services ATOMIC_INC and ATOMIC_ADD instructions
#include "FunctionalUnit.h"

extern pthread_mutex_t global_mutex;

class GlobalRegisterFile : public FunctionalUnit {
 public:
  GlobalRegisterFile(int num_regs, unsigned int sys_threads, unsigned int report_period);
  ~GlobalRegisterFile();

  void Reset();

  int ReadInt(int which_reg) const;
  unsigned int ReadUint(int which_reg) const;
  float ReadFloat(int which_reg) const;

  void WriteInt(int which_reg, int value);
  void WriteUint(int which_reg, unsigned int value);
  void WriteFloat(int which_reg, float value);

  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();

  int num_registers;
  union {
    int* idata;
    unsigned int* udata;
    float* fdata;
  };
  int issued_this_cycle;
  unsigned int total_system_threads;
  unsigned int last_report_cycle;
  unsigned int report_period;
};

#endif // _SIMHWRT_GLOBAL_REGISTER_FILE_H_
