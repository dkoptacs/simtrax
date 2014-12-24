#include <vector>

#include "FunctionalUnit.h"
#include "IssueUnit.h"
#include "L1Cache.h"
#include "L2Cache.h"
#include "SimpleRegisterFile.h"
#include "ThreadState.h"
#include "TraxCore.h"

TraxCore::TraxCore(int _num_thread_procs, int _threads_per_proc, int _num_regs,
		   ThreadProcessor::SchedulingScheme ss, std::vector<Instruction*>* _instructions, 
		   L2Cache* _L2,
		   size_t coreid, size_t l2id, bool _enable_profiling)
  :num_thread_procs(_num_thread_procs), threads_per_proc(_threads_per_proc), num_regs(_num_regs) {

  enable_profiling = _enable_profiling;
  instructions = _instructions;
  L2 = _L2;
  enable_proc_trace = -1;
  schedule = ss;
  core_id = coreid;
  l2_id = l2id;
}

TraxCore::~TraxCore(){
  size_t i;
  delete issuer;
  for(i=0; i<thread_procs.size(); i++){
    delete thread_procs[i];
  }
}

void TraxCore::initialize(const char* icache_params_file, int issue_verbosity, 
			  int num_icaches, int icache_banks, int simd_width, 
			  char* jump_table, int jtable_size, 
			  std::vector<std::string> ascii_literals) {
  // set up thread states
  for (int i = 0; i < num_thread_procs; i++) {
    ThreadProcessor *tp = new ThreadProcessor(threads_per_proc, num_regs, i, schedule, instructions, modules, &functional_units, (size_t)i, core_id, l2_id);
    if(i==enable_proc_trace){
      tp->EnableRegisterDump();
    }
    thread_procs.push_back(tp);
  }

  functional_units.push_back(dynamic_cast<FunctionalUnit*>(L1));
  issuer = new IssueUnit(icache_params_file, thread_procs, functional_units, issue_verbosity, num_icaches, icache_banks, simd_width, enable_profiling);
  modules.push_back(issuer);
  module_names.push_back(std::string("IssueLogic"));

  // Find the LocalStore (stack) unit and pre-load the data segment
  for (size_t i = 0; i < modules.size(); i++) {
    LocalStore* ls_unit = dynamic_cast<LocalStore*>(modules[i]);
    if (ls_unit) {
      ls_unit->LoadJumpTable(jump_table, jtable_size);
      //ls_unit->LoadAsciiLiterals(ascii_literals);

      // While we have a pointer to the local store, loop through units again to find DebugUnit (ugh)
      // DebugUnit needs pointer to stack for PRINTF instruction to work
      for (size_t i = 0; i < functional_units.size(); i++) {
	DebugUnit* debug = dynamic_cast<DebugUnit*>(functional_units[i]);
	if(debug)
	  debug->setLocalStore(ls_unit);
      }
    }
    utilizations.push_back(0.0);
  }

  cycle_num = 0;

}

// Resets all gathered statistics
void TraxCore::Reset()
{
  cycle_num = 0;
  for(int i=0; i < num_thread_procs; i++)
    thread_procs[i]->Reset();
  issuer->Reset();
  L1->Reset();
}


void TraxCore::EnableRegisterDump(int proc_num){
  enable_proc_trace = proc_num;
}

// Sets a pointer to the symbol names used in the assembly for better printing info
void TraxCore::SetSymbols(std::vector<symbol*> *regs)
{
  for (size_t i = 0; i < functional_units.size(); i++) {
    DebugUnit* debug = dynamic_cast<DebugUnit*>(functional_units[i]);
    if(debug)
      debug->setRegisters(regs);
  }
}

long long int TraxCore::CountStalls() {
  long long int stall_cycles = 0;
  for (int i = 0; i < num_thread_procs; i++) {
    stall_cycles += issuer->current_cycle - thread_procs[i]->GetActiveThread()->last_issue;
  }
  return stall_cycles;
}

// This function is for stats-tracking only.
// We will use one core to hold the sums of all other cores' stats
void TraxCore::AddStats(TraxCore* otherCore)
{
  // Module utilizations
  for(size_t i = 0; i < utilizations.size(); i++)
    utilizations[i] += otherCore->utilizations[i];
  
  // Per-instruction stats
  issuer->AddStats(otherCore->issuer);
  
  // L1 cache stats
  L1->AddStats(otherCore->L1);

}
