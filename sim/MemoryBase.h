#ifndef _SIMHWRT_MEMORY_BASE_H_
#define _SIMHWRT_MEMORY_BASE_H_

#include "FunctionalUnit.h"
#include "FourByte.h"

class MemoryBase : public FunctionalUnit {
public:
  MemoryBase();
  virtual ~MemoryBase();
  virtual FourByte* getData();
  virtual int getSize();

  virtual bool SupportsOp(Instruction::Opcode op) const;
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread);

  // From HardwareModule
  virtual void ClockRise();
  virtual void ClockFall();
  virtual void print();
  virtual void PrintStats();

  void LoadMemory( const char* file,
		   int& start_wq, int& start_framebuffer, int& start_scene,
		   int& start_matls,
		   int& start_camera, int& start_bg_color, int& start_light,
		   int& end_memory, float *&light_pos, int &start_permutation
		   );
  void WriteMemory( const char* file,
		    int start_wq, int start_framebuffer, int start_scene,
		    int start_matls,
		    int start_camera, int start_bg_color, int start_light,
		    int end_memory, float *light_pos, int start_permutation
		    );

  // These should only be used in top level memory... L2 or higher depending
  FourByte* data;
  int num_blocks;
};

#endif // _SIMHWRT_MEMORY_BASE_H_
