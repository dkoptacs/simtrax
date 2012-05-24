#include "ThreadState.h"
#include "SimpleRegisterFile.h"
#include "Instruction.h"
#include "WriteRequest.h"
#include <assert.h>


#define N 200

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

bool WriteQueue::ReadyBy(int which_reg, long long int which_cycle,
			 reg_value &val, Instruction::Opcode &op) {
  // might be better to know how many writes to this reg are in flight to choose the last one.
  int num = size();
  for(int i = 1; i <= num; ++i) {
    if (requests[(head-i+N)%N].which_reg == which_reg) {
      // this magic number represents the number of pipe stages ahead you can read the value
      if (requests[(head-i+N)%N].ready_cycle <= which_cycle) {
	val.idata = requests[(head-i+N)%N].idata;
	//printf("readyby returning %d from %d. head = %d, tail = %d\n", val.idata, (head-i+N)%N, head, tail);
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
  program_counter = 0;
  next_program_counter = 1;
  instruction_id  = 0;
  instructions_in_flight = 0;
  instructions_issued = 0;

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
  program_counter = 0;
  next_program_counter = 1;
  instruction_id = 0;
  instructions_in_flight = 0;
  fetched_instruction = NULL;
  issued_this_cycle = NULL;
  write_requests.clear();
  for (int i = 0; i < registers->num_registers; i++) {
    register_ready[i] = 0;
    writes_in_flight[i] = 0;
  }

}

ThreadState::~ThreadState(){
  delete registers;
}

bool ThreadState::QueueWrite(int which_reg, reg_value val, long long int which_cycle, Instruction::Opcode op) {
  // check to make sure we can write on the cycle
//   printf("Queuing write to reg: %d with val: %d on cycle: %lld from op: %s\n", which_reg, val.idata, which_cycle,
//   Instruction::Opnames[op].c_str());
  //  write_requests.print();
  if (!write_requests.CycleUsed(which_cycle)) {
    WriteRequest* new_write = write_requests.push(which_cycle);
    new_write->which_reg = which_reg;
    new_write->idata = val.idata;
    new_write->op = op;
    writes_in_flight[which_reg]++;
    //printf("%d writes in flight.\n",writes_in_flight[which_reg]);
    register_ready[which_reg] = which_cycle;
    return true;
  }
  return false;
}

Instruction::Opcode ThreadState::GetFailOp(int which_reg) {
  // find which op was last queued to write to this reg
  return write_requests.GetOp(which_reg);
}

bool ThreadState::ReadRegister(int which_reg, long long int which_cycle, reg_value& val, Instruction::Opcode &op) {
  //printf("which reg = %d\n", which_reg);
  //write_requests.print();
  //printf("writes_in_flight[33] = %d\n", writes_in_flight[33]);
  // attempts to read the given register by the given cycle, if it fails return the op it's stalled on
  // check if register is being written
  //printf("Reading reg %d, on cycle: %lld, %d writes in flight\n", which_reg, which_cycle, writes_in_flight[which_reg]);
  //printf("Write queue size: %d\n", write_requests.size());
  if (writes_in_flight[which_reg] > 0) {
    // check if register forwarding is possible
    
    bool retval = write_requests.ReadyBy(which_reg, which_cycle, val, op);
    if (retval) {
      // report forwarding
      registers->ReadForwarded(which_reg, which_cycle);
    }
    //write_requests.print();
    //printf("1ReadRegister returning %d, %d\n", val.idata, retval);
    return retval;
  }
  else {
    //if (which_reg == 35) printf("Reg 35 being read.");
    // not being written, read the value and return true
    //printf("2ReadRegister returning %d, true\n", registers->idata[which_reg]);
    val.idata = registers->idata[which_reg];
    return true;
  }
}

void ThreadState::ApplyWrites(long long int cur_cycle) {
  WriteRequest* request = write_requests.front();
  //printf("Write queue size: %d\n", write_requests.size());
  while(!write_requests.empty() && request->IsReady(cur_cycle)) {
    //printf("Writing register: %d with value: %d\n",request->which_reg, request->idata);
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
