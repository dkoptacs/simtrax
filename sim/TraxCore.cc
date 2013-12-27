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
		   size_t coreid, size_t l2id)
  :num_thread_procs(_num_thread_procs), threads_per_proc(_threads_per_proc), num_regs(_num_regs) {

  instructions = _instructions;
  L2 = _L2;
  enable_proc_trace = -1;
  schedule = ss;
  core_id = coreid;
  l2_id = l2id;
  /*
  for (int i = 0; i < num_threads; i++) {
    SimpleRegisterFile* simple_regs = new SimpleRegisterFile(num_regs);
    long long int start_cycle = 0;
    simple_regs->WriteInt(0, 0, start_cycle);

    //simple_regs->WriteFloat(0, 1);
    simple_regs->WriteFloat(1, 2, start_cycle);
    simple_regs->WriteInt(8, 0, start_cycle);
    simple_regs->WriteInt(9, 10, start_cycle);
    simple_regs->WriteInt(12, 0, start_cycle);
    simple_regs->WriteInt(23, 30, start_cycle);
    register_files.push_back(simple_regs);
    modules.push_back(simple_regs);

    module_names.push_back("Register File");
  }
  */
}

TraxCore::~TraxCore(){
  size_t i;
  delete issuer;
  for(i=0; i<thread_procs.size(); i++){
    delete thread_procs[i];
  }
}

void TraxCore::initialize(int issue_verbosity, int num_icaches, int icache_banks, int simd_width, std::vector<int> jump_table) {
  // set up thread states
  for (int i = 0; i < num_thread_procs; i++) {
    ThreadProcessor *tp = new ThreadProcessor(threads_per_proc, num_regs, i, schedule, instructions, modules, &functional_units, (size_t)i, core_id, l2_id);
    if(i==enable_proc_trace){
      tp->EnableRegisterDump();
    }
    thread_procs.push_back(tp);

    //threads.push_back(new ThreadState(register_files[i], *instructions));
  }

  functional_units.push_back(dynamic_cast<FunctionalUnit*>(L1));
  issuer = new IssueUnit(thread_procs, functional_units, issue_verbosity, num_icaches, icache_banks, simd_width);
//   issuer = new NewIssueUnit(thread_procs, functional_units, issue_verbosity, num_icaches, icache_banks, simd_width);
  modules.push_back(issuer);
  module_names.push_back(std::string("IssueLogic"));

  for (size_t i = 0; i < modules.size(); i++) {
    LocalStore* ls_unit = dynamic_cast<LocalStore*>(modules[i]);
    if (ls_unit) {
      ls_unit->LoadJumpTable(jump_table);
    }
    utilizations.push_back(0.0);
  }

  cycle_num = 0;

}

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
