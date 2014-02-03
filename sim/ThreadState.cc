#include "ThreadState.h"
#include "SimpleRegisterFile.h"
#include "Instruction.h"
#include "WriteRequest.h"
#include "L2Cache.h"
#include <assert.h>
#include <stdlib.h>

#define N 1000
#define MAX_STALL_CYCLES 16


WriteQueue::WriteQueue() {
  requests = new WriteRequest[N];
  head = 0;
  tail = 0;
}

WriteQueue::~WriteQueue() {
  delete[] requests;
}

void WriteQueue::clear()
{
  delete[] requests;
  requests = new WriteRequest[N];
  head = 0;
  tail = 0;
}

WriteRequest* WriteQueue::push(long long int cycle) {
  if(size() == N-1){
    printf("\n\nWrite queue overflow\n\n");
    print();
    exit(1);
  }
  WriteRequest* ret = &requests[head];
  ret->ready_cycle = cycle;
  head++;
  head = head % N;

  return ret;
}

WriteRequest* WriteQueue::front() {
  return &requests[tail];
}

void WriteQueue::pop() {
  tail++;
  tail = tail % N;
}

int WriteQueue::size() {
  int temp = head-tail;
  while(temp < 0) temp+=N;
  return temp;
}

bool WriteQueue::empty() {
  return head == tail;
}

bool WriteQueue::update(ThreadState* thread, int which_reg, long long int which_cycle, unsigned int val, long long new_cycle, unsigned int new_val, Instruction::Opcode new_op, Instruction* new_instr)
{
  int num = size();

  for(int i = 0; i < num; ++i) 
    {
      if (requests[(tail+i)%N].which_reg == which_reg &&
	  requests[(tail+i)%N].ready_cycle == which_cycle &&
	  requests[(tail+i)%N].udata == val)
	{

	  // Find the first available cycle on or after ready cycle
	  // Always allow unknown_latency to enqueue
	  if(new_cycle != which_cycle && new_cycle != UNKNOWN_LATENCY)
	    {
	      int stall_cycles = -1;
		while(CycleUsed(new_cycle + ++stall_cycles))
		  {}
	      
	      if(stall_cycles > MAX_STALL_CYCLES)
		{
		  //TODO: Take the error out and actually limit the queue length (cause a stall instead of exiting)
		  printf("error: %s stalled for > %d cycles (%d)\n", Instruction::Opnames[new_op].c_str(), MAX_STALL_CYCLES, stall_cycles);
		  exit(1);
		}
	      new_cycle += stall_cycles;
	    }
	  requests[(tail+i)%N].ready_cycle = new_cycle;
	  requests[(tail+i)%N].udata = new_val;
	  requests[(tail+i)%N].op = new_op;
	  // If this function was called by UpdateWriteCycle, then we don't change the instruction
	  if(new_instr != NULL)
	    requests[(tail+i)%N].instr = new_instr;
	  thread->register_ready[which_reg] = new_cycle;
	  return true;
	}
    }
  
  // If we can't find the write request to be updated, that means it was squashed by a more relevant one
  //printf("Warning: did not find write to be updated (squashed instruction)\n");
  return false;
  
  //printf("error: did not find write to be updated\n");
  //printf("looking for reg %d, cycle %lld, val %u, setting new cycle to %lld\n", which_reg, which_cycle, val, new_cycle);
  //print();
  //exit(1);
}

bool WriteQueue::CycleUsed(long long int cycle) {
  // do a linear search to see if the cycle is used
  int num = size();
  for(int i = 0; i < num; ++i) {
    //printf("tail = %d, tail + i mod N = %d\n", tail, (tail+i)%N);
    if (requests[(tail+i)%N].ready_cycle == cycle) return true;
  }
  return false;
}

Instruction::Opcode WriteQueue::GetOp(int which_reg) {
  // do a linear search to find the op for the register
  int num = size();
  Instruction::Opcode return_op = Instruction::NOP;
  long long int last_cycle = 0;
  for(int i = 0; i < num; ++i) {
    if (requests[(tail+i)%N].which_reg == which_reg) {
      if (last_cycle < requests[(tail+i)%N].ready_cycle) {
	last_cycle = requests[(tail+i)%N].ready_cycle;
	return_op = requests[(tail+i)%N].op;
      }
    }
  }
  // in case the reg isn't being written...
  return return_op;
}

