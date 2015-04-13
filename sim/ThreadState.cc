#include "ThreadState.h"
#include "SimpleRegisterFile.h"
#include "Instruction.h"
#include "WriteRequest.h"
#include "L2Cache.h"
#include <assert.h>
#include <stdlib.h>

#define N 1000


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
  return false;
}

bool WriteQueue::updateMSA(ThreadState* thread, int which_reg, long long int which_cycle, reg_value val, long long new_cycle, reg_value new_val, Instruction::Opcode new_op, Instruction* new_instr)
{
  int num = size();

  for(int i = 0; i < num; ++i) 
    {
      if (requests[(tail+i)%N].which_reg == which_reg &&
	  requests[(tail+i)%N].ready_cycle == which_cycle &&
	  requests[(tail+i)%N].udata == val.udata)
	{

	  requests[(tail+i)%N].ready_cycle = new_cycle;
	  requests[(tail+i)%N].udata = new_val.udata;
	  requests[(tail+i)%N].udataMSA[0] = new_val.udataMSA[0];
	  requests[(tail+i)%N].udataMSA[1] = new_val.udataMSA[1];
	  requests[(tail+i)%N].udataMSA[2] = new_val.udataMSA[2];
	  requests[(tail+i)%N].op = new_op;
	  // If this function was called by UpdateWriteCycle, then we don't change the instruction
	  if(new_instr != NULL)
	    requests[(tail+i)%N].instr = new_instr;
	  thread->register_ready[which_reg] = new_cycle;
	  return true;
	}
    }
  
  // If we can't find the write request to be updated, that means it was squashed by a more relevant one
  return false;
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
  compare_register = 0;
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
  runtime = NULL;

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

bool ThreadState::QueueWrite(int which_reg, reg_value val, long long int which_cycle, Instruction::Opcode op, Instruction* instr, bool isMSA) {
  // check to make sure we can write on the cycle

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
      if((!isMSA && !write_requests.update(this, which_reg, old_ready, old_val.udata, which_cycle, val.udata, op, instr)) ||
	 (isMSA && !write_requests.updateMSA(this, which_reg, old_ready, old_val, which_cycle, val, op, instr)))
	{
	  printf("Error: found old write request but could not update it(1)\n");
	  exit(1);
	}
      return true;
    }

  WriteRequest* new_write = write_requests.push(which_cycle);
  new_write->which_reg = which_reg;
  new_write->idata = val.idata;
  new_write->op = op;
  new_write->instr = instr;
  if(isMSA)
    {
      new_write->isMSA = true;
      new_write->idataMSA[0] = val.idataMSA[0];
      new_write->idataMSA[1] = val.idataMSA[1];
      new_write->idataMSA[2] = val.idataMSA[2];
    }
  writes_in_flight[which_reg]++;
  register_ready[which_reg] = which_cycle;
  return true;
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

// Check if a register can be read, and return its value.
bool ThreadState::ReadRegister(int which_reg, long long int which_cycle, reg_value& val, Instruction::Opcode &op, bool isMSA) {


  if (writes_in_flight[which_reg] > 0) {
    // check if register forwarding is possible
    long long int old_ready;
    bool retval = write_requests.ReadyBy(which_reg, which_cycle, old_ready, val, op);
    if (retval) {
      // report forwarding
      registers->ReadForwarded(which_reg, which_cycle);
    }

    return retval;
  }
  else {
    // not being written, read the value and return true
    val.idata = registers->idata[which_reg];
    if(isMSA)
      {
	// 128-bit (4x32) version of ReadRegister. Since the 128-bit registers are mapped to the 32-bit registers, we use the same 
	// control logic for the 32-bit register to determine if it's ready to be read, then simply read the other three 32-bit words.
	// 128-bit registers are never read two different 32-words at a time
	val.idataMSA[0] = registers->idata[which_reg + (registers->num_registers * 1)];
	val.idataMSA[1] = registers->idata[which_reg + (registers->num_registers * 2)];
	val.idataMSA[2] = registers->idata[which_reg + (registers->num_registers * 3)];
      }
    return true;
  }
}

void ThreadState::ApplyWrites(long long int cur_cycle) {

  WriteRequest* request = write_requests.front();
  while(!write_requests.empty() && request->IsReady(cur_cycle)) {
    registers->WriteInt(request->which_reg, request->idata, request->ready_cycle);
    if(request->isMSA)
      {
	registers->WriteIntMSA(request->which_reg + (registers->num_registers * 1), request->idataMSA[0], request->ready_cycle);
	registers->WriteIntMSA(request->which_reg + (registers->num_registers * 2), request->idataMSA[1], request->ready_cycle);
	registers->WriteIntMSA(request->which_reg + (registers->num_registers * 3), request->idataMSA[2], request->ready_cycle);
      }
    writes_in_flight[request->which_reg]--;
    write_requests.pop();
    request = write_requests.front();
  }
}

void ThreadState::CompleteInstruction(Instruction* ins) {
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
