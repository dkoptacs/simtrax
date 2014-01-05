#include "IssueUnit.h"
#include "ThreadState.h"
#include "SimpleRegisterFile.h"
#include "L1Cache.h"
#include "FPMul.h"
#include "FPAddSub.h"
#include "ReadConfig.h"
#include "memory_controller.h"
#include <stdlib.h>
#include <fstream>

IssueUnit::IssueUnit(std::vector<ThreadProcessor*>& _thread_procs,
                     std::vector<FunctionalUnit*>& functional_units,
                     int _verbosity, int _num_icaches, int _icache_banks, 
		     int _simd_width) 
 :verbosity(_verbosity){
  printed_single_kernel = false;

  if(!ReadCacheParams(4096, _icache_banks, 4, area, energy, false))
    perror("WARNING: Unable to find area and energy profile for specified instruction cache.\nAssuming 0\n");
  
  area *= _num_icaches;
  
  // flag denoting stats have not been calculated yet
  issue_stats.avg_issue = -1.0;

  num_halted = 0;
  halted = false;
  vector_stats = false;
  current_vec_ops = 0;
  lastOp = Instruction::NOP;
  last_pc = -1;
  for(int i=0; i < 14; i++)
    total_vector_ops[i] = 0;

  kernel_cycles = new long long int*[MAX_NUM_KERNELS];
  kernel_calls = new int*[MAX_NUM_KERNELS];
  kernel_profiling = new int*[MAX_NUM_KERNELS];

  for (size_t i = 0; i < MAX_NUM_KERNELS; ++i) {
    kernel_cycles[i] = new long long int[_thread_procs.size()];
    kernel_calls[i] = new int[_thread_procs.size()];
    kernel_profiling[i] = new int[_thread_procs.size()];
    for(size_t j = 0; j < _thread_procs.size(); j++)
      {
	// set kernel profiling to off
	kernel_profiling[i][j] = 0;
	//initialize cycles and calls for each kernel
	kernel_cycles[i][j] = 0;
	kernel_calls[i][j] = 0;
      }
  }
//   start_clock_counter = -1;
//   profile_num_kernels = 0;
//   total_kernel_cycles = 0;
//   total_kernel_stalls = 0;
//   total_kernel_fu_dependencies = 0;
  schedule_data = new ScheduleData[_thread_procs.size()];

  int count = 0;
  thread_issue_count = new int[_thread_procs.size()+1];
  for (size_t i = 0; i < _thread_procs.size(); i++) {
    thread_procs.push_back(_thread_procs[i]);
    thread_issue_count[count++] = 0;
    schedule_data[i].last_issued = Instruction::NOP;
    schedule_data[i].last_stall = Instruction::NOP;
  }
  // for counting the max number of threads
  thread_issue_count[count] = 0;

  for (size_t i = 0; i < functional_units.size(); i++) {
    units.push_back(functional_units[i]);
  }

  current_cycle  = 0;
  //end_sleep_cycle = -1;
  start_proc = 0;

  for (size_t i = 0; i < Instruction::NUM_OPS; i++) {
    instruction_bins[i] = 0;
    unit_contention[i] = 0;
    data_depend_bins[i] = 0;
  }
  fu_dependence = 0;
  data_dependence = 0;
  not_ready = 0;
  not_fetched = 0;
  halted_count = 0;

  size_t instruction_count = thread_procs[0]->GetActiveThread()->instructions.size()+1;
  profile_instruction_count = new long long int[instruction_count];
  profile_instruction_cycle_count = new long long int[instruction_count];
  for (size_t i = 0; i < instruction_count; i++) {
    profile_instruction_count[i] = 0;
    profile_instruction_cycle_count[i] = 0;
  }

  instructions_issued = 0;
  instructions_stalled = 0;
  instructions_misc = 0;

  // output files
  if (verbosity > 1) {
    buf = new char[100];
    thread_trace.resize(thread_procs.size());
    for (size_t i = 0; i < thread_procs.size(); i++) {
      sprintf(buf, "t%d_trace.txt", static_cast<int>(i));
      thread_trace[i] = fopen(buf, "w");
    }
    all_trace = fopen("t_all_trace.txt", "w");
  }

  // tracking who atomincs
  atominc_bins = new int[thread_procs.size()];
  for (size_t i = 0; i < thread_procs.size(); ++i) {
    // set to -1 to make the first thread in the block issue first
    atominc_bins[i] = 0;
  }

  // Start reading queue 0
  read_queue = 0;

  // SIMD setup
  simd_width = _simd_width;
  simd_last_issued = new unsigned int[thread_procs.size()/simd_width];
  for (size_t i = 0; i < thread_procs.size()/simd_width; ++i) {
    // set to -1 to make the first thread in the block issue first
    simd_last_issued[i] = -1;
  }
  simd_state = new unsigned int[thread_procs.size()];
  for (size_t i = 0; i < thread_procs.size(); ++i) {
    simd_state[i] = 0;
  }
  simd_stalls = 0;
  simd_issue = 0;
  simd_bonus_fetches = 0;

  // iCache setup
  num_icaches = _num_icaches;  
  icache_banks = _icache_banks;
  bank_fetched = new int*[num_icaches];
  for (int j = 0; j < num_icaches; ++j) {
    bank_fetched[j] = new int[icache_banks];
    for (int i = 0; i < icache_banks; ++i) {
      bank_fetched[j][i] = 0;
    }
  }
  iCache_conflicts = 0;
  total_bank_cycles = 0;
  bank_cycles_used = 0;
}

