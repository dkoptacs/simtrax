#ifndef __SIMHWRT_DEBUGGER_H_
#define __SIMHWRT_DEBUGGER_H_
#include <vector>
#include <string>
#include "Assembler.h"
#include "DwarfReader.h"
#include "Instruction.h"
#include "ThreadState.h"

struct symbol;
struct SourceInfo;

class Debugger
{
 public:
  Debugger();
  void setDwarfReader(const DwarfReader* _dwarfReader);
  void enable();
  void disable();
  bool isEnabled();
  void run(ThreadState* thread, Instruction* ins);

 private:
  const DwarfReader* dwarfReader;
  SourceInfo currentSourceLine;
  int        currentAsmLine;
  bool       enabled;

  enum Command 
  {
    NONE = 0,
    START,
    STEP,
    NEXT,
    QUIT,
    CONTINUE,
    RUN
  };

  Command lastCommand;

  void readCommand(ThreadState* thread, Instruction* ins);
  bool isExecutingCommand(int command );

};


#endif  //__SIMHWRT_DEBUGGER_H_
