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

#if TRACK_LINE_STATS
  total_reads = new long long int[cache_size>>line_size];
  memset(total_reads, 0, sizeof(long long int)*cache_size>>line_size);
  total_validates = new long long int[cache_size>>line_size];
  memset(total_validates, 0, sizeof(long long int)*cache_size>>line_size);
  line_accesses = new long long int[cache_size>>line_size];
  memset(line_accesses, 0, sizeof(long long int)*cache_size>>line_size);
#endif
  
  issued_this_cycle = new int[num_banks];
  read_address = new int[num_banks];
  for (int i = 0; i < num_banks; ++i) {
    issued_this_cycle[i] = 0;
    read_address[i] = -1;
  }
  //issued_this_cycle = 0;

  // start at cycle 0
  current_cycle = 0;

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
  bus_transfers = 0;
  bus_hits = 0;
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
  bus_transfers = 0;
  bus_hits = 0;
}

bool L1Cache::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::LOAD || op == Instruction::STORE || op == Instruction::ATOMIC_FPADD || op == Instruction::LOADL1)
    return true;
  return false;
}

bool L1Cache::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  // Synchronize current cycle with issuer
  //current_cycle = issuer->current_cycle;
  
  //int bank_id = 0;
  long long int temp_latency = 0;
  long long int bus_latency = 0;
  BusTransfer *bus_transfer;
  int unroll_type = 0;
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
    
    bus_latency = IsOnBus(address, bus_transfer);
    
    //printf("cycle: %lld, PC %d, loading address %d\n", issuer->current_cycle, ins.pc_address, address);
    int bank_id = address % num_banks;
    if (read_address[bank_id] == address) {
      same_word_conflicts++;
      if (read_copy) {
	//printf("\tread_copy hit\n", address);
	// allow the load to happen for free!
	// queue register write
	long long int write_cycle = hit_latency + issuer->current_cycle;
	reg_value result;
	result.udata = data[address].uvalue;
	if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	  //printf("pipeline hazzard1\n");
	  // pipeline hazzard
	  return false;
	}
	//printf("\thit\n");
	hits++;
	accesses++;
	//TODO: Take this out?
	//UpdateCache(address, write_cycle);
	issued_this_cycle[bank_id]++;
	return true;
      }
    }
    if (issued_this_cycle[bank_id] && !unit_off) {
      bank_conflicts++;
      //printf("\tbank conflict\n", address);
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
    //printf("\ttag = %d, index = %d\n", tag, index);
    //    int index_tag = (address & index_tag_mask) >> index_shift;
    if (tags[index] != tag || !valid[index] || unit_off) {
      //printf("\tmiss\n", address);
      // cache miss, check nearby L1s (if snooping set)
      if ((L1_1 != NULL && L1_1->snoop(address)) ||
	  (L1_2 != NULL && L1_2->snoop(address)) || 
	  (L1_3 != NULL && L1_3->snoop(address))) {
	// hit here
	int near_latency = 5;
	// queue register write
	long long int write_cycle = hit_latency + issuer->current_cycle + near_latency;
	reg_value result;
	result.udata = data[address].uvalue;
	if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	  //printf("pipeline hazzard2\n");
	  // pipeline hazzard
	  return false;
	}
	nearby_hits++;
	accesses++;

#if TRACK_LINE_STATS
	line_accesses[index]++;
#endif
	
	//TODO: Take this out?
	//UpdateCache(address, write_cycle);
	read_address[bank_id] = address;
	issued_this_cycle[bank_id]++;
	return true;
      }
      

      //TODO: Need to figure out a solution to the (TODO) below before turning this on at all
      //      Currently not tracking the L2->L1 bus utilization properly.
      //      Not all L2 accesses cause a bus transfer
      //      For example, two consecutive loads to the same cache line, going to the same L1, only 1 bus transfer
      
      /*
      if(PendingUpdate(address, temp_latency)) // check for incoming cache lines (some time in the future)
	{


	// If the L1 tracks its own incoming lines, then it doesn't need to go to the L2, and don't need to check for bank conflict

	  //if(L2->BankConflict(address, issuer->current_cycle))
	  //{
	      // L2 stalled
	  //  return false;
	  //}
	  	  
	  reg_value result;
	  result.udata = data[address].uvalue;
	  
	  long long int write_cycle = hit_latency + temp_latency + issuer->current_cycle;
	  if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {	      
	    // pipeline hazzard
	    return false;
	  }
	  misses++;
	  accesses++;

	  //TODO: For now I'm considering this an L2 access. The energy consumption is significantly less, however.
	  //      Will need to track these as "L2_read_hits" or something
	  //      Also, it's unclear where this cache line is coming from, is it just from the L2, or is it from usimm?
	  pthread_mutex_lock(&(L2->cache_mutex));
	  L2->accesses++;
	  L2->hits++;
	  pthread_mutex_unlock(&(L2->cache_mutex));
	 
	  
	  read_address[bank_id] = address;
	  issued_this_cycle[bank_id]++;
	  return true;
	}
*/


      //TODO: WORKING HERE:
      //      Need to run simulations with all this bus stuff turned off.
      //      Latency and energy will be accurate assuming the HW is simple,
      //      meaning no bus monitoring (same line can be transfered twice on consecutive cycles)

      // TODO: This needs to track whether the bus traffic is coming from dram or from the L2
      //       Currently just count it as the L1 having it's own bus queue monitoring HW,
      //       so it doesn't go out to L2 or DRAM at all if it finds it on the bus
      //       Figure out where it's reasonable or not to assume each L1 has its own queue
      

      // Need to tack on this threads request to the existing bus transfer if UNKNOWN_LATENCY, 
      // so that it can be notified when ready

      
      else if(bus_latency != -1)
	{

	  if(bus_latency != UNKNOWN_LATENCY)	    
	    bus_latency += hit_latency;
	  
	  //printf("\tfound on bus with latency %lld\n", bus_latency);	  
	  //printf("here1, temp latency = %lld\n", temp_latency);
	  
	  
	  
	  reg_value result;
	  result.udata = data[address].uvalue;
	  // TODO: Problem here si that this write is never updated if incoming time is UNKNOWN_LATENCY,
	  //       noramally those are updated by usimm since it has a pointer to all of the threads
	  //       waiting, but this doesn't add itself to usimm's list
	  //printf("\tnotifying thread, register %d, cycle %d, result %u\n", ins.args[0], bus_latency, result.udata);
	  if (!thread->QueueWrite(ins.args[0], result, bus_latency, ins.op))
	    {
	      //printf("\tpipeline hazzard\n");
	      //printf("here2\n");
	      // pipeline hazzard
	      return false;
	    }
	  
	  
	  //printf("\tadding recipient with reg %d to transfer index %d, tag %d, prev size = %d\n", ins.args[0], bus_transfer->index, bus_transfer->tag, bus_transfer->recipients.size());
	  bus_transfer->AddRecipient(address, ins.args[0], thread);
	  //printf("\tsize = %d\n", bus_transfer->recipients.size());
	  //printf("here3\n");
	  bus_hits++;
	  misses++;
	  accesses++;
	  read_address[bank_id] = address;
	  issued_this_cycle[bank_id]++;
	  return true;
	}
      
      else if (L2->IssueInstruction(&ins, this, thread, temp_latency, issuer->current_cycle, address, unroll_type)) 
	{
	  
	  reg_value result;
	  result.udata = data[address].uvalue;
	  
	  // If we know the latency, we can queue up the write
	  if(temp_latency != UNKNOWN_LATENCY)
	    {
	      // queue write to register
	      long long int write_cycle = hit_latency + temp_latency + issuer->current_cycle;
	      
	      
	      //printf("queueing write for %lld\n", write_cycle);
	      //printf("\tnotifying thread, register %d, cycle %d, result %u\n", ins.args[0], write_cycle, result.udata);
	      if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {	      
		// pipeline hazzard
		pthread_mutex_lock(&(L2->cache_mutex));
		L2->accesses--;
		if(unroll_type == UNROLL_MISS)
		  L2->misses--;
		else
		  L2->hits--;
		pthread_mutex_unlock(&(L2->cache_mutex));
		//printf("pipeline hazzard3\n");
		return false;
	      }
	      // TODO: use the bus traffic to avoid unecessary usimm reads
	      // If a read is coming on the bus from L2, then that line gets invalidated next cycle and read again,
	      // the second read will go all the way to usimm, when it can just read it off the bus
	      AddBusTraffic(address, write_cycle, thread, ins.args[0]);
	      UpdateCache(address, write_cycle);
	    }
	  else
	    {
	      // Otherwise, enqueue it with unknown latency, usimm will update it's return cycle later
	      //printf("queueing write for %lld\n", UNKNOWN_LATENCY);
	      
	      if (!thread->QueueWrite(ins.args[0], result, UNKNOWN_LATENCY, ins.op)) 
		{
		  //printf("L1: queue write failed, reg: %d, val: %u\n", ins.args[0], result.udata);
		  // pipeline hazzard
		  pthread_mutex_lock(&(L2->cache_mutex));
		  L2->accesses--;
		  if(unroll_type == UNROLL_MISS)
		    L2->misses--;
		  else
		    L2->hits--;
		  pthread_mutex_unlock(&(L2->cache_mutex));
		  //printf("pipeline hazzard4\n");
		  return false;
		}
	      
	      //thread->register_ready[ins.args[0]] = UNKNOWN_LATENCY; // set the register not ready, will be updated by usimm
	      AddBusTraffic(address, UNKNOWN_LATENCY, thread, ins.args[0]);
	    }

	  //printf("\tmiss\n");
	  misses++;
	  accesses++;	  
	  read_address[bank_id] = address;
	  issued_this_cycle[bank_id]++;

#if TRACK_LINE_STATS
	  line_accesses[index]++;
#endif

	  return true;
	} 
      else 
	{
	  // L2 stalled
	  return false;
	}
    } 
    else 
      { // cache hit
	//printf("\thit\n", address);
	// queue register write
	long long int write_cycle = hit_latency + issuer->current_cycle;
	reg_value result;
	result.udata = data[address].uvalue;
	//printf("\tnotifying thread, register %d, cycle %d, result %u\n", ins.args[0], write_cycle, result.udata);
	if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) 
	  {
	    // pipeline hazzard
	    //printf("pipeline hazzard5\n");
	    return false;
	  }

	//printf("\thit\n");
	hits++;
	accesses++;
	read_address[bank_id] = address;
	issued_this_cycle[bank_id]++;

