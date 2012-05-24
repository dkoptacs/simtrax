#include <fstream>

#include "L2Cache.h"
#include "L1Cache.h"
#include "MainMemory.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <cassert>

L2Cache::L2Cache(MainMemory* _mem, int _cache_size, int _hit_latency,
		 int _num_banks = 4, int _line_size = 2,
		 bool _memory_trace = false, bool _l2_off = false, bool l1_off = false) :
  cache_size(_cache_size), num_banks(_num_banks), line_size(_line_size),
  mem(_mem)
{
  // Turn off if turned off
  unit_off = _l2_off;
  if (unit_off) {
    hit_latency = 0;
    if (l1_off) {
      // can only turn line size to 0 if l1 is also off for bandwidth reasons
      line_size = 0;
    }
  }

  outstanding_data = 0;
  // 8w * 1000MHz * 4B/w = 32 GB/s
  // max_data_per_cycle = 1; // number of words allowed per cycle for max 4GB/s BW (1000Mhz)
  // max_data_per_cycle = 8; // number of words allowed per cycle for max 32GB/s BW (1000Mhz)
  max_data_per_cycle = 64; // number of words allowed per cycle for max 256GB/s BW (1000Mhz)
  max_outstanding_data = 256;
  line_fill_size = 1 << line_size;
  hit_latency = _hit_latency;
  // get num_blocks from MainMemory
  num_blocks = mem->num_blocks;
  data = mem->data;

  // create pthread mutex for this L2
  pthread_mutex_init(&cache_mutex, NULL);

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
  last_issued = new long long int[num_banks];
  memset(last_issued, -1, sizeof(long long int)*num_banks);

  // set up the address tag storage indexed by index
  tags = new int[cache_size>>line_size];
  memset(tags, 0, sizeof(int)*cache_size>>line_size);
  valid = new bool[cache_size>>line_size];
  memset(valid, false, sizeof(bool)*cache_size>>line_size);
  //  issued_this_cycle = new int[num_banks];
  issued_this_cycle = 0;

  // initialize statistics
  bandwidth_stalls = 0;
  hits = 0;
  stores = 0;
  accesses = 0;
  misses = 0;
  bank_conflicts = 0;
  memory_faults = 0;
}

void L2Cache::Clear()
{
  memset(valid, false, sizeof(bool)*cache_size>>line_size);
  memset(last_issued, -1, sizeof(long long int)*num_banks);
}

void L2Cache::Reset()
{
  Clear();
  bandwidth_stalls = 0;
  hits = 0;
  stores = 0;
  accesses = 0;
  misses = 0;
  bank_conflicts = 0;
  memory_faults = 0;
  outstanding_data = 0;
}

L2Cache::~L2Cache() {
  pthread_mutex_destroy(&cache_mutex);
  delete [] last_issued;
  delete [] tags;
  delete [] valid;
}

bool L2Cache::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::LOAD || op == Instruction::STORE || op == Instruction::ATOMIC_FPADD)
    return true;
  return false;
}

bool L2Cache::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  // doesn't accept any instructins on it's own
  return false;

}

// From HardwareModule
void L2Cache::ClockRise() {
  outstanding_data -= max_data_per_cycle;
  if(outstanding_data < 0)
    outstanding_data = 0;
}

void L2Cache::ClockFall() {
  // all the work used to be here... it should be done in higher levels now
  // self->UpdateCache(address); etc.
}

