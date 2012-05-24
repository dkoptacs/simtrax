#ifndef _SIMHWRT_THREAD_PROCESSOR_H_
#define _SIMHWRT_THREAD_PROCESSOR_H_
#include "ThreadState.h"
#include "Instruction.h"
#include <vector>

class IssueUnit;

struct ScheduleData{
  Instruction::Opcode last_issued;
  Instruction::Opcode last_stall;
};

class ThreadProcessor{
 public:
  std::vector<ThreadState*> thread_states;
  enum SchedulingScheme{
    SIMPLE=0,
    PRESTALL,
    POSTSTALL
  };
  SchedulingScheme schedule;
  int active_thread;
  int num_threads;
  int proc_id;
  bool halted;
  int num_halted;
  ThreadProcessor(int _num_threads, int num_regs, int _proc_id, SchedulingScheme ss, std::vector<Instruction*>* _instructions, std::vector<HardwareModule*> &modules, std::vector<FunctionalUnit*> *_functional_units, size_t threadprocid, size_t coreid, size_t l2id);
  ~ThreadProcessor();
  void Reset();
  bool ReadRegister(int which_reg, long long int which_cycle, reg_value &val, Instruction::Opcode &op);
  
  void ApplyWrites(long long int cur_cycle);
  
  void CompleteInstruction(Instruction* ins);
    
  // Begin new write queue stuff
  bool QueueWrite(int which_reg, reg_value val, long long int which_cycle, Instruction::Opcode op);
  
  void Schedule(Instruction::Opcode last_issued, Instruction::Opcode last_stall);
  
  void SwitchThread(Instruction::Opcode last_idssued, Instruction::Opcode last_stall);

  ThreadState* GetActiveThread();

  void EnableRegisterDump();
 
 private:
  std::vector<FunctionalUnit*> *functional_units;
  void SimpleScheduler();
  void PrestallScheduler();
  void PoststallScheduler();
  
};

#endif