#if TRACK_LINE_STATS
	total_reads[index]++;
	line_accesses[index]++;
#endif

	return true;
      }
  } /// end loads
  
  // Returns whether or not the address is cached in L1
  if (ins.op == Instruction::LOADL1) {
    // Read the register
    reg_value arg1;
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("Error in L1Cache LOADL1. Should have passed.\n");
    }    
    int address = arg1.idata + ins.args[2];
    //printf("Cycle: %lld, LoadL1 address: %d\n", issuer->current_cycle, address);
    int bank_id = address % num_banks;
    if (address < 0 || address >= num_blocks) {
      printf("ERROR: L1 MEMORY FAULT.  REQUEST FOR LOAD OF ADDRESS %d (not in [0, %d])\n",
             address, num_blocks);
      exit(1);
      return true;
    }
    if (issued_this_cycle[bank_id] && !unit_off) {
      bank_conflicts++;
      return false;
    }
    // check for hit



    int index = (address & index_mask) >> index_shift;
    int tag = address & tag_mask;
    //    int index_tag = (address & index_tag_mask) >> index_shift;
    if (tags[index] != tag || !valid[index] || unit_off)
      {
      //if (tags[index] != tag || unit_off) {
      // cache miss, set miss register
      //accesses++;
      // queue register write
      long long int write_cycle = hit_latency + issuer->current_cycle;
      reg_value result;
      // Fail value
      //printf("\tloadl1 missed\n");
      result.idata = 0;
      if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	// pipeline hazzard
	//printf("pipeline hazzard6\n");
	return false;
      }

      // TODO: might want to leave these next two lines in
      //read_address[bank_id] = address;
      //issued_this_cycle[bank_id]++;
      return true;
    } else {
      //printf("\tloadl1 hit\n");
      //hits++;
      //accesses++;
      // queue register write
      long long int write_cycle = hit_latency + issuer->current_cycle;
      reg_value result;
      result.idata = 1;
      if (!thread->QueueWrite(ins.args[0], result, write_cycle, ins.op)) {
	// pipeline hazzard
	
	return false;
      }
      // TODO: might want to leave these next two lines in
      //read_address[bank_id] = address;
      //issued_this_cycle[bank_id]++;
      return true;
    }
  } /// end loadl1

  // atomic fp add
  if (ins.op == Instruction::ATOMIC_FPADD) {
    // Read the register
    reg_value arg0, arg1;
    int unroll_type = 0;
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
    if (L2->IssueInstruction(&ins, this, thread, temp_latency, issuer->current_cycle, address, unroll_type)) {
      // complete in temp_latency cycles
      //      misses++;
      //      outstanding_requests++;
      pthread_mutex_lock(&atominc_mutex);
      //execute atomic add here
      data[address].fvalue += arg1.fdata;
      pthread_mutex_unlock(&atominc_mutex);
      UpdateCache(address, temp_latency + issuer->current_cycle);
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
    int unroll_type = 0;
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
    if (L2->IssueInstruction(&ins, this, thread, temp_latency, issuer->current_cycle, address, unroll_type)) {
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
  processed_this_cycle = 0;
  for (int i = 0; i < num_banks; ++i) {
    issued_this_cycle[i] = 0;
    read_address[i] = -1;
  }
  //  issued_this_cycle = 0;
  //  issued_atominc = false;
}

void L1Cache::ClockFall() {
  // Synchronize current cycle with issuer
  //current_cycle = issuer->current_cycle;

  // Commit all updates that should be completed by now
  for (std::vector<CacheUpdate>::iterator i = update_list.begin(); i != update_list.end();) {
    //printf("\ti->tag = %d, i->cycle = %lld\n", i->tag, i->update_cycle);
    if (i->update_cycle <= current_cycle) {
      //if(i->index == 95)
      //printf("L1 validating on cycle %lld, tag: %d, index: %d\n", current_cycle, i->tag, i->index);

#if TRACK_LINE_STATS
      if(tags[i->index] != i->tag)
	total_validates[i->index]++;
#endif

      tags[i->index] = i->tag;
      valid[i->index] = true;

      // remove completed cache update from the list
      //printf("cycle %lld, removing from l1 queue\n", current_cycle);
      update_list.erase(i); // this increments the iterator for some reason
    }
    else ++i; // normal increment
  }

  // Remove old bus traffic
  
  for (std::vector<BusTransfer>::iterator i = bus_traffic.begin(); i != bus_traffic.end();) {
    //printf("\ti->tag = %d, i->cycle = %lld\n", i->tag, i->update_cycle);
    if (i->update_cycle <= current_cycle) {
      //if(i->tag == 1024000 && i->index == 95)
      //printf("rempving index %d, tag %d from bus\n", i->index, i->tag);
      // remove completed cache update from the list
      bus_traffic.erase(i); // this increments the iterator for some reason
    }
    else ++i; // normal increment
  }
  


  current_cycle++;
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
bool L1Cache::UpdateCache(int address, long long int write_cycle) {
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask;
  // Schedule cache update
  //printf("adding address %d on cycle %lld to l1 queue\n", address, write_cycle);
  update_list.push_back(CacheUpdate(index, tag, write_cycle));
  // tags[index] = tag;
  // valid[index] = true;
  return true;
}

// Checks for a future incoming cache line for the address given
bool L1Cache::PendingUpdate(int address, long long int& temp_latency)
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


// The following three functions implement an MSHR of sorts
void L1Cache::AddBusTraffic(int address, long long int write_cycle, ThreadState* thread, int which_reg)
{
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask; 

  //printf("\tadding bus traffic: index = %d tag = %d, which_reg = %d\n", index, tag, which_reg);

  //if(tag == 1024000 && index == 95)
  //{
  //  printf("thread %d, adding 1024000, 95 on cycle %lld, to be completed: %lld\n", thread->thread_id, current_cycle, write_cycle);
  //}

  // Check if this cache line is already scheduled to be on the bus
  //for (std::vector<BusTransfer>::iterator i = bus_traffic.begin(); i != bus_traffic.end(); i++) 
  //{
  //  if(i->tag == tag && i->index == index)
  //	{
  //	  i->recipients.push_back(RegisterWrite(which_reg, thread));
  //	  return;
  //	}
  //} 
  //if(tag == 1024000 && index == 95)
  //printf("\tadded\n");
  bus_transfers++;
  BusTransfer transfer(index, tag, write_cycle);
  transfer.AddRecipient(address, which_reg, thread);
  //transfer.recipients.push_back(RegisterWrite(which_reg, thread));
  bus_traffic.push_back(transfer);
}

long long int L1Cache::IsOnBus(int address, BusTransfer*& transfer)
{
  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask; 

  // Check if this cache line is already scheduled to be on the bus
  for (std::vector<BusTransfer>::iterator i = bus_traffic.begin(); i != bus_traffic.end(); i++) 
    {
      if(i->tag == tag && i->index == index)
	{
	  transfer = &(*i);
	  return i->update_cycle;
	}
    } 
  return -1;
}


// Updates the return latency for loads waiting with unknown return time.
// Address passed in is only used to determine which cache line.
// Multiple loads to different addresses on the same cache line may need to be updated.
void L1Cache::UpdateBus(int address, long long int write_cycle)
{

  int index = (address & index_mask) >> index_shift;
  int tag = address & tag_mask; 
  //printf("\tupdating bus on cycle %lld, address = %d, index = %d, tag = %d\n", current_cycle, address, index, tag);
  
  for (std::vector<BusTransfer>::iterator i = bus_traffic.begin(); i != bus_traffic.end(); i++) 
    {
      if(i->tag == tag && i->index == index)
	{
	  i->update_cycle = write_cycle;
	  for(int j=0; j < (int)(i->recipients.size()); j++)
	    {
	      RegisterWrite reg_write = i->recipients.at(j);
	      reg_value result;
	      result.udata = data[reg_write.address].uvalue;
	      //printf("\tnotifying thread, register %d, cycle %d, result %u\n", reg_write.which_reg, write_cycle, result.udata);
	      //reg_write.thread->register_ready[reg_write.which_reg] = write_cycle;
	      reg_write.thread->UpdateWriteCycle(reg_write.which_reg, UNKNOWN_LATENCY, result.udata, write_cycle, Instruction::LOAD);
	      //reg_write.thread->QueueWrite(reg_write.which_reg, result, write_cycle, Instruction::LOAD);
	    }
	  return;
	}
    } 
  
  printf("error: did not find transfer on bus, current cycle: %lld\n", current_cycle);
  
  printf("looking for tag: %d, index: %d, cycle: %lld\n", tag, index, write_cycle);
  printf("bus: \n");
  for (std::vector<BusTransfer>::iterator i = bus_traffic.begin(); i != bus_traffic.end(); i++) 
    printf("tag: %d, index: %d, cycle: %lld\n", i->tag, i->index, i->update_cycle);

  exit(1);
}


// This function is for stats-tracking only.
// We will use one core to hold the sums of all other cores' stats
void L1Cache::AddStats(L1Cache* otherL1)
{
  
  hits += otherL1->hits;
  stores += otherL1->stores;
  accesses += otherL1->accesses;
  misses += otherL1->misses;
  nearby_hits += otherL1->nearby_hits;
  bank_conflicts += otherL1->bank_conflicts;
  same_word_conflicts += otherL1->same_word_conflicts;
  bus_transfers += otherL1->bus_transfers;
  bus_hits += otherL1->bus_hits;
  
}
