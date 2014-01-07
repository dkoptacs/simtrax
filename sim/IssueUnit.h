#ifndef _SIMHWRT_NEW_ISSUE_UNIT_H_
#define _SIMHWRT_NEW_ISSUE_UNIT_H_

#include "ThreadProcessor.h"
#include "HardwareModule.h"
#include "Instruction.h"
#include "FunctionalUnit.h"
#include <setjmp.h>
#include <vector>
#include <map>
#include <queue>
#include <string.h>

#define MAX_NUM_KERNELS 16

struct IssueStats
{
  double avg_issue;
  double avg_iCache_conflicts;
  double avg_fu_dependence;
  double avg_data_dependence;
  double avg_halted_count;
  double avg_misc_count;
  
  void AddStats(IssueStats otherStats)
  {
    avg_issue += otherStats.avg_issue;
    avg_iCache_conflicts += otherStats.avg_iCache_conflicts;
    avg_fu_dependence += otherStats.avg_fu_dependence;
    avg_data_dependence += otherStats.avg_data_dependence;
    avg_halted_count += otherStats.avg_halted_count;
    avg_misc_count += otherStats.avg_misc_count;
  }

};

// An IssueUnit takes the current ProgramCounter
// fetches the next instruction, executes whatever
// it can, manages dependencies and is notified by
// execution units when an instruction is complete.
// For example, if the IssueUnit is handed:
//
// LOAD R0, %addr
// LOAD R1, %addr2
// ADD  R2 <= R0 + R1
//
// It will first issue LOAD R0, then LOAD R1 and
// will force ADD to wait until it's dependent values
// are ready (only after both ins 0 and 1 commit)



class IssueUnit : public HardwareModule {
public:
  bool printed_single_kernel;
  long long int kernel_instruction_count[Instruction::NUM_OPS];
  long long int kernel_stall_cycles[Instruction::NUM_OPS];
  long long int kernel_fu_dependencies[Instruction::NUM_OPS];
/*   long long int kernel_executions[MAX_KERNEL_CYCLES]; */
  long long int **kernel_cycles;
  int **kernel_calls;
  int **kernel_profiling;
  long long int current_cycle;
  //long long int end_sleep_cycle;
/*   long long int total_kernel_stalls; */
/*   long long int total_kernel_fu_dependencies; */
/*   long long int start_clock_counter; */
/*   long long int profile_num_kernels; */
/*   long long int total_kernel_cycles; */
  long long int not_ready, not_fetched;
  long long int halted_count;
  long long int instructions_issued;
  long long int instructions_stalled;
  long long int instructions_misc;
  long long int instruction_id;
  long long int program_counter;
  long long int* writes_register;

  IssueUnit(const char* icache_params_file, std::vector<ThreadProcessor*>& _thread_procs, std::vector<FunctionalUnit*>& functional_units,
	    int verbosity, int num_icaches, int icache_banks, int simd_width);
  ~IssueUnit();

  void Reset();

  void ClockRise();
  void ClockFall();
  void print();
  void print(int total_system_TMs);

  void HaltSystem();

  void IssueVerbosity(ThreadState* thread, Instruction* fetched_instruction, size_t proc_id);
  void DataDependVerbosity(ThreadState* thread, Instruction* fetched_instruction, size_t proc_id);
  bool Issue(ThreadProcessor* tp, ThreadState* thread, Instruction* fetched_instruction, size_t proc_id);
  void MultipleIssueClockFall();
  void SIMDClockFall();
  void AddStats(IssueUnit* otherIssuer);
  void CalculateIssueStats();


  bool vector_stats;
  size_t num_halted;
  bool halted;
  int current_vec_ops;
  Instruction::Opcode lastOp;
  std::vector<int> vector_writes;
  long long int total_vector_ops[14];
  long long int last_pc;
  jmp_buf jump_location;
  std::vector<ThreadProcessor*> thread_procs;
  //std::map<int, int> thread_issue_count;
  int * thread_issue_count;
  size_t start_proc;
  int verbosity;
  std::vector<FILE*> thread_trace;
  FILE* all_trace;
  char *buf;
  //std::vector<Instruction*> all_instructions;
  std::vector<FunctionalUnit*> units;
  std::vector<Instruction*> issued_this_cycle;

  int *atominc_bins;
  long long int instruction_bins[Instruction::NUM_OPS];
  long long int unit_contention[Instruction::NUM_OPS];
  long long int data_depend_bins[Instruction::NUM_OPS];
  long long int fu_dependence, data_dependence;
  IssueStats issue_stats;
  
  // The current read queue for this TM
  int read_queue;

  //SIMD stuffs
  unsigned int simd_width;
  unsigned int *simd_last_issued;
  unsigned int *simd_state;
  long long int simd_stalls;
  long long int simd_issue;
  long long int simd_bonus_fetches;

  long long int *profile_instruction_count;
  long long int *profile_instruction_cycle_count;

  int num_icaches;
  int icache_banks;
  int **bank_fetched;
  static const int fetch_per_cycle = 2;
  long long int iCache_conflicts;
  long long int total_bank_cycles;
  long long int bank_cycles_used;
  ScheduleData *schedule_data;

  int current_proc_id;

#if 0
  Instruction* fetched_instruction;
  Instruction* issued_this_cycle;
  // if writes_register[i] == -1, register[i] is current else it depends on writes_register[i]
    int instructions_in_flight;

  
#endif
};

#endif // _SIMHWRT_NEW_ISSUE_UNIT_H_