void IssueUnit::Reset()
{
  num_halted = 0;
  halted = false;
  current_vec_ops = 0;
  lastOp = Instruction::NOP;
  last_pc = -1;
  for(int i=0; i < 14; i++)
    total_vector_ops[i] = 0;



  for (size_t i = 0; i < MAX_NUM_KERNELS; ++i) {
    for(size_t j = 0; j < thread_procs.size(); j++)
      {
	// set kernel profiling to off
	kernel_profiling[i][j] = 0;
	//initialize cycles and calls for each kernel
	kernel_cycles[i][j] = 0;
	kernel_calls[i][j] = 0;
      }
  }
  
  int count = 0;
  for (size_t i = 0; i < thread_procs.size(); i++) {
    thread_issue_count[count++] = 0;
    schedule_data[i].last_issued = Instruction::NOP;
    schedule_data[i].last_stall = Instruction::NOP;
  }
  // for counting the max number of threads
  thread_issue_count[count] = 0;

  current_cycle  = 0;
  //end_sleep_cycle = -1;
  start_proc = 0;

  for (size_t i = 0; i < Instruction::NUM_OPS; i++) {
    instruction_bins[i] = 0;
    unit_contention[i] = 0;
    data_depend_bins[i] = 0;
  }
  fu_dependence = 0;
  data_dependence = 0;
  not_ready = 0;
  not_fetched = 0;
  halted_count = 0;

  size_t instruction_count = thread_procs[0]->GetActiveThread()->instructions.size()+1;
  for (size_t i = 0; i < instruction_count; i++) {
    profile_instruction_count[i] = 0;
    profile_instruction_cycle_count[i] = 0;
  }

  instructions_issued = 0;
  instructions_stalled = 0;
  instructions_misc = 0;

  read_queue = 0;

  // tracking who atomincs
  for (size_t i = 0; i < thread_procs.size(); ++i) {
    // set to -1 to make the first thread in the block issue first
    atominc_bins[i] = 0;
  }

  // SIMD setup
  for (size_t i = 0; i < thread_procs.size() / simd_width; ++i) {
    // set to -1 to make the first thread in the block issue first
    simd_last_issued[i] = -1;
  }
  for (size_t i = 0; i < thread_procs.size(); ++i) {
    simd_state[i] = 0;
  }
  simd_stalls = 0;
  simd_issue = 0;
  simd_bonus_fetches = 0;

  // iCache setup
  for (int j = 0; j < num_icaches; ++j) {
    for (int i = 0; i < icache_banks; ++i) {
      bank_fetched[j][i] = 0;
    }
  }
  iCache_conflicts = 0;
  total_bank_cycles = 0;
  bank_cycles_used = 0;
}

IssueUnit::~IssueUnit() {
  if (verbosity > 1) {
    delete [] buf;
    fclose(all_trace);
    for (size_t i = 0; i < thread_procs.size(); i++) {
      fclose(thread_trace[i]);
    }
  }

  for(size_t i = 0; i < MAX_NUM_KERNELS; i++)
    {
      delete [] kernel_cycles[i];
      delete [] kernel_calls[i];
      delete [] kernel_profiling[i];
    }

  delete [] kernel_cycles;
  delete [] kernel_calls;
  delete [] kernel_profiling;
  delete [] thread_issue_count;
  delete [] atominc_bins;
  delete [] simd_last_issued;
  delete [] simd_state;
  for (int j = 0; j < num_icaches; ++j) {
    delete [] bank_fetched[j];
  }
  delete [] bank_fetched;
}

