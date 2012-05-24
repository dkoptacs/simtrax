#ifndef _SIMHWRT_MAIN_MEMORY_H_
#define _SIMHWRT_MAIN_MEMORY_H_

#include "FunctionalUnit.h"
#include "FourByte.h"
#include "MemoryBase.h"

class L2Cache;

class MainMemory : public MemoryBase {
public:
  MainMemory(int _num_blocks, int _latency);
  FourByte* getData() {return data;}
  int getSize() {return num_blocks;}

  bool SupportsOp(Instruction::Opcode op) const {return false;}
  bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {return false;}

  // From HardwareModule
  void ClockRise() {issued_atominc = false;}
  void ClockFall() {}
  // should do something in these
  void print() {return;}
  void PrintStats() {return;}

  // L2 issuing to main memory
  bool IssueInstruction(Instruction* ins, L2Cache* L2, ThreadState* thread,
			int& ret_latency, long long int current_cycle);

  int GetLatency(Instruction* ins);

  int latency;
  // allow only one atomic increment per cycle
  bool issued_atominc;

  // stuff for incremental image output
  int store_count;
  int image_height, image_width;
  int stores_between_output;
  int start_framebuffer;
  bool incremental_output;

  // These are implemented in the parent class
//   void LoadMemory( const char* file,
// 		   int& start_wq, int& start_framebuffer, int& start_scene,
// 		   int& start_matls,
// 		   int& start_camera, int& start_bg_color, int& start_light,
// 		   int& end_memory, float *&light_pos, int &start_permutation
// 		   );
//   void WriteMemory( const char* file,
// 		    int start_wq, int start_framebuffer, int start_scene,
// 		    int start_matls,
// 		    int start_camera, int start_bg_color, int start_light,
// 		    int end_memory, float *light_pos, int start_permutation
// 		    );
  // These should only be used in top level memory... L2 or higher depending
//   FourByte* data;
//   int num_blocks;
};

#endif // _SIMHWRT_MAIN_MEMORY_H_
