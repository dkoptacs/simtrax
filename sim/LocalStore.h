#ifndef _SIMHWRT_LOCAL_STORE_H_
#define _SIMHWRT_LOCAL_STORE_H_

#include "FunctionalUnit.h"
#include "ThreadState.h"
#include "FourByte.h"

// Hard-code these values since we only use one type of localstore
#define LOCALSTORE_AREA .01396329
#define LOCALSTORE_ENERGY .00692867


class LocalStore : public FunctionalUnit {
public:
  LocalStore(int latency, int width);
  ~LocalStore();
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  virtual double Utilization();

  bool IssueLoad(int write_reg, int address, ThreadState* thread, long long int write_cycle, Instruction& ins);
  bool IssueStore(reg_value write_val, int address, ThreadState* thread, long long int write_cycle, Instruction& ins);
  void LoadJumpTable(std::vector<int> jump_table);
  void LoadAsciiLiterals(std::vector<std::string> ascii_literals);

  int jtable_size;
  int width;
  int issued_this_cycle;

  FourByte ** storage;
};

#endif // _SIMHWRT_LOCAL_STORE_H_