void IssueUnit::HaltSystem() {
  // need to do something else here.
  //longjmp(jump_location, 0);
  halted = true;
}

void IssueUnit::ClockRise() {

  // schedule next thread for multitrthreading
  for(size_t i=0; i<thread_procs.size(); i++){
    thread_procs[i]->Schedule(schedule_data[i].last_issued, schedule_data[i].last_stall);
    schedule_data[i].last_issued = Instruction::NOP;
    schedule_data[i].last_stall = Instruction::NOP;
  }
  // clear issued "bit" (for printing)
  int num_issued = thread_procs.size();
  for (size_t i = 0; i < thread_procs.size(); i++) {
    // decrement when not issued
    if (thread_procs[i]->GetActiveThread()->issued_this_cycle == NULL) num_issued--;
    else instructions_issued++;
    thread_procs[i]->GetActiveThread()->issued_this_cycle = NULL;
  }
  // increment the counter for the number of threads issued this cycle
  thread_issue_count[num_issued]++;

  // reset iCache reads for the cycle
  for (int j = 0; j < num_icaches; ++j) {
    for (int i = 0; i < icache_banks; ++i) {
      bank_fetched[j][i] = 0;
      total_bank_cycles++;
    }
  }
}

void IssueUnit::IssueVerbosity(ThreadState* thread, Instruction* fetched_instruction, size_t proc_id) {
  if (verbosity == 1) {     
    printf( "Cycle %lld: Thread %d: Instruction %llu (PC:%llu): ISSUED (%s)\n", current_cycle, 
	    static_cast<int>(proc_id), 
	    fetched_instruction->id,
	    thread->program_counter - 1,
	    Instruction::Opnames[fetched_instruction->op].c_str() );
  } else if (verbosity > 1) {
    sprintf( buf, "%s %d %d %d\n", Instruction::Opnames[fetched_instruction->op].c_str(),
	     fetched_instruction->args[0], fetched_instruction->args[1], fetched_instruction->args[2]);
    fprintf( thread_trace[proc_id], "%s", buf );
    fprintf( all_trace, "%s", buf );
    sprintf( buf, "Cycle %lld: Thread %d: Instruction %llu (PC:%llu): ISSUED (%s)\n", current_cycle, 
	     static_cast<int>(proc_id), 
	     fetched_instruction->id,
	     thread->program_counter,
	     Instruction::Opnames[fetched_instruction->op].c_str() );
    fprintf( thread_trace[proc_id], "%s", buf );
    fprintf( all_trace, "%s", buf );
  }
}

void IssueUnit::DataDependVerbosity(ThreadState* thread, Instruction* fetched_instruction, size_t proc_id) {
  if (verbosity == 1) {
    printf( "Cycle %lld: Thread %d: Instruction %llu: NR: DATA DEPENDENCY (%s)\n", current_cycle,
	    static_cast<int>(proc_id), 
	    fetched_instruction->id,
	    Instruction::Opnames[fetched_instruction->op].c_str() );
  } else if (verbosity > 1) {
    sprintf( buf, "Cycle %lld: Thread %d: Instruction %llu: NR: DATA DEPENDENCY (%s)\n", current_cycle,
	     static_cast<int>(proc_id), 
	     fetched_instruction->id,
	     Instruction::Opnames[fetched_instruction->op].c_str() );
    fprintf( thread_trace[proc_id], "%s", buf );
    fprintf( all_trace, "%s", buf );
  }
}

