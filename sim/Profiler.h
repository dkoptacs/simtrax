#ifndef __SIMHWRT_PROFILER_H_
#define __SIMHWRT_PROFILER_H_
#include <vector>
#include <string>
#include "DwarfReader.h"

struct symbol;


class Profiler
{
 public:
  Profiler();
  void setDwarfReader(DwarfReader* _dwarfReader);
  
  RuntimeNode* MostRelevantAncestor(Instruction* ins, RuntimeNode* current);
  RuntimeNode* MostRelevantDescendant(Instruction* ins, RuntimeNode* current);
  RuntimeNode* UpdateRuntime(Instruction* ins, RuntimeNode* current_runtime, char stall_type);

  void PrintProfile(std::vector<Instruction*>& instructions, long long int total_thread_cycles);

  void WriteDot(const char* filename);
  void WriteDotRecursive(FILE* output, CompilationUnit node);

 private:
  DwarfReader* dwarfReader;

};

// Profiler's stall type codes
#define STALL_EXECUTE 1 // not really a stall, but must count the cycle for issuing
#define STALL_DATA 2
#define STALL_CONTENTION 3


#endif  //__SIMHWRT_PROFILER_H_
