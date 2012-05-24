#include "Synchronize.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <cassert>

Synchronize::Synchronize(int _latency, int _width, int _simd_width, int _num_threads) :
  FunctionalUnit(_latency), width(_width), simd_width(_simd_width),
  num_threads(_num_threads) {
  issued_this_cycle = 0;
  
  locked = new int[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    locked[i] = true;
  }
  at_lock = new int[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    at_lock[i] = false;
  }
}

// From FunctionalUnit
bool Synchronize::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::SYNC)
    return true;
  else
    return false;
}

bool Synchronize::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  int proc_id = issuer->current_proc_id;
  int simd_block_id = proc_id / simd_width;
  at_lock[proc_id] = true;
  int num_at_lock = 0;
//   if (simd_block_id == 3) {
//     printf("at_lock %d:%d: ", simd_block_id, locked[simd_block_id]);
//   }
  for (int i = 0; i < simd_width; ++i) {
    if (at_lock[simd_block_id + i]) {
      num_at_lock++;
    }
//   if (simd_block_id == 3) {
//     printf("%d ",at_lock[simd_block_id+i]);
//   }
  }
//   if (simd_block_id == 3) {
//     printf("\n");
//   }
  if (num_at_lock == simd_width) {
    // time to let everyone go
    locked[simd_block_id] = false;
  }
  
  if (locked[simd_block_id]) {
    return false;
  } else {
    at_lock[proc_id] = false;

    // check if all the threads made it past the lock so that we can turn the lock on again
    num_at_lock = 0;
    for (int i = 0; i < simd_width; ++i) {
      if (at_lock[simd_block_id + i]) {
	num_at_lock++;
      }
    }
    if (num_at_lock == 0) {
      // time to lock this block
      locked[simd_block_id] = true;
    }
    return true;
  }
}

// From HardwareModule
void Synchronize::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
}

void Synchronize::ClockFall() {
  issued_this_cycle = 0;
}

void Synchronize::print() {
  printf("%d instructions issued this cycle.", issued_this_cycle);
}