bool IssueUnit::Issue(ThreadProcessor* tp, ThreadState* thread, Instruction* fetched_instruction, size_t proc_id) {

  int fail_reg = -1;
  bool issued = false;
  if (fetched_instruction->ReadyToIssue(thread->register_ready, &fail_reg, current_cycle)) {
    // check functional units
    if (fetched_instruction->op == Instruction::NOP || fetched_instruction->op == Instruction::SYNC) {
      issued = true;
      // These are not being counted as issued for stats.
      // To count them add: thread->issued_this_cycle = fetched_instruction;
      instructions_misc++;
    } 
    else if (fetched_instruction->op == Instruction::SLEEP)
      {
	if(thread->end_sleep_cycle < 0)
	  {
	    reg_value arg;
	    Instruction::Opcode failop = Instruction::NOP;
	    if(thread->ReadRegister(fetched_instruction->args[0], current_cycle, arg, failop))
	      {
		thread->end_sleep_cycle = current_cycle + arg.udata;
		//printf("set end sleep to %d\n", thread->end_sleep_cycle);
	      }
	    //else
	      //printf("failed to read reg\n");
	  }
	if(thread->end_sleep_cycle >= 0)
	  if(current_cycle >= thread->end_sleep_cycle)
	    {
	      issued = true;
	      thread->end_sleep_cycle = -1;
	    }
      }
    else if (fetched_instruction->op == Instruction::PROF) {
#if 0
      if(fetched_instruction->args[0] == 2)
	{
	  if(!printed_single_kernel)
	    {
	      verbosity = 1;
	      printed_single_kernel = true;
	    }
	  else
	    verbosity = 0;
	}
#endif
      // kernel profiling
      int kernel_prof_id = fetched_instruction->args[0]; //kernel id
      // check for out of bounds error
      if (kernel_prof_id >= MAX_NUM_KERNELS) {
	printf("PROF kernel ID out of range.\n");
      } else {
	// toggle profiling for this kernel
	kernel_profiling[kernel_prof_id][tp->proc_id] = !kernel_profiling[kernel_prof_id][tp->proc_id];
	// count calls if turned on
	if (kernel_profiling[kernel_prof_id][tp->proc_id]) {
	  kernel_calls[kernel_prof_id][tp->proc_id]++;
	}
      }
      issued = true;
      // don't count as an issued instruction. Classify with NOP.
      //thread->issued_this_cycle = fetched_instruction;      
      instructions_misc++;
    }
    // TODO: Need to add these to the ISA
    else if (fetched_instruction->op == Instruction::SETTRIPIPE)
      {
	for (size_t i = 0; i < units.size(); i++) 
	  {
	    // Double triangle pipelines use 8 MULs, 4 ADDs, leaving 1 and 4 (if we assume 9 MULs, 8 ADDs)
	    FPMul* munit = dynamic_cast<FPMul*>(units[i]);
	    if(munit)
	      munit->width = 1;
	    FPAddSub* aunit = dynamic_cast<FPAddSub*>(units[i]);
	    if(aunit)
	      aunit->width = 4;	    
	  }
	issued = true;
      }
    else if (fetched_instruction->op == Instruction::SETBOXPIPE)
      {
	for (size_t i = 0; i < units.size(); i++) 
	  {
	    // Box pipeline use 6 MULs, 6 ADDs, leaving 3 and 2 (if we assume 9 MULs, 8 ADDs)
	    FPMul* munit = dynamic_cast<FPMul*>(units[i]);
	    if(munit)
	      munit->width = 3;
	    FPAddSub* aunit = dynamic_cast<FPAddSub*>(units[i]);
	    if(aunit)
	      aunit->width = 2;	    
	  }
	issued = true;
      }
 
    else if (fetched_instruction->op == Instruction::HALT) {
      
      
      // Only HaltSystem() after all threads reach a halt, otherwise return false
      for (size_t i = 0; i < thread_procs.size(); ++i) {
	ThreadState *curr_thread = thread_procs[i]->GetActiveThread();
	if (!curr_thread->fetched_instruction || curr_thread->fetched_instruction->op != Instruction::HALT) {
	  // stall halts until they all agree
	  halted_count++;
	  return false;
	}
      }
      // All threads fetched a halt
      //printf("cycle: %lld, proc_id: %d\n", current_cycle, (int)proc_id);
      //printf("ISSUER FOUND HALT.  HALTING...\n");
      HaltSystem();
    } else if (thread->registers->SupportsOp(fetched_instruction->op)) {
      // This is a register operation it'll definitely succeed
      if (thread->registers->AcceptInstruction(*fetched_instruction, this, thread)) {
	thread->instructions_in_flight++;
	issued = true;
	// Count this as issued
	thread->issued_this_cycle = fetched_instruction;
      }
      else {
	// Something else is blocking the register file
	// Should maybe count these for multiple issue mode
      }
    } else {
      // regular op, check the functional unit list
      for (size_t i = 0; i < units.size(); i++) {
	if (units[i]->SupportsOp(fetched_instruction->op)) {
	  if (units[i]->AcceptInstruction(*fetched_instruction,
					  this, thread)) {
	    // This is just counting atomic incs per thread
	    if (fetched_instruction->op == Instruction::ATOMIC_INC) {
	      atominc_bins[proc_id]++;
	    }

	    
#if 0
	    // Note(DK): this is for tree rotations. When one part of the chip finishes updating the BVH,
	    //           we need to fush the caches to force cache coherency. Otherwise it would be cheating.
	    //
	    // reset L1 caches when a barrier instruction completes
	    // TODO: this should be its own instruction
	    if(fetched_instruction->op == Instruction::BARRIER)
	      {
		for(size_t j = 0; j < units.size(); j++)
		  {
		    L1Cache* unit = dynamic_cast<L1Cache*>(units[j]);
		    if(unit)
		      unit->Clear();
		  }
	      }
#endif

	    thread->instructions_in_flight++;
	    issued = true;
	    break;
	  }
	}
      }
      if (issued) {
	thread->instructions_issued++;
	thread->issued_this_cycle = fetched_instruction;

	// something about multi-threading. Should be checked
	schedule_data[proc_id].last_issued = fetched_instruction->op;
      } else {
	// represents a unit issue conflict
	unit_contention[fetched_instruction->op]++;
      }
    }
    if (!issued) {
      if (fetched_instruction->op == Instruction::SLEEP)
	instructions_misc++;
      else
	fu_dependence++;
    }
  } else {
    data_dependence++;
    // find which instruction caused the stall (from old IssueUnit.cc)
    data_depend_bins[thread->GetFailOp(fail_reg)]++;
    schedule_data[proc_id].last_stall = thread->GetFailOp(fail_reg);
    DataDependVerbosity(thread, fetched_instruction, proc_id);
    not_ready++;
  }

  if (issued) {
    // stats and info
    simd_issue++;
    profile_instruction_count[thread->program_counter]++;
    instruction_bins[fetched_instruction->op]++;
    IssueVerbosity(thread, fetched_instruction, proc_id);
  }

  return issued;
}

