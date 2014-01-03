#include <fstream>

#include "L2Cache.h"
#include "L1Cache.h"
#include "MainMemory.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include "memory_controller.h"
#include <cassert>

extern pthread_mutex_t usimm_mutex[MAX_NUM_CHANNELS];

L2Cache::L2Cache(MainMemory* _mem, int _cache_size, int _hit_latency,
		 bool _disable_usimm, int _num_banks = 4, int _line_size = 2,
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

  disable_usimm = _disable_usimm;

  max_data_per_cycle = mem->max_bandwidth; 

  // Convert from bytes to words
  max_data_per_cycle /= 4;
  
  outstanding_data = 0;

  // Deprecated: Max BW is now in the config file
  // 8w * 1000MHz * 4B/w = 32 GB/s
  //max_data_per_cycle = 1; // number of words allowed per cycle for max 4GB/s BW (1000Mhz)
  //max_data_per_cycle = 2; // number of words allowed per cycle for max 8GB/s BW (1000Mhz)
  //max_data_per_cycle = 8; // number of words allowed per cycle for max 32GB/s BW (1000Mhz)
  //max_data_per_cycle = 64; // number of words allowed per cycle for max 256GB/s BW (1000Mhz)

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
  current_cycle = 0;

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
  //if(current_cycle == 148191)
  //printf("here\n");

  // Commit all updates that should be completed by now
  pthread_mutex_lock(&cache_mutex);
  for (std::vector<CacheUpdate>::iterator i = update_list.begin(); i != update_list.end();) {
    if (i->update_cycle <= current_cycle) {
      //917504, 6751
      //if(i->index == 6751)
      //printf("L2 validating index on cycle %lld\n", current_cycle);
      tags[i->index] = i->tag;
      valid[i->index] = true;
      // remove completed cache update from the list
      //printf("cycle %lld, removing from l2 queue\n", current_cycle);
      update_list.erase(i); // this increments the iterator for some reason
    }
    else ++i; // normal increment
  }
  current_cycle++;
  pthread_mutex_unlock(&cache_mutex);
}

