#ifndef _SIMHWRT_L1_CACHE_H_
#define _SIMHWRT_L1_CACHE_H_

// A simple memory that implements one level of direct-mapped cache
// with parameterized memory size and cache size
#include "MemoryBase.h"
#include "MainMemory.h"

#define TRACK_LINE_STATS 0

class L2Cache;
class MainMemory;

class L1Cache : public MemoryBase {
public:
  // We need line_size, cache_size, issue_width
  // line_size determines the number of words in the line by 2^line_size
  // Also need hit_latency, and miss_penalty
  // issue_width is essentially just the number of copies of the cache available.
  // cache_size is the size of the cache in blocks (words)
  // num_blocks is the size of the memory in blocks (words)
  L1Cache(L2Cache* L2, int hit_latency,
	  int cache_size, int num_banks, int line_size, 
	  bool memory_trace, bool l1_off, bool l1_read_copy);

  ~L1Cache();
  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  virtual void PrintStats();
  virtual double Utilization();
  void Clear();
  void Reset();
  void AddStats(L1Cache* otherL1);

  bool snoop(int address);

  bool unit_off;
  bool read_copy;
  int hit_latency;
  int cache_size;
  int num_banks;
  int line_size;
  int processed_this_cycle;
  L2Cache * L2;
  L1Cache * L1_1;
  L1Cache * L1_2;
  L1Cache * L1_3;

  // need a way to decide when instructions finish
  //InstructionPriorityQueue* instructions;

  // For address calculations
  int offset_mask, index_mask, tag_mask, index_tag_mask;
  int index_shift;

  // Address Storage
  int * tags;
  bool * valid;
  //long long int * reads_since_validate;
#if TRACK_LINE_STATS
  long long int * total_reads;
  long long int * total_validates;
  long long int * line_accesses;
#endif
  bool UpdateCache(int address, long long int write_cycle);
  void AddBusTraffic(int address, long long int write_cycle, ThreadState* thread, int which_reg);
  void UpdateBus(int address, long long int write_cycle);
  bool PendingUpdate(int address, long long int& temp_latency);
  long long int IsOnBus(int address, BusTransfer*& transfer);
  int * issued_this_cycle;
  int * read_address;
  std::vector<CacheUpdate> update_list;
  std::vector<BusTransfer> bus_traffic;
  //  bool issued_atominc;

  // cycle count
  long long int current_cycle;

  // Hit statistics
  long long int hits, stores, accesses, misses;
  long long int nearby_hits;
  long long int bank_conflicts;
  long long int same_word_conflicts;
  long long int bus_transfers;
  long long int bus_hits;
  
//   // Memory access record
//   bool memory_trace;
//   int *access_record;
//   int *access_hit;
//   int *access_miss;
//   int *cache_record;
//   int *cache_hits;
//   int *cache_miss;
};

#endif // _SIMHWRT_L1_CACHE_H_
