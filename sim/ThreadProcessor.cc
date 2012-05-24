#include "ThreadProcessor.h"

ThreadProcessor::ThreadProcessor(int _num_threads, int num_regs, int _proc_id, SchedulingScheme ss, std::vector<Instruction*>* _instructions, std::vector<HardwareModule*> &modules, std::vector<FunctionalUnit*> *_functional_units, size_t threadprocid, size_t coreid, size_t l2id)
{
  num_threads = _num_threads;
  active_thread = 0;
  proc_id = _proc_id;
  schedule = ss;
  functional_units = _functional_units;
  halted = false;
  num_halted = 0;

  // set up multiple thread states for multi-threading (default is 1 thread per proc)
  for(int i=0; i<num_threads; i++){
    int thread_id = proc_id * num_threads + i;
    SimpleRegisterFile *simple_regs = new SimpleRegisterFile(num_regs, thread_id);
    // load IDs in to reserved registers
    simple_regs->udata[1] = threadprocid * num_threads + i;
    simple_regs->udata[2] = coreid;
    simple_regs->udata[3] = l2id;
    
    //TODO: threadprocid should be varied for multi-threading
    thread_states.push_back(new ThreadState(simple_regs, *_instructions, threadprocid, coreid));
    modules.push_back(simple_regs);
  }
}

ThreadProcessor::~ThreadProcessor(){
  for(size_t i=0; i<thread_states.size(); i++){
    delete thread_states[i];
  }
}

void ThreadProcessor::Reset()
{
  halted = false;
  num_halted = 0;
  for(int i=0; i < num_threads; i++)
    thread_states[i]->Reset();
}

bool ThreadProcessor::ReadRegister(int which_reg, long long int which_cycle, reg_value &val, Instruction::Opcode &op){
  return thread_states.at(active_thread)->ReadRegister(which_reg, which_cycle, val, op);
}

void ThreadProcessor::ApplyWrites(long long int cur_cycle){
  thread_states.at(active_thread)->ApplyWrites(cur_cycle);
}

void ThreadProcessor::CompleteInstruction(Instruction* ins){
  thread_states.at(active_thread)->CompleteInstruction(ins);
}


//TODO: Multithreading does not work with the new HALT system. The entire ThreadProcessor (all multi-threads)
// will halt as soon as one of them halts.
void ThreadProcessor::Schedule(Instruction::Opcode last_issued, Instruction::Opcode last_stall){
  // Removed to allow multiple issue. Should be fixed if we want multithreading at the same time
//   if(last_issued!=Instruction::NOP && last_stall!=Instruction::NOP)
//     {
//       printf("error, stall and issue at the same time\n");
//       exit(1);
//     }
  if(last_issued!=Instruction::NOP){
    GetActiveThread()->Wake(true); 
  }
  SwitchThread(last_issued, last_stall);
}

void ThreadProcessor::SwitchThread(Instruction::Opcode last_issued, Instruction::Opcode last_stall){
  size_t i;
  int stall_length = 0;  
  switch(schedule){
  case SIMPLE:
    SimpleScheduler();
    break;
  case PRESTALL:
    if(last_issued!=Instruction::NOP){
      for(i=0; i<functional_units->size(); i++){
	if(functional_units->at(i)->SupportsOp(last_issued)){
	  stall_length = functional_units->at(i)->GetLatency() + 1;
	}
      }
    }
    if(last_stall==Instruction::LOAD)
      stall_length = 51;
    if(stall_length > 1)
      GetActiveThread()->Sleep(stall_length);
    PrestallScheduler();
    break;
  case POSTSTALL:
    if(last_stall!=Instruction::NOP){
      for(i=0; i<functional_units->size(); i++){
	if(functional_units->at(i)->SupportsOp(last_stall)){
	  stall_length = functional_units->at(i)->GetLatency() + 1;
	  break;
	}
      }
    }
    if(last_stall==Instruction::LOAD)
      stall_length = 51;
    /*if(last_stall!=Instruction::LOAD &&
      last_stall!=Instruction::FPINVSQRT &&
      last_stall!=Instruction::FPINV &&
      last_stall!=Instruction::FPMUL &&
      last_stall!=Instruction::MUL)
      stall_length = 0;*/
    
    if(stall_length > 1)
      {
	GetActiveThread()->Sleep(stall_length);	
      }
    PoststallScheduler();
    break;
  default:
    SimpleScheduler();
    break;
  };
}

void ThreadProcessor::SimpleScheduler(){
  // simple round robin scheduling
  active_thread++;
  active_thread = active_thread % num_threads;
}

void ThreadProcessor::PrestallScheduler(){
  int last_awake = active_thread;
  // try to wake up the current thread if it's asleep
  if(!GetActiveThread()->Wake())
    last_awake = -1;
  // then try to wake the rest of them and find one that succeeds
  bool has_switched = false;
  for(int i=(active_thread+1) % num_threads, j=0; j<num_threads-1; i = (i+1)%num_threads, j++){
    if(thread_states[i]->Wake() && !has_switched){
      has_switched = true;
      active_thread = i;
    }
  }
  // same as poststall scheduling, except we don't want to switch if the current thread is still running
  if(last_awake!=-1)
    active_thread = last_awake;
}

void ThreadProcessor::PoststallScheduler(){
  // try to wake up the current thread if it's asleep
  GetActiveThread()->Wake();
  // then try to wake the rest of them and find one that succeeds
  bool has_switched = false;
  for(int i=(active_thread+1) % num_threads, j=0; j<num_threads-1; i = (i+1)%num_threads, j++){
    if(thread_states[i]->Wake() && !has_switched){
      has_switched = true;
      active_thread = i;
    }
  }
}

ThreadState* ThreadProcessor::GetActiveThread(){
  return thread_states.at(active_thread);
}

void ThreadProcessor::EnableRegisterDump(){
  char filename[512];
  for(size_t i=0; i<thread_states.size(); i++){
    sprintf(filename, "thread%d_trace.txt", proc_id * num_threads + (int)i);
    thread_states[i]->registers->EnableDump(filename);
  }
}