void IssueUnit::MultipleIssueClockFall() {

  if(halted)
    return;

  // This just checks for long periods without successful issue
  static unsigned int current_issue_fails = 0;
  if (current_issue_fails > 500000 * thread_procs.size()) 
    {
      bool allHalted = true;
      printf("_-=<WARNING>=-_ 500000 cycles without a successful issue.\n");
      for (size_t thread_offset = 0;
	   thread_offset < thread_procs.size();
	   thread_offset += simd_width) {
	size_t proc_id = thread_offset;
	ThreadState* thread = thread_procs[proc_id]->GetActiveThread();
	Instruction* fetched_instruction = thread->fetched_instruction;
	if(fetched_instruction->op != Instruction::HALT)
	  allHalted = false;
	printf( "Cycle %lld: Thread %d: Instruction %llu (PC:%llu): current op (%s)\n", current_cycle, 
		static_cast<int>(proc_id), 
		fetched_instruction->id,
		thread->program_counter - 1,
		Instruction::Opnames[fetched_instruction->op].c_str() );
	printf("\tthread_id = %d, core_id = %d\n", thread->thread_id, thread->core_id);
	printf("\tregister_ready[%d][%d][%d] = %lld, %lld, %lld\n", fetched_instruction->args[0], fetched_instruction->args[1], fetched_instruction->args[2], thread->register_ready[fetched_instruction->args[0]], thread->register_ready[fetched_instruction->args[1]], thread->register_ready[fetched_instruction->args[2]]);
      }

      exit(1);

      
      if(allHalted)
	{
	  printf("halting\n");
	  HaltSystem();
	}
      else
	{
	  printf("Not halting\n");
	  verbosity = 1;
	}
      
    }

  // Begin standard issue code
  int issue_width = 1;

  // Choose a start thread
  int start_thread = 0;
  long long int max_time_not_issued = 0;
  for (size_t thread_id = 0; thread_id < thread_procs.size(); thread_id++) {
    ThreadState* thread = thread_procs[thread_id]->GetActiveThread();
    long long int time_not_issued = current_cycle - thread->last_issue;
    if (time_not_issued > max_time_not_issued) {
      start_thread = thread_id;
      max_time_not_issued = time_not_issued;
    }
  }

  // first choose a simd block
  for (size_t thread_offset = 0;
       thread_offset < thread_procs.size();
       thread_offset += simd_width) {
    if(thread_procs[thread_offset]->halted)
      continue;
    int num_issued = 0;
    bool issued = true;

    // change this when simd is added back in
    size_t proc_id = (thread_offset + start_thread) % thread_procs.size();
    ThreadState* thread = thread_procs[proc_id]->GetActiveThread();
    Instruction* fetched_instruction = thread->fetched_instruction;

    // count the cycles on this instruction
    profile_instruction_cycle_count[thread->program_counter]++;
    
    // check if an instruction was already fetched but failed to issue
    if (fetched_instruction != NULL) {
      // Issue
      issued = Issue(thread_procs[proc_id], thread, fetched_instruction, proc_id);
      if (issued) {
	thread->fetched_instruction = NULL;
	fetched_instruction = NULL;
	num_issued++;
	current_issue_fails = 0;
	thread->last_issue = current_cycle;
      } else {
	// stall
	instructions_stalled++;
	current_issue_fails++;
      }
    }

    while (issued && num_issued < issue_width) {
      // Fetch this block
      int icache_num = (proc_id/simd_width) % num_icaches;
      int bank_num = thread->program_counter % icache_banks;
      if (bank_fetched[icache_num][bank_num] < fetch_per_cycle) {
	bank_fetched[icache_num][bank_num]++;
	thread->fetched_instruction = thread->instructions[thread->program_counter];
	thread->program_counter = thread->next_program_counter;
	thread->next_program_counter++;
	fetched_instruction = thread->fetched_instruction;
	thread->fetched_instruction->id = thread->instruction_id++;
	// stats
	bank_cycles_used++;
	// Issue
	issued = Issue(thread_procs[proc_id], thread, fetched_instruction, proc_id);
	if (issued) {
	  thread->fetched_instruction = NULL;
	  fetched_instruction = NULL;
	  num_issued++;
	  current_issue_fails = 0;
	  thread->last_issue = current_cycle;
	} else {
	  // stall - record reason
	  instructions_stalled++;
	  current_issue_fails++;
	  break;
	}
      }
      else {
	// icache couldn't fetch
	iCache_conflicts++;
	instructions_stalled++;
	// unfilled_issue_slots += issued_width - num_issued;
	current_issue_fails++;
	break;
      }
    }
  }
}

