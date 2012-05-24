#include <fstream>

#include "L1Cache.h"
#include "L2Cache.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <cassert>
#include <pthread.h>
#include <cstdlib>

extern pthread_mutex_t atominc_mutex;

L1Cache::L1Cache(L2Cache* _L2, int _hit_latency,
		 int _cache_size, int _num_banks = 4, int _line_size = 2,
		 bool _memory_trace = false, bool _l1_off = false, bool _l1_read_copy = false) :
  hit_latency(_hit_latency),
  cache_size(_cache_size), num_banks(_num_banks), line_size(_line_size),
  L2(_L2)
{
  // get num_blocks from L2 (total memory)
  num_blocks = L2->num_blocks;
  data = L2->data;

  // Turn off if turned off
  unit_off = _l1_off;
  if (unit_off) {
    hit_latency = 0;
    line_size = 0;
  }
  read_copy = _l1_read_copy;

  // compute address masks
  offset_mask = (1 << line_size) - 1;
  // This wouldn't work if cache_size is not a power of 2
  index_mask = ((cache_size >> line_size) - 1) << line_size;
  tag_mask = 0xFFFFFFFF;
  tag_mask &= ~index_mask;
  tag_mask &= ~offset_mask;
  // compute how far to shift to get the index in the lower bits.
  index_shift = line_size;
  index_tag_mask = tag_mask | index_mask;

  // need something with the banks

  // set up the address tag storage indexed by index
  tags = new int[cache_size>>line_size];
  memset(tags, 0, sizeof(int)*cache_size>>line_size);
  valid = new bool[cache_size>>line_size];
  memset(valid, false, sizeof(bool)*cache_size>>line_size);
  issued_this_cycle = new int[num_banks];
  read_address = new int[num_banks];
  for (int i = 0; i < num_banks; ++i) {
    issued_this_cycle[i] = 0;
    read_address[i] = -1;
  }
  //issued_this_cycle = 0;

  // set nearby L1s to NULL
  L1_1 = NULL;
  L1_2 = NULL;
  L1_3 = NULL;

  // initialize statistics
  hits = 0;
  stores = 0;
  accesses = 0;
  misses = 0;
  nearby_hits = 0;
  bank_conflicts = 0;
  same_word_conflicts = 0;
}

L1Cache::~L1Cache() {
  delete[] tags;
  delete[] valid;
  delete[] issued_this_cycle;
}

void L1Cache::Clear()
{
  memset(valid, false, sizeof(bool)*cache_size>>line_size);
}

void L1Cache::Reset()
{
  Clear();
  hits = 0;
  stores = 0;
  accesses = 0;
  misses = 0;
  nearby_hits = 0;
  bank_conflicts = 0;
  same_word_conflicts = 0;
}

bool L1Cache::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::LOAD || op == Instruction::STORE || op == Instruction::ATOMIC_FPADD || op == Instruction::LOADL1)
    return true;
  return false;
}

bool L1Cache::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  //int bank_id = 0;
  int temp_latency = 0;
  Instruction::Opcode failop;

//   if (issued_this_cycle >= num_banks)
//     return false;

  // handle loads
  if (ins.op == Instruction::LOAD) {
    reg_value arg1;
    // Read the register
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("Error in L1Cache register read. Should have passed.\n");
    }
//     printf("Value in reg   = %d\n",thread->registers->idata[ins.args[1]]);
//     printf("Value returned = %d\n",arg1.idata);
    int address = arg1.idata + ins.args[2];
    int bank_id = address % num_banks;
    if (read_address[bank_id] == address) {
      same_word_conflicts++;
      if (read_copy) {
	// allow the load to happen for free!
	hits++;
	accesses++;
	// queue register write
	long long int write_cycle = hit_latency + issuer->current_cycle;
	reg_value result;
	result.udata = data[address].uvalue;
	if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	  // pipeline hazzard
	  return false;
	}
	UpdateCache(address);
	issued_this_cycle[bank_id]++;
	return true;
      }
    }
    if (issued_this_cycle[bank_id] && !unit_off) {
      bank_conflicts++;
      return false;
    }
    if (address < 0 || address >= num_blocks) {
      printf("ERROR: L1 MEMORY FAULT.  REQUEST FOR LOAD OF ADDRESS %d (not in [0, %d])\n",
             address, num_blocks);
      ins.print();
      printf("\n");
      exit(1);
      return true; // incorrect execution, but continues
    }
    // Block loads for memory that are being written
