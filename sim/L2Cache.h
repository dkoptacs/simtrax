#ifndef _SIMHWRT_L2_CACHE_H_
#define _SIMHWRT_L2_CACHE_H_

// A simple memory that implements one level of direct-mapped cache
// with parameterized memory size and cache size
#include "MemoryBase.h"
#include <pthread.h>

class MainMemory;
class L1Cache;

class L2Cache : public MemoryBase {
public:
  // We need line_size, cache_size, issue_width
  // line_size determines the number of words in the line by 2^line_size
  // Also need hit_latency, and miss_penalty
  // issue_width is essentially just the number of copies of the cache available.
  // cache_size is the size of the cache in blocks (words)
  // num_blocks is the size of the memory in blocks (words)
  L2Cache(MainMemory* mem, int cache_size, int hit_latency,
	  int num_banks, int line_size, bool memory_trace, bool l2_off, bool l1_off);

  ~L2Cache();
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  virtual void PrintStats();
  virtual double Utilization();

  bool IssueInstruction(Instruction* ins, L1Cache* L1, ThreadState* thread, 
			int &ret_latency, long long int current_cycle);
  void Reset();
  void Clear();

  int outstanding_data;
  int max_outstanding_data;
  int max_data_per_cycle;
  int line_fill_size;
  pthread_mutex_t cache_mutex;

  bool unit_off;
  int hit_latency;
  int cache_size;
  int num_banks;
  long long int *last_issued;
  int line_size;
  int processed_this_cycle;
  MainMemory * mem;

  // need a way to decide when instructions finish
  //InstructionPriorityQueue* instructions;

  // For address calculations
  int offset_mask, index_mask, tag_mask, index_tag_mask;
  int index_shift;

  // Address Storage
  int * tags;
  bool * valid;
  bool UpdateCache(int address);
  int issued_this_cycle;
  //  bool issued_atominc;

  // Hit statistics

  long long int bandwidth_stalls;
  long long int memory_faults;
  long long int hits, stores, accesses, misses;
  long long int bank_conflicts;
//   // Memory access record
//   bool memory_trace;
//   int *access_record;
//   int *access_hit;
//   int *access_miss;
//   int *cache_record;
//   int *cache_hits;
//   int *cache_miss;
};

#endif // _SIMHWRT_L2_CACHE_H_
