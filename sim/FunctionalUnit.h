#ifndef _SIMHWRT_FUNCTIONAL_UNIT_H_
#define _SIMHWRT_FUNCTIONAL_UNIT_H_

// A functional unit is an object that can execute a particular class
// of instructions.  For example, a FP ALU might only execute FPADD
// and FPSUB but not FPMUL.

#include "HardwareModule.h"
#include "Instruction.h"

class IssueUnit;
class ThreadState;

class FunctionalUnit : public HardwareModule {
 protected:
  int latency;
 public:
  FunctionalUnit(int _latency){
    latency = _latency;
  }
  int GetLatency(){
    return latency;
  }
  // If the unit supports this opcode, return true else false
  virtual bool SupportsOp(Instruction::Opcode op) const = 0;
  // If you can handle this instruction now, return true else false
  // You can assume that SupportsOps(ins.op) == true
  virtual bool AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) { return false; };
};


#endif // _SIMHWRT_FUNCTIONAL_UNIT_H_