//     if (being_written[address]) {
//       //printf("Blocking load of address %d\n", address);
//       return false;
//     }
    // check for hit
    int index = (address & index_mask) >> index_shift;
    int tag = address & tag_mask;
    //    int index_tag = (address & index_tag_mask) >> index_shift;
    if (tags[index] != tag || !valid[index] || unit_off) {
      // cache miss, check nearby L1s (if snooping set)
      if ((L1_1 != NULL && L1_1->snoop(address)) ||
	  (L1_2 != NULL && L1_2->snoop(address)) || 
	  (L1_3 != NULL && L1_3->snoop(address))) {
	// hit here
	nearby_hits++;
	accesses++;
	int near_latency = 5;
	// queue register write
	long long int write_cycle = hit_latency + issuer->current_cycle + near_latency;
	reg_value result;
	result.udata = data[address].uvalue;
	if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	  // pipeline hazzard
	  return false;
	}
	UpdateCache(address);
	read_address[bank_id] = address;
	issued_this_cycle[bank_id]++;
	return true;
      } else if (L2->IssueInstruction(&ins, this, thread, temp_latency, issuer->current_cycle)) {
	misses++;
	accesses++;
	// queue write to register
	long long int write_cycle = hit_latency + temp_latency + issuer->current_cycle;
	reg_value result;
	result.udata = data[address].uvalue;
	if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	  // pipeline hazzard
	  return false;
	}
	UpdateCache(address);
	read_address[bank_id] = address;
	issued_this_cycle[bank_id]++;
	return true;
      } else {
	// L2 stalled
	return false;
      }
    } else { // cache hit
      hits++;
      accesses++;
      // queue register write
      long long int write_cycle = hit_latency + issuer->current_cycle;
      reg_value result;
      result.udata = data[address].uvalue;
      if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	// pipeline hazzard
	return false;
      }
      UpdateCache(address);
      read_address[bank_id] = address;
      issued_this_cycle[bank_id]++;
      return true;
    }
  } /// end loads

  // load from L1 if it's there otherwise set MAX_INT
  if (ins.op == Instruction::LOADL1) {
    // Read the register
    reg_value arg1;
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("Error in L1Cache LOADL1. Should have passed.\n");
    }    
    int address = arg1.idata + ins.args[2];
    int bank_id = address % num_banks;
    if (address < 0 || address >= num_blocks) {
      printf("ERROR: L1 MEMORY FAULT.  REQUEST FOR LOAD OF ADDRESS %d (not in [0, %d])\n",
             address, num_blocks);
      exit(1);
      return true; // incorrect execution, but continues
    }
    if (issued_this_cycle[bank_id] && !unit_off) {
      bank_conflicts++;
      return false;
    }
    // check for hit
    int index = (address & index_mask) >> index_shift;
    int tag = address & tag_mask;
    //    int index_tag = (address & index_tag_mask) >> index_shift;
    if (tags[index] != tag || unit_off) {
      // cache miss, set miss register
      accesses++;
      // queue register write
      long long int write_cycle = hit_latency + issuer->current_cycle;
      reg_value result;
      // Fail value
      result.idata = -1;
      if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	// pipeline hazzard
	return false;
      }
      UpdateCache(address);
      read_address[bank_id] = address;
      issued_this_cycle[bank_id]++;
      return true;
    } else {
      hits++;
      accesses++;
      // queue register write
      long long int write_cycle = hit_latency + issuer->current_cycle;
      reg_value result;
      result.udata = data[address].uvalue;
      if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	// pipeline hazzard
	return false;
      }
      UpdateCache(address);
      read_address[bank_id] = address;
      issued_this_cycle[bank_id]++;
      return true;
    }
  } /// end loadl1

  // atomic fp add
  if (ins.op == Instruction::ATOMIC_FPADD) {
    // Read the register
    reg_value arg0, arg1;
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop) ||
	!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("Error in L1Cache ATOMIC_FPADD. Should have passed.\n");
    }    
    int address = arg0.idata + ins.args[2];
    int bank_id = address % num_banks;
    if (issued_this_cycle[bank_id] && !unit_off) {
      bank_conflicts++;
      return false;
    }
    int index = (address & index_mask) >> index_shift;
    int tag = address & tag_mask;
    // Check cache on write
    if (tags[index] == tag) {
      // set cache as dirty
      tags[index] = 0xFFFFFFFF;
    }
    if (L2->IssueInstruction(&ins, this, thread, temp_latency, issuer->current_cycle)) {
      // complete in temp_latency cycles
      //      misses++;
      //      outstanding_requests++;
      pthread_mutex_lock(&atominc_mutex);
      //execute atomic add here
      data[address].fvalue += arg1.fdata;
      pthread_mutex_unlock(&atominc_mutex);
      UpdateCache(address);
      //read_address[bank_id] = address; // can't be replicated
      issued_this_cycle[bank_id]++;
      return true;
    } else {
      // L2 stalled
      return false;
    }
  } /// end atominc

  // writes
  if (ins.op == Instruction::STORE) {
    // Read the register
    reg_value arg0;
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop)) {
      // bad stuff happened
      printf("Error in L1Cache STORE. Should have passed.\n");
    }    
    int address = arg0.idata + ins.args[2];
    int bank_id = address % num_banks;
    if (issued_this_cycle[bank_id] && !unit_off) {
      bank_conflicts++;
      return false;
    }
    int index = (address & index_mask) >> index_shift;
    int tag = address & tag_mask;
    // Check cache on write
    if (tags[index] == tag) {
      // set cache as dirty
      //tags[index] = 0xFFFFFFFF;
      // set as cached
      tags[index] = tag;
    }
    if (L2->IssueInstruction(&ins, this, thread, temp_latency, issuer->current_cycle)) {
      stores++;
      misses++;
      accesses++;
      // doesn't write a register, goes around cache
      thread->CompleteInstruction(&ins);
      issued_this_cycle[bank_id]++;
      return true;
    } else {
      return false;
    }// end store
  } else {
    return false;
  }
}

