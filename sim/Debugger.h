#ifndef __SIMHWRT_DEBUGGER_H_
#define __SIMHWRT_DEBUGGER_H_
#include <map>
#include <vector>
#include <string>
#include "Assembler.h"
#include "DwarfReader.h"
#include "Instruction.h"
#include "ThreadState.h"
#include "LocalStore.h"


struct symbol;
struct SourceInfo;

struct BreakPoint
{
  SourceInfo stop_at;
  bool currentlyExecuting;
  // other state to come (enabled, num hits, etc).

  BreakPoint() :
  currentlyExecuting(false)
  {}

};

class Debugger
{
 public:
  Debugger();
  void setDwarfReader(DwarfReader* _dwarfReader);
  void setRegisterSymbols(const std::vector<symbol*>* _regs);
  void setLocalStore(LocalStore* _ls_unit);
  void enable();
  void disable();
  bool isEnabled();
  void run(ThreadState* thread, Instruction* ins);

 private:
  DwarfReader* dwarfReader;
  SourceInfo currentSourceInfo;
  int        currentAsmLine;
  bool       enabled;
  std::vector<BreakPoint> breakPoints;
  const std::vector<symbol*>* regs;
  LocalStore* ls_unit;
  RuntimeNode* currentFrame;
  int reg0Id;

  bool isBreakPoint(SourceInfo si);
  void printSourceInfo(SourceInfo si);
  int findSourceFile(std::string name);
  bool isInitialized();
  Location evaluateLocation(ThreadState* thread, Location varLoc, Location frameBase);
  Location findVariable(RuntimeNode* currentFrame, std::string name, ThreadState* thread, std::string& typeName);
  Location findVariableLocation(CompilationUnit* containingFrame, std::string name, ThreadState* thread, Location baseLoc, std::string& type);
  int readRegister(const Location& loc, ThreadState* thread);
  int readStack(const Location& loc, ThreadState* thread);

  inline int dwOpToRegId(int dwOp)
  {
    if(reg0Id < 0)
      {
	printf("Error: Debugger's register symbols invalid or uninitialized\n");
	exit(1);
      }
    return reg0Id + (dwOp - DW_OP_reg0);
  }

  enum Command 
  {
    NONE = 0,
    START,
    STEP,
    NEXT,
    QUIT,
    CONTINUE,
    BREAK,
    WATCH,
    PRINT,
    BACKTRACE,
    INFO,
    RUN
  };

  Command lastCommand;
  std::string lastEntry;

  void readCommand(ThreadState* thread, Instruction* ins);
  bool isExecutingCommand(int command );

};


extern std::string expFilename;
extern std::string expInteger;
extern std::string expNext;
extern std::string expQuit;
extern std::string expBreak;
extern std::string expBreakFile;
extern std::string expRun;
extern std::string expPrint;
extern std::string expInfo;
extern std::string expBacktrace;
extern std::string expHexLiteral;
extern std::string expWatch;


#endif  //__SIMHWRT_DEBUGGER_H_
