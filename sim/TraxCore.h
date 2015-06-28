#ifndef _SIMHWRT_TRAX_CORE_H_
#define _SIMHWRT_TRAX_CORE_H_

// In our current terminology, a TraxCore is actually a TM.

#include "ThreadProcessor.h"
#include "FunctionalUnit.h"
#include "Assembler.h"
#include "DebugUnit.h"
#include "LocalStore.h"
#include "Profiler.h"
#include "Debugger.h"
#include <pthread.h>

class Instruction;
class SimpleRegisterFile;
class ThreadState;
class IssueUnit;
class OldIssueUnit;
class L2Cache;
class L1Cache;

// A single Trax core
class TraxCore {
public:
  long long int cycle_num;
  TraxCore(int num_thread_procs, int _threads_per_core, int num_regs, 
	   ThreadProcessor::SchedulingScheme ss, std::vector<Instruction*>* instructions, 
	   L2Cache* L2, size_t coreid, size_t l2id, bool _enable_profiling, 
	   Profiler* _profiler, Debugger* _debugger);
  ~TraxCore();

  // sets up issue unit and thread states
  void initialize(const char* icache_params_file, int issue_verbosity, int num_icaches, 
		  int icache_banks, int simd_width, char* jump_table, int jtable_size,
		  std::vector<std::string> ascii_literals);
  void EnableRegisterDump(int proc_num);
  void Reset();
  void SetSymbols(std::vector<symbol*> *regs);
  void AddStats(TraxCore* otherCore);

  // count thread stalls for fairness
  long long int CountStalls();

  //data members...
  bool enable_profiling;
  Profiler* profiler;
  Debugger* debugger;
  int num_thread_procs;
  int threads_per_proc;
  int num_regs;
  int enable_proc_trace;
  size_t core_id;
  size_t l2_id;
  // for pthread synchronization ---
  int *num_cores_left;
  int *current_num_cores;
  pthread_mutex_t* sync_mutex;
  pthread_cond_t* sync_cond;
  //--------------------------------
  ThreadProcessor::SchedulingScheme schedule;
  // some memory pointers to chipwide memory and data member for local memory
  L2Cache* L2;
  L1Cache* L1;

  // instructions just pointed to
  std::vector<Instruction*>* instructions;
  std::vector<ThreadProcessor*> thread_procs;
  IssueUnit* issuer;

  // modules from loadConfig
  std::vector<HardwareModule*> modules;
  std::vector<double> utilizations;
  std::vector<std::string> module_names;
  std::vector<FunctionalUnit*> functional_units;

  // memory is going to be a little tricky
  MemoryBase* memory;

};

#endif // _SIMHWRT_TRAX_CORE_H_