void IssueUnit::SIMDClockFall() {
  // first choose a simd block
  for (size_t thread_offset = 0;
       thread_offset < thread_procs.size();
       thread_offset += simd_width) {
    int num_issued = 0;
    bool issued = true;

    // try to issue any threads that have been fetched first
    for (size_t i = 0; i < simd_width; ++i) {
      size_t proc_id = thread_offset + i;
      ThreadState* thread = thread_procs[proc_id]->GetActiveThread();
      Instruction* fetched_instruction = thread->fetched_instruction;
    
      // count the cycles on this instruction
      profile_instruction_cycle_count[thread->program_counter]++;
    
      // issue fetched instructions
      if (fetched_instruction != NULL) {
	// Issue
	issued = Issue(thread_procs[proc_id], thread, fetched_instruction, proc_id);
	if (issued) {
	  thread->fetched_instruction = NULL;
	  fetched_instruction = NULL;
	  num_issued++;
	  simd_state[proc_id] = 0;
	  thread->last_issue = current_cycle;
	  // save the last issued to fetch the next one first
	  simd_last_issued[thread_offset] = proc_id - thread_offset;
	} else {
	  // stall
	  instructions_stalled++;
	}
      }
    }
    
    // check if any threads in this block still need to issue
    bool any_fetched = false;
    for (size_t i = 0; i < simd_width; ++i) {
      size_t proc_id = thread_offset + i;
      if (simd_state[proc_id] > 0) {
	any_fetched = true;
	break;
      }
    }

    if (any_fetched) {
      // don't fetch this block. Waiting on threads to finish
      continue;
    }

    // start with the one after the last one to issue, but only when fetching
    size_t proc_id = (simd_last_issued[thread_offset] + 1) % simd_width + thread_offset;
    ThreadState* thread = thread_procs[proc_id]->GetActiveThread();

    // Fetch this block
    int icache_num = (proc_id/simd_width) % num_icaches;
    int bank_num = thread->program_counter % icache_banks;
    if (bank_fetched[icache_num][bank_num] < fetch_per_cycle) {
      bank_fetched[icache_num][bank_num]++;
      thread->fetched_instruction = thread->instructions[thread->program_counter];
      thread->program_counter = thread->next_program_counter;
      thread->next_program_counter++;
      thread->fetched_instruction->id = thread->instruction_id++;
      // stats
      bank_cycles_used++;
      // mark thread as fetched
      simd_state[proc_id] = 1;
      
      // fetch simd block
      for (size_t i = 1; i < simd_width; ++i) {
	size_t mod_val = (proc_id - thread_offset + i) % simd_width;
	size_t next_proc_id = mod_val + thread_offset;	
	ThreadState* next_thread = thread_procs[next_proc_id]->GetActiveThread();
	// check if this thread is on the same instruction
	if (thread->program_counter - 1 == next_thread->program_counter) {
	  next_thread->fetched_instruction = next_thread->instructions[next_thread->program_counter];
	  next_thread->program_counter = next_thread->next_program_counter;
	  next_thread->next_program_counter++;
	  next_thread->fetched_instruction->id = next_thread->instruction_id++;
	  // mark thread as fetched
	  simd_state[next_proc_id] = 1;
	}
      }
    }
    else {
      // icache couldn't fetch
      iCache_conflicts++;
      instructions_stalled++;
      // unfilled_issue_slots += issued_width - num_issued;
    }
  }
}

