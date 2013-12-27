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

class CacheUpdate {
 public:
  int index, tag;
  long long int update_cycle;
 CacheUpdate(int _index, int _tag, long long int _update_cycle) 
   :index(_index), tag(_tag), update_cycle(_update_cycle) {};
};

class RegisterWrite
{
 public:
  int address;
  int which_reg;
  ThreadState* thread;
 RegisterWrite(int _address, int _which_reg, ThreadState* _thread) 
   :address(_address), which_reg(_which_reg), thread(_thread) {};
};

class BusTransfer {
 public:
  int index, tag;
  long long int update_cycle;
  std::vector<RegisterWrite> recipients;
 BusTransfer(int _index, int _tag, long long int _update_cycle)
   :index(_index), tag(_tag), update_cycle(_update_cycle) {};
  void AddRecipient(int _address, int _which_reg, ThreadState* _thread)
  {
    recipients.push_back(RegisterWrite(_address, _which_reg, _thread));
  }
};


#endif // _SIMHWRT_MEMORY_BASE_H_
