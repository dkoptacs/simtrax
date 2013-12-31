#include <fstream>
#include <stdlib.h> // for gcc (exit)

#include "MemoryBase.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"

MemoryBase::MemoryBase() : FunctionalUnit(0){}
MemoryBase::~MemoryBase() {
  delete data;
}
FourByte* MemoryBase::getData() {
  return data;
}
int MemoryBase::getSize() {
  return num_blocks;
}

bool MemoryBase::SupportsOp(Instruction::Opcode op) const {
  return false;
}
bool MemoryBase::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  printf("In MemoryBase::AcceptInstruction. Something is wrong.\n");
  return false;
}

// From HardwareModule
void MemoryBase::ClockRise() {
}
void MemoryBase::ClockFall() {
}
void MemoryBase::print() {
}
void MemoryBase::PrintStats() {
}

// Loads memory from file (previously dumped)
void MemoryBase::LoadMemory(const char* file,
                int& start_wq, int& start_framebuffer, int& start_scene,
		int& start_matls,
                int& start_camera, int& start_bg_color, int& start_light,
                int& end_memory, float *&light_pos, int &start_permutation
			    ) 
{

  FILE *input = fopen(file, "rb");
  if(!input)
    {
      printf("failed to open memory file %s\n", file);
      exit(1);
    }

  int numRead = fread(data, sizeof(FourByte), num_blocks, input);

  if(numRead <= 0)
    {
      printf("error: could not read memory file %s\n", file);
      exit(1);
    }

  printf("Read %d blocks from memory dump (mem size = %d)\n", numRead, num_blocks);

}

// dumps memory to file
void MemoryBase::WriteMemory(const char* file,
                int start_wq, int start_framebuffer, int start_scene,
		int start_matls,
                int start_camera, int start_bg_color, int start_light,
                int end_memory, float *light_pos, int start_permutation
			     ) {

  FILE *output = fopen(file, "wb");
  if(!output)
    {
      printf("failed to open memory file %s\n", file);
      exit(1);
    }

  printf("Writing %d blocks to memory dump (mem size = %d)\n", end_memory, num_blocks);
  if(fwrite(data, sizeof(FourByte), end_memory, output) != end_memory)
    {
      printf("error: could not write memory file %s\n", file);
      exit(1);
    }
}