bool L2Cache::IssueInstruction(Instruction* ins, L1Cache * L1, ThreadState* thread, long long int& ret_latency, long long int issuer_current_cycle, int address, int& unroll_type) {


  //TODO: check if issuer_current_cycle matches current_cycle

  //current_cycle = issuer_current_cycle;
  //  int bank_id = 0;
//   if (issued_this_cycle >= num_banks)
//     return false;

  // handle loads
  if (ins->op == Instruction::LOAD) {

    //int address = thread->registers->ReadInt(ins->args[1], issuer_current_cycle) + ins->args[2];
    //if(address != L1_addr)
    //{
    //	printf("error, mismatched address\n");
    //	printf("from instr: %d, from l1: %d\n", address, L1_addr);
    //}
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
    if (last_issued[bank_id] == issuer_current_cycle && !unit_off) {
      pthread_mutex_lock(&cache_mutex);
      bank_conflicts++;
      pthread_mutex_unlock(&cache_mutex);
      return false;
    }
    // check for a hit
    int index = (address & index_mask) >> index_shift;

    //printf("L2: address = %d, line = %d\n", address, index);

    int tag = address & tag_mask;
    if (tags[index] != tag || !valid[index] || unit_off) 
      {
      // miss (add main memory latency)

      long long int queued_latency = 0;
      if(PendingUpdate(address, queued_latency)) // check for incoming cache lines (some time in the future)
	{ 
	  //if(tag == 917504 && index == 6751)
	  //printf("cycle %lld, L2 returning pending update\n", current_cycle);
	  ret_latency = hit_latency + queued_latency;
	  
	  if(queued_latency > 0)
	    {
	      pthread_mutex_lock(&cache_mutex);
	      unroll_type = UNROLL_MISS;
	      misses++;
	      pthread_mutex_unlock(&cache_mutex);
	    }
	  else
	    {
	      pthread_mutex_lock(&cache_mutex);
	      unroll_type = UNROLL_HIT;
	      hits++;
	      pthread_mutex_unlock(&cache_mutex);
	    }
	}
      else // need to go to DRAM
	{ 
	  if(disable_usimm)
	    {
	      pthread_mutex_lock(&cache_mutex);
	      if(outstanding_data >= max_outstanding_data)
		{
		  bandwidth_stalls++;
		  pthread_mutex_unlock(&cache_mutex);
		  return false;
		}
	      outstanding_data += line_fill_size;
	      pthread_mutex_unlock(&cache_mutex);
	    }
	  
	  
	  reg_value result;
	  result.udata = data[address].uvalue;
	  
	  if(!disable_usimm)
	    {
	      long long int old_ready = thread->register_ready[ins->args[0]];
	      thread->register_ready[ins->args[0]] = UNKNOWN_LATENCY;
	      
	      long long int usimmAddr = traxAddrToUsimm(address);
	      dram_address_t dram_addr = calcDramAddr(usimmAddr);


	      //printf("channel = %d\n", dram_addr.channel);

	      //if(tag == 917504 && index == 6751)
	      //printf("cycle %lld, L2 going to dram\n", current_cycle);
	      
	      pthread_mutex_lock(&(usimm_mutex[dram_addr.channel]));
	      //pthread_mutex_lock(&(usimm_mutex[0]));
	      // give usimm current_cycle * 5 since it is operating at 5x frequency
	      request_t *req = insert_read(dram_addr, address, issuer_current_cycle * DRAM_CLOCK_MULTIPLIER, thread->thread_id, 0, ins->pc_address, ins->args[0], result, ins->op, thread, L1, this);
	      pthread_mutex_unlock(&(usimm_mutex[dram_addr.channel]));
	      //pthread_mutex_unlock(&(usimm_mutex[0]));
	      
	      if(req == NULL) // read queue was full
		{
		  //TODO: Need to track these read queue stalls
		  thread->register_ready[ins->args[0]] = old_ready;
		  return false;
		}
	      
	      // check if the request already exist, if so keep track of that stat
	      // will need to subtract some energy cost since these coalesced reads don't cause another bus transfer (L2 -> L1)

	      if(req->request_served == 1)
		{
		  ret_latency = hit_latency + (req->completion_time / DRAM_CLOCK_MULTIPLIER - issuer_current_cycle);
		  
		  //if(tag == 917504 && index == 6751)
		  //printf("cycle %lld, L2 returning serviced request\n", current_cycle);

	  
		  //UpdateCache(address, req->completion_time / DRAM_CLOCK_MULTIPLIER);
		  
		}
	      else
		{
		  //if(tag == 917504 && index == 6751)
		  //printf("cycle %lld, L2 returning unknown latency\n", current_cycle);
		  
		  ret_latency = UNKNOWN_LATENCY;
		}
	    }
	  else
	    {
	      ret_latency = hit_latency + mem->GetLatency(ins); 
	      if(!UpdateCache(address, ret_latency + current_cycle))
		return false;
	    }
	  
	  pthread_mutex_lock(&cache_mutex);
	  unroll_type = UNROLL_MISS;
	  misses++;
	  pthread_mutex_unlock(&cache_mutex);
	}
      }
    else 
      {
	//if(tag == 917504 && index == 6751)
	//printf("cycle %lld, L2 HIT\n", current_cycle);
	// hit (queue load)
	ret_latency = hit_latency;
	pthread_mutex_lock(&cache_mutex);
	hits++;
	unroll_type = UNROLL_HIT;
	pthread_mutex_unlock(&cache_mutex);
      }
    pthread_mutex_lock(&cache_mutex);
    last_issued[bank_id] = issuer_current_cycle;
    accesses++;
    pthread_mutex_unlock(&cache_mutex);
    return true;
  }

  // atomic fp add
  if (ins->op == Instruction::ATOMIC_FPADD) {
    //int address = thread->registers->ReadInt(ins->args[0], issuer_current_cycle) + ins->args[2];
    int bank_id = address % num_banks;
    // check for bank conflicts
    if (last_issued[bank_id] == issuer_current_cycle && !unit_off) {
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
    last_issued[bank_id] = issuer_current_cycle;
    // add memory latency
    ret_latency = hit_latency + mem->GetLatency(ins);
    return true;
  }// end atominc


  // writes
  // should probably check for max_concurrent (MSHR)
  //int address = thread->registers->ReadInt(ins->args[0], issuer_current_cycle) + ins->args[2];
  if (address < 0 || address >= num_blocks) {
    printf("ERROR: L2 MEMORY FAULT.  REQUEST FOR STORE TO ADDRESS %d (not in [0, %d])\n\t",
	   address, num_blocks);
    ins->print();
    printf("\n");
  }
  int bank_id = address % num_banks;
  // check for bank conflicts
  if (last_issued[bank_id] == issuer_current_cycle && !unit_off) {
    pthread_mutex_lock(&cache_mutex);
    bank_conflicts++;
    pthread_mutex_unlock(&cache_mutex);
    return false;
  }

  if(disable_usimm)
    {
      pthread_mutex_lock(&cache_mutex);
      if(outstanding_data >= max_outstanding_data)
	{
	  
	  bandwidth_stalls++;
	  pthread_mutex_unlock(&cache_mutex);
	  return false;
	}
      outstanding_data += line_fill_size;
      pthread_mutex_unlock(&cache_mutex);
    }

// if caches are write-around, disable this
#if 0
  // check for line in cache
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;
  if (tags[index] == tag) { // ***** is this right? ***** //
    // set as dirty
    //tags[index] = 0xFFFFFFFF;

    // cache it
    UpdateCache(address, current_cycle);
    //tags[index] = tag;
  }
#endif

  reg_value result;

  if(!disable_usimm)
    {

      long long int usimmAddr = traxAddrToUsimm(address);
      dram_address_t dram_addr = calcDramAddr(usimmAddr);
      
      pthread_mutex_lock(&(usimm_mutex[dram_addr.channel]));
      //pthread_mutex_lock(&(usimm_mutex[0]));
      request_t *req = insert_write(dram_addr, address, issuer_current_cycle * DRAM_CLOCK_MULTIPLIER, thread->thread_id, 0, 0, ins->args[0], result, ins->op, thread, L1, this);
      pthread_mutex_unlock(&(usimm_mutex[dram_addr.channel]));
      //pthread_mutex_unlock(&(usimm_mutex[0]));
      if(req == NULL) // write queue was full
	return false;
    }

  pthread_mutex_lock(&cache_mutex);
  last_issued[bank_id] = issuer_current_cycle;
  stores++;
  misses++;
  accesses++;
  pthread_mutex_unlock(&cache_mutex);
  int temp_latency = 0;
  mem->IssueInstruction(ins, this, thread, temp_latency, issuer_current_cycle);
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
  if(disable_usimm)
    printf("L2 bandwidth limited stalls: %lld\n", bandwidth_stalls);
}

double L2Cache::Utilization() {
  return static_cast<double>(processed_this_cycle) / num_banks;
}

// This schedules an update to the tag to reflect the address given
bool L2Cache::UpdateCache(int address, long long int update_cycle) {
  //TODO: Enable this for "fake" read queue limiting
  //if(disable_usimm)
  //{
  //     if(update_list.size() >= 80)
  //	return false;
  //}
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;
  //printf("adding address %d on cycle %lld to l2 queue\n", address, update_cycle);
  pthread_mutex_lock(&cache_mutex);
  update_list.push_back(CacheUpdate(index, tag, update_cycle));
  // tags[index] = tag;
  // valid[index] = true;
  pthread_mutex_unlock(&cache_mutex);
  return true;
}

bool L2Cache::BankConflict(int address, long long int cycle)
{
  int bank_id = address % num_banks;
  // check for bank conflicts                                                                                                                       
  if (last_issued[bank_id] == cycle && !unit_off) {
    pthread_mutex_lock(&cache_mutex);
    bank_conflicts++;
    pthread_mutex_unlock(&cache_mutex);
    return true;
  }
  return false;
}

// Checks for a future incoming cache line for the address given
// This shouldn't  need to be synchronized since it's read-only
bool L2Cache::PendingUpdate(int address, long long int& temp_latency)
{
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;

  for (std::vector<CacheUpdate>::iterator i = update_list.begin(); i != update_list.end(); i++) 
    {
      //printf("\ti->tag = %d, i->cycle = %lld\n", i->tag, i->update_cycle);
      if(i->tag == tag && i->index == index)
	{
	  temp_latency = i->update_cycle - current_cycle;
	  return true;
	}
    } 
  return false;
}

// convert the TRaX address to byte-addressed, cache-line-aligned
long long int L2Cache::traxAddrToUsimm(int address)
{
  long long int retVal = address;

  // multiply by 4 (word -> byte), then mask off bits to get cache line number 
  retVal *= 4;
  retVal &= (0xFFFFFFFF << (line_size + 2)); // +2 here to convert from word line-size to byte line-size    

  return retVal;
}