Instruction* WriteQueue::GetInstruction(int which_reg) {
  // do a linear search to find the op for the register
  int num = size();
  Instruction* return_instr = NULL;
  long long int last_cycle = 0;
  for(int i = 0; i < num; ++i) {
    if (requests[(tail+i)%N].which_reg == which_reg) {
      if (last_cycle < requests[(tail+i)%N].ready_cycle) {
	last_cycle = requests[(tail+i)%N].ready_cycle;
	return_instr = requests[(tail+i)%N].instr;
      }
    }
  }
  // in case the reg isn't being written...
  return return_instr;
}

bool WriteQueue::ReadyBy(int which_reg, long long int which_cycle,
			 long long int &ready_cycle, reg_value &val, Instruction::Opcode &op) {
  // might be better to know how many writes to this reg are in flight to choose the last one.
  int num = size();
  for(int i = 1; i <= num; ++i) {
    if (requests[(head-i+N)%N].which_reg == which_reg) {
      // this magic number represents the number of pipe stages ahead you can read the value
      //printf("\tin ReadyBy, which_cycle = %lld, ready_cycle = %lld\n", which_cycle, requests[(head-i+N)%N].ready_cycle);
      if (requests[(head-i+N)%N].ready_cycle <= which_cycle) {
	val.idata = requests[(head-i+N)%N].idata;
	ready_cycle = requests[(head-i+N)%N].ready_cycle;
	return true;
      }
      else {
	op = requests[(head-i+N)%N].op;
	return false;
      }
    }
  }
  printf("Register reported being written but no write found in queue. which_reg = %d, which_cycle = %lld\n", which_reg, which_cycle);
  return true;
}

void WriteQueue::print() {
  int num = size();
  printf("----start-----\n");
  for(int i = 0; i < num; ++i) {
    requests[(tail+i)%N].print();
  }
  printf("-----end-----\n");
}



ThreadState::ThreadState(SimpleRegisterFile* regs,
                         std::vector<Instruction*>& _instructions,
			 unsigned int _thread_id, unsigned int coreid) :
  thread_id(_thread_id), core_id(coreid), registers(regs), instructions(_instructions) {
  carry_register = 0;
  halted = false;
  sleep_cycles = 0;
  last_issue = 0;
  program_counter = 0;
  next_program_counter = 1;
  instruction_id  = 0;
  instructions_in_flight = 0;
  instructions_issued = 0;
  end_sleep_cycle = -1;

  fetched_instruction = NULL;
  issued_this_cycle = NULL;

  register_ready = new long long int[regs->num_registers];
  writes_in_flight = new int[regs->num_registers];
  for (int i = 0; i < registers->num_registers; i++) {
    register_ready[i] = 0;
    writes_in_flight[i] = 0;
  }
}

void ThreadState::Reset()
{
  halted = false;
  last_issue = 0;
  program_counter = 0;
  next_program_counter = 1;
  instruction_id = 0;
  instructions_in_flight = 0;
  fetched_instruction = NULL;
  issued_this_cycle = NULL;
  end_sleep_cycle = -1;
  write_requests.clear();
  for (int i = 0; i < registers->num_registers; i++) {
    register_ready[i] = 0;
    writes_in_flight[i] = 0;
  }
}

ThreadState::~ThreadState(){
  delete registers;
}