// From HardwareModule
void L1Cache::ClockRise() {
  // nothing to do
  processed_this_cycle = 0;
  for (int i = 0; i < num_banks; ++i) {
    issued_this_cycle[i] = 0;
    read_address[i] = -1;
  }
  //  issued_this_cycle = 0;
  //  issued_atominc = false;
}

void L1Cache::ClockFall() {
  // all the work used to be here... it should be done in higher levels now
  // this->UpdateCache(address); etc.
}

void L1Cache::print() {
  printf("L1Cache\n");
//   printf("%d instructions in-flight",
//          instructions->Size());
}

void L1Cache::PrintStats() {
  if (unit_off) {
    printf("L1 OFF!\n");
  }
  printf("L1 accesses: \t%lld\n", accesses);
  printf("L1 hits: \t%lld\n", hits);
  printf("L1 misses: \t%lld\n", misses);
  printf("L1 bank conflicts: \t%lld\n", bank_conflicts);
  printf("L1 stores: \t%lld\n", stores);
  printf("L1 near hit: \t%lld\n", nearby_hits);  
  printf("L1 hit rate: \t%f\n", static_cast<float>(hits)/accesses);
//   printf("  Bandwidth:\t%.4f GB/Sec\n", static_cast<float>(misses)*4.f/current_cycle * 
// 	 (1<<line_size) * .5);
//   printf("  Max Bandwidth:%.4f GB/Sec (Theoretical)\n", static_cast<float>(num_banks)*4.f *
// 	 (1<<line_size) * .5);

}

double L1Cache::Utilization() {
  return static_cast<double>(processed_this_cycle) / num_banks;
}

// This is for nearby L1s to snoop
bool L1Cache::snoop(int address) {
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;
  return tags[index] == tag;
}

// This updates the tag to reflect the address given
bool L1Cache::UpdateCache(int address) {
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;
  tags[index] = tag;
  valid[index] = true;
  return true;
}
