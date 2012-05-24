#include <fstream>

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
			    ) {
  light_pos = new float[3];
  std::ifstream filein(file);
  // check for errors
  if (filein.fail()) 
    { printf( "Error opening memory file '%s' for reading.\n", file ); }
  
  filein >> num_blocks;
  data = new FourByte[num_blocks];

//   filein >> start_wq ;
//   filein >> start_framebuffer ;
//   filein >> start_scene ;
//   filein >> start_matls ;
//   filein >> start_camera ; 
//   filein >> start_bg_color ; 
//   filein >> start_light ;
//   filein >> end_memory ;
//   filein >> light_pos[0] >> light_pos[1] >> light_pos[2] ; 
//   filein >> start_permutation ;

  for (int i = 0; i < num_blocks; ++i) {
    filein >> data[i].ivalue;
  }
}

// dumps memory to file
void MemoryBase::WriteMemory(const char* file,
                int start_wq, int start_framebuffer, int start_scene,
		int start_matls,
                int start_camera, int start_bg_color, int start_light,
                int end_memory, float *light_pos, int start_permutation
			     ) {
  std::ofstream fileout(file);
  // check for errors
  if (fileout.fail()) 
    { printf( "Error opening memory file '%s' for writing.\n", file ); }
  
  fileout << num_blocks << std::endl;

//   fileout << start_wq << std::endl;
//   fileout << start_framebuffer << std::endl;
//   fileout << start_scene << std::endl;
//   fileout << start_matls << std::endl;
//   fileout << start_camera << std::endl; 
//   fileout << start_bg_color << std::endl; 
//   fileout << start_light << std::endl;
//   fileout << end_memory << std::endl;
//   fileout << light_pos[0] << ' ' << light_pos[1] << ' ' << light_pos[2] << std::endl; 
//   fileout << start_permutation << std::endl;

  for (int i = 0; i < num_blocks; ++i) {
    fileout << data[i].ivalue << ' ';
  }
}