bool ThreadState::QueueWrite(int which_reg, reg_value val, long long int which_cycle, Instruction::Opcode op, Instruction* instr) {
  // check to make sure we can write on the cycle
//   printf("Queuing write to reg: %d with val: %d on cycle: %lld from op: %s\n", which_reg, val.idata, which_cycle,
//   Instruction::Opnames[op].c_str());
  //  write_requests.print();

  int stall_cycles = -1;

  // If there is already a write in flight to this register, either 
  // -The compiler generated a useless instruction, squash the old one
  // -Or the old one hasn't cleared the write queue yet, but it's cycle has passed
  //  (hence the register has been read then written by a subsequent instruction(s))
  if(writes_in_flight[which_reg] > 0)
    {
      Instruction::Opcode temp;
      reg_value old_val;
      long long int old_ready;
      // Get the old enqueued result
      write_requests.ReadyBy(which_reg, UNKNOWN_LATENCY, old_ready, old_val, temp);
      // This should never fail
      if(!write_requests.update(this, which_reg, old_ready, old_val.udata, which_cycle, val.udata, op, instr))
	{
	  printf("Error: found old write request but could not update it(1)\n");
	  exit(1);
	}
      return true;
    }

  // Find the next available cycle on or after the ready cycle
  // Unknown latencies can always be enqueue because they will be udpated 
  // with a real latency later.
  if(which_cycle != UNKNOWN_LATENCY)
    while(write_requests.CycleUsed(which_cycle + ++stall_cycles))
      {}



  if(stall_cycles > MAX_STALL_CYCLES)
    {
      //TODO: Take the error out and actually limit the queue length (cause a stall instead of exiting)
      printf("error: %s stalled for > %d cycles (%d)\n", Instruction::Opnames[op].c_str(), MAX_STALL_CYCLES, stall_cycles);
      exit(1);
    }
  
  if(which_cycle != UNKNOWN_LATENCY)
    which_cycle += stall_cycles;

  //if (which_cycle == UNKNOWN_LATENCY || !write_requests.CycleUsed(which_cycle)) 
  //{
  WriteRequest* new_write = write_requests.push(which_cycle);
  new_write->which_reg = which_reg;
  new_write->idata = val.idata;
  new_write->op = op;
  new_write->instr = instr;
  writes_in_flight[which_reg]++;
  //printf("%d writes in flight.\n",writes_in_flight[which_reg]);
  register_ready[which_reg] = which_cycle;
  return true;
  //}
  //return false;
}

void ThreadState::UpdateWriteCycle(int which_reg, long long int which_cycle, unsigned int val, long long int new_cycle, Instruction::Opcode op)
{
  write_requests.update(this, which_reg, which_cycle, val, new_cycle, val, op, NULL);
}

Instruction::Opcode ThreadState::GetFailOp(int which_reg) {
  // find which op was last queued to write to this reg
  return write_requests.GetOp(which_reg);
}

Instruction* ThreadState::GetFailInstruction(int which_reg) {
  // find which instruction was last queued to write to this reg
  return write_requests.GetInstruction(which_reg);
}

bool ThreadState::ReadRegister(int which_reg, long long int which_cycle, reg_value& val, Instruction::Opcode &op) {
  //printf("which reg = %d\n", which_reg);
  //write_requests.print();
  //printf("writes_in_flight[33] = %d\n", writes_in_flight[33]);
  // attempts to read the given register by the given cycle, if it fails return the op it's stalled on
  // check if register is being written
  //printf("Reading reg %d, on cycle: %lld, %d writes in flight\n", which_reg, which_cycle, writes_in_flight[which_reg]);
  //printf("Write queue size: %d\n", write_requests.size());

  //printf("reading register %d", which_reg);
  //printf("\tin flight = %d\n", writes_in_flight[which_reg]);

  if (writes_in_flight[which_reg] > 0) {
    // check if register forwarding is possible
    long long int old_ready;
    bool retval = write_requests.ReadyBy(which_reg, which_cycle, old_ready, val, op);
    if (retval) {
      // report forwarding
      registers->ReadForwarded(which_reg, which_cycle);
    }
    //write_requests.print();
    //printf("\tReadyBy returning %d, %d\n", val.idata, retval);
    //printf("\tReadRegister returning %d\n", retval);
    return retval;
  }
  else {
    //if (which_reg == 35) printf("Reg 35 being read.");
    // not being written, read the value and return true
    //printf("2ReadRegister returning %d, true\n", registers->idata[which_reg]);
    val.idata = registers->idata[which_reg];
    //printf("\tReadRegister returning true\n");
    return true;
  }
}

void ThreadState::ApplyWrites(long long int cur_cycle) {

  WriteRequest* request = write_requests.front();
  //printf("Write queue size: %d\n", write_requests.size());
  while(!write_requests.empty() && request->IsReady(cur_cycle)) {
    registers->WriteInt(request->which_reg, request->idata, request->ready_cycle);
    writes_in_flight[request->which_reg]--;
    //printf("%d writes in flight.\n",writes_in_flight[request->which_reg]);
    write_requests.pop();
    request = write_requests.front();
  }
}

void ThreadState::CompleteInstruction(Instruction* ins) {
  //printf("Completed Instruction: ");
  //ins->print();
  //printf("\n");
  instructions_in_flight--;
}

void ThreadState::Sleep(int num_cycles){
  if(sleep_cycles==0)
    sleep_cycles = num_cycles;
}

bool ThreadState::Wake(bool force){
  if(force){
    sleep_cycles = 0;
  }
  if(sleep_cycles > 0){
    sleep_cycles--;
    return false;
  }
  return true;
}