void IssueUnit::ClockFall() {
  // first allow threads to write to their registers
  for (size_t i = 0; i < thread_procs.size(); i++) {
    {
      // kernel profiling
      for (int j = 0; j < MAX_NUM_KERNELS; ++j) {
	if (kernel_profiling[j][i]) {
	  kernel_cycles[j][i]++;
	}
      }
      for(int j = 0; j < thread_procs[i]->num_threads; j++)
	thread_procs[i]->thread_states[j]->ApplyWrites(current_cycle);
    }
  }
  if (simd_width < 2) {
    MultipleIssueClockFall();
  } else {
    SIMDClockFall();
  }
  current_cycle++;
}

void IssueUnit::print()
{
  print(-1);
}

void IssueUnit::print(int total_system_TMs = -1) 
{ 
  printf("profile data:\n");
  printf("kernel\ttotal calls\ttotal cycles\n");
  

  // print machine-wide kernel stats
  for (int i = 0; i < MAX_NUM_KERNELS; ++i) 
    {
      long long int total_calls = 0;
      long long int total_cycles = 0;
      for(size_t j = 0; j < thread_procs.size(); j++)
	if(kernel_calls[i][j] > 0)
	  {
	    total_calls += kernel_calls[i][j];
	    total_cycles += kernel_cycles[i][j];
	  }
      if(total_calls == 0)
	continue;
      
      printf("%d\t%lld\t%lld\n", i, total_calls, total_cycles);
    }
  
  
  printf("Data dependence stalls (caused by):\n");
  for ( size_t i = 0; i < Instruction::NUM_OPS; i++) {
    if (data_depend_bins[i] != 0) {
      printf("   %-12s\t%lld\t(%.3f %%)\n", Instruction::Opnames[i].c_str(),
	     data_depend_bins[i],
	     static_cast<float>(data_depend_bins[i])/data_dependence*100);
    }
  }
  

  long long int total = 0;
  for ( size_t i = 0; i < Instruction::NUM_OPS; i++) {
    total += unit_contention[i];
  }
  
  printf("Number of thread-cycles resource conflicts found when issuing:\n");
  for ( size_t i = 0; i < Instruction::NUM_OPS; i++) {
    if (unit_contention[i] != 0) {
      printf("   %-12s\t%lld\t(%.3f %%)\n", Instruction::Opnames[i].c_str(), 
	     unit_contention[i],
	     static_cast<float>(unit_contention[i]) / total * 100);
    }
  }
    

  total = 0;
  for ( size_t i = 0; i < Instruction::NUM_OPS; i++) {
    total += instruction_bins[i];
  }
  
  printf("Dynamic Instruction Mix: (%lld total)\n", total);
  // switch to reporting everything
  for ( size_t i = 0; i < Instruction::NUM_OPS; i++) {
    if (instruction_bins[i] != 0) {
      printf("   %-12s\t%lld\t(%.3f %%)\n", Instruction::Opnames[i].c_str(),
	     instruction_bins[i],
	     static_cast<float>(instruction_bins[i]) / total * 100);
    }
  }
    

  if(issue_stats.avg_issue < 0.0)
    CalculateIssueStats();

  int total_system_threads = 0;
  float divisor = 0.f;
  if(total_system_TMs == -1)
    {
      total_system_threads = thread_procs.size();
      divisor = 1.f;
    }
  else
    {
      total_system_threads = thread_procs.size() * total_system_TMs;
      divisor = static_cast<float>(total_system_TMs);
    }

  printf("Issue statistics:\n");
  printf(" --Average #threads Issuing each cycle: %.4lf\n", issue_stats.avg_issue);
  printf(" --Issue Rate: %.2f\n", issue_stats.avg_issue / static_cast<float>(total_system_threads));
  printf(" --iCache conflicts: %lld (%f%%)\n", iCache_conflicts, issue_stats.avg_iCache_conflicts / divisor);
  printf(" --thread*cycles of FU dependence: %lld (%f%%)\n", fu_dependence, issue_stats.avg_fu_dependence / divisor);
  printf(" --thread*cycles of data dependence: %lld (%f%%)\n", data_dependence, issue_stats.avg_data_dependence / divisor);
  printf(" --thread*cycles halted: %lld (%f%%)\n", halted_count, issue_stats.avg_halted_count / divisor);
  printf(" --thread*cycles of issue NOP/other: %lld (%f%%)\n", instructions_misc, issue_stats.avg_misc_count / divisor);

}


