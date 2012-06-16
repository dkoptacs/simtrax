#ifndef _SIMHWRT_THREAD_STATE_H_
#define _SIMHWRT_THREAD_STATE_H_

#include <queue>
#include <vector>

#include "Instruction.h"
#include "SimpleRegisterFile.h"

class Instruction;
class SimpleRegisterFile;
class WriteRequest;

class WriteQueue {
 public:
  WriteQueue();
  ~WriteQueue();
  void clear();
  WriteRequest* push(long long int cycle);
  WriteRequest* front();
  int size();
  void pop();
  bool empty();
  bool CycleUsed(long long int cycle);
  Instruction::Opcode GetOp(int which_reg);
  bool ReadyBy(int which_reg, long long int which_cycle, reg_value &val, Instruction::Opcode &op);
  void print();

  WriteRequest* requests;
  int head;
  int tail;
};


class ThreadState {
public:
  long long int instructions_issued;
  long long int program_counter;
  long long int next_program_counter;
  long long int instruction_id;
  long long int* register_ready;
  unsigned int thread_id;
  size_t core_id;
  int carry_register;
  ThreadState(SimpleRegisterFile* regs,
              std::vector<Instruction*>& instructions,
	      unsigned int thread_id, unsigned int core_id);
  ~ThreadState();
  void Reset();
  // Deprecating the following
  //WriteRequest* RequestWrite(int which_reg, long long int ready_cycle, Instruction::Opcode op);
  // end deprecated

  bool ReadRegister(int which_reg, long long int which_cycle, reg_value &val, Instruction::Opcode &op);

  void ApplyWrites(long long int cur_cycle);

  void CompleteInstruction(Instruction* ins);

  SimpleRegisterFile* registers;
  std::vector<Instruction*> instructions;

  // Begin new write queue stuff
  bool QueueWrite(int which_reg, reg_value val, long long int which_cycle, Instruction::Opcode op);
  Instruction::Opcode GetFailOp(int which_reg);

  void Sleep(int num_cycles);
  
  bool Wake(bool force=false);


  WriteQueue write_requests;
  
  int* writes_in_flight;
  // end new write queue


  Instruction* fetched_instruction;
  Instruction* issued_this_cycle;
  
  int instructions_in_flight;
  int sleep_cycles;
  long long int last_issue;
  bool halted;
  
};

#endif // _SIMHWRT_THREAD_STATE_H_