bool L2Cache::IssueInstruction(Instruction* ins, L1Cache * L1, ThreadState* thread, int& ret_latency, long long int current_cycle) {
  //  int bank_id = 0;
//   if (issued_this_cycle >= num_banks)
//     return false;

  // handle loads
  if (ins->op == Instruction::LOAD) {
    int address = thread->registers->ReadInt(ins->args[1], current_cycle) + ins->args[2];
    int bank_id = address % num_banks;
    if (address < 0 || address >= num_blocks) {
      //printf("ERROR: MEMORY FAULT.  REQUEST FOR LOAD OF ADDRESS %d (not in [0, %d])\n",
      //  address, num_blocks);
      pthread_mutex_lock(&cache_mutex);
      memory_faults++;
      pthread_mutex_unlock(&cache_mutex);
      return true; // just so we complete... incorrect execution
    }
    // check for bank conflicts
    if (last_issued[bank_id] == current_cycle && !unit_off) {
      pthread_mutex_lock(&cache_mutex);
      bank_conflicts++;
      pthread_mutex_unlock(&cache_mutex);
      return false;
    }
    // check for a hit
    int index = (address & index_mask) >> index_shift;
    int tag = address & tag_mask;
    if (tags[index] != tag || !valid[index] || unit_off) {
      // miss (add main memory latency)
      pthread_mutex_lock(&cache_mutex);
      if(outstanding_data >= max_outstanding_data)
	{
	  
	  bandwidth_stalls++;
	  pthread_mutex_unlock(&cache_mutex);
	  return false;
	}
      outstanding_data += line_fill_size;
      pthread_mutex_unlock(&cache_mutex);
      ret_latency = hit_latency + mem->GetLatency(ins);
      UpdateCache(address);
      pthread_mutex_lock(&cache_mutex);
      misses++;
      pthread_mutex_unlock(&cache_mutex);
    } else {
      // hit (queue load)
      ret_latency = hit_latency;
      pthread_mutex_lock(&cache_mutex);
      hits++;
      pthread_mutex_unlock(&cache_mutex);
    }
    pthread_mutex_lock(&cache_mutex);
    last_issued[bank_id] = current_cycle;
    accesses++;
    pthread_mutex_unlock(&cache_mutex);
    return true;
  }

  // atomic fp add
  if (ins->op == Instruction::ATOMIC_FPADD) {
    int address = thread->registers->ReadInt(ins->args[0], current_cycle) + ins->args[2];
    int bank_id = address % num_banks;
    // check for bank conflicts
    if (last_issued[bank_id] == current_cycle && !unit_off) {
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
    last_issued[bank_id] = current_cycle;
    // add memory latency
    ret_latency = hit_latency + mem->GetLatency(ins);
    return true;
  }// end atominc


  // writes
  // should probably check for max_concurrent (MSHR)
  int address = thread->registers->ReadInt(ins->args[0], current_cycle) + ins->args[2];
  if (address < 0 || address >= num_blocks) {
    printf("ERROR: L2 MEMORY FAULT.  REQUEST FOR STORE TO ADDRESS %d (not in [0, %d])\n\t",
	   address, num_blocks);
    ins->print();
    printf("\n");
  }
  int bank_id = address % num_banks;
  // check for bank conflicts
  if (last_issued[bank_id] == current_cycle && !unit_off) {
    pthread_mutex_lock(&cache_mutex);
    bank_conflicts++;
    pthread_mutex_unlock(&cache_mutex);
    return false;
  }
  pthread_mutex_lock(&cache_mutex);
  if(outstanding_data >= max_outstanding_data)
    {
      
      bandwidth_stalls++;
      pthread_mutex_unlock(&cache_mutex);
      return false;
    }
  outstanding_data += line_fill_size;
  pthread_mutex_unlock(&cache_mutex);
  // check for line in cache
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;
  if (tags[index] == tag) {
    // set as dirty
    //tags[index] = 0xFFFFFFFF;

    // cache it
    UpdateCache(address);
    //tags[index] = tag;
  }
  pthread_mutex_lock(&cache_mutex);
  last_issued[bank_id] = current_cycle;
  stores++;
  misses++;
  accesses++;
  pthread_mutex_unlock(&cache_mutex);
  int temp_latency = 0;
  mem->IssueInstruction(ins, this, thread, temp_latency, current_cycle);
  ret_latency = hit_latency + temp_latency;
  return true;
}

void L2Cache::print() {
//   printf("%d instructions in-flight",
//          instructions->Size());
}

void L2Cache::PrintStats() {
  if (unit_off) {
    printf("L2 OFF!\n");
  }
  printf("L2 accesses: \t%lld\n", accesses);
  printf("L2 hits: \t%lld\n", hits);
  printf("L2 misses: \t%lld\n", misses);
  printf("L2 stores: \t%lld\n", stores);
  printf("L2 bank conflicts: \t%lld\n", bank_conflicts);
  printf("L2 hit rate: \t%f\n", static_cast<float>(hits)/accesses);
  printf("L2 memory faults: %lld\n", memory_faults);
  printf("L2 bandwidth limited stalls: %lld\n", bandwidth_stalls);
}

double L2Cache::Utilization() {
  return static_cast<double>(processed_this_cycle) / num_banks;
}

// This updates the tag to reflect the address given
bool L2Cache::UpdateCache(int address) {
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;
  pthread_mutex_lock(&cache_mutex);
  tags[index] = tag;
  valid[index] = true;
  pthread_mutex_unlock(&cache_mutex);
  return true;
}