// This function is for stats-tracking only.
// We will use one core to hold the sums of all other cores' stats
void IssueUnit::AddStats(IssueUnit* otherIssuer)
{
  if(issue_stats.avg_issue < 0.0)
    CalculateIssueStats();
  if(otherIssuer->issue_stats.avg_issue < 0.0)
    otherIssuer->CalculateIssueStats();  
  issue_stats.AddStats(otherIssuer->issue_stats);
  
  fu_dependence += otherIssuer->fu_dependence;
  data_dependence += otherIssuer->data_dependence;
  iCache_conflicts += otherIssuer->iCache_conflicts;
  halted_count += otherIssuer->halted_count;

  
  current_cycle = current_cycle < otherIssuer->current_cycle ? otherIssuer->current_cycle : current_cycle;

  // Per-instruction stats
  for(int i = 0; i < Instruction::NUM_OPS; i++)
    {
      instruction_bins[i] += otherIssuer->instruction_bins[i];
      unit_contention[i] += otherIssuer->unit_contention[i];
      data_depend_bins[i] += otherIssuer->data_depend_bins[i];
    }  

  // Profiling kernels
  for (int i = 0; i < MAX_NUM_KERNELS; ++i) 
    {
      for(size_t j = 0; j < thread_procs.size(); j++)           
	{
	  kernel_calls[i][j] += otherIssuer->kernel_calls[i][j];
	  kernel_cycles[i][j] += otherIssuer->kernel_cycles[i][j];
	}
    }

  // Issue statistics
  for (size_t i = 0; i < thread_procs.size(); i++) 
    for(size_t j = 0; j < thread_procs[i]->thread_states.size(); j++)
      thread_procs[i]->thread_states[j]->instructions_issued += otherIssuer->thread_procs[i]->thread_states[j]->instructions_issued;
  
  
  for(size_t i = 0; i < thread_procs.size() + 1; i++)
    thread_issue_count[i] += otherIssuer->thread_issue_count[i];

}

// Computes the per-TM issue rate statistics.
// These can't use chip-wide cycle count since it varies from TM to TM
void IssueUnit::CalculateIssueStats()
{
  issue_stats.avg_issue = 0.0;
  for (size_t i = 0; i < thread_procs.size()+1; ++i) 
    issue_stats.avg_issue += thread_issue_count[i]/static_cast<double>(current_cycle+1) * i;
  
  issue_stats.avg_iCache_conflicts = static_cast<float>(iCache_conflicts)/thread_procs.size()/current_cycle*100;
  issue_stats.avg_fu_dependence = static_cast<float>(fu_dependence)/thread_procs.size()/current_cycle*100;
  issue_stats.avg_data_dependence = static_cast<float>(data_dependence)/thread_procs.size()/current_cycle*100;
  issue_stats.avg_halted_count = static_cast<float>(halted_count)/thread_procs.size()/current_cycle*100;
  issue_stats.avg_misc_count = static_cast<float>(instructions_misc)/thread_procs.size()/current_cycle*100;

}
