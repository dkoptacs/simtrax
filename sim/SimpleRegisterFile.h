#ifndef _SIMHWRT_SIMPLE_REGISTER_FILE_H_
#define _SIMHWRT_SIMPLE_REGISTER_FILE_H_

// A super simple register file that assumes every
// register is 32-bits wide and therefore "castable"
// to any type you call it.
#include "FunctionalUnit.h"
#include "DiskBuffer.h"
#include <stdint.h>

#define HI_REG 4
#define LO_REG 5

// Hard-code these values since we only use one type of register file
#define RF_AREA .0147665
#define RF_ENERGY .0079

struct RegisterTraceV1 {
  uint64_t cycle;
  enum {
    Read=0,
    Write
  } access_type;
  uint8_t register_number;
  uint16_t thread_id;
  static const unsigned version = 1;
};
typedef RegisterTraceV1 RegisterTrace;

class SimpleRegisterFile : public FunctionalUnit {
 public:
  long long int current_cycle;
  SimpleRegisterFile(int num_regs, int _thread_id);
  ~SimpleRegisterFile();

  int ReadInt(int which_reg, long long int which_cycle) const;
  unsigned int ReadUint(int which_reg, long long int which_cycle) const;
  float ReadFloat(int which_reg, long long int which_cycle) const;
  void ReadForwarded(int which_reg, long long int which_cycle) const;

  void WriteInt(int which_reg, int value, long long int which_cycle);
  void WriteUint(int which_reg, unsigned int value, long long int which_cycle);
  void WriteFloat(int which_reg, float value, long long int which_cycle);

  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();

  // for register trace
  DiskBuffer<RegisterTrace, unsigned> *buffer;
  int thread_id;
  void EnableDump(char *filename);

  int num_registers;
  union {
    int* idata;
    unsigned int* udata;
    float* fdata;
  };
  int issued_this_cycle;
  // just fillers, should both be 1
  int width;
};

union reg_value {
  int idata;
  unsigned int udata;
  float fdata;
};

#endif // _SIMHWRT_SIMPLE_REGISTER_FILE_H_
