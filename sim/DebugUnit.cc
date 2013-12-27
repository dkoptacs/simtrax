#include "DebugUnit.h"
#include "SimpleRegisterFile.h"
#include "IssueUnit.h"
#include "ThreadState.h"
#include "WriteRequest.h"
#include <float.h>
#include <math.h>
#include <cassert>

DebugUnit::DebugUnit(int _latency) :
  FunctionalUnit(_latency) {
}

// From FunctionalUnit
bool DebugUnit::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::PRINT)
    return true;
  else
    return false;
}

bool DebugUnit::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (ins.args[0]==0)
    printf("\n");
  else {
    reg_value arg;
    Instruction::Opcode failop = Instruction::NOP;
    if(!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg, failop)){
      printf("PRINT: failed to read register\n");
    }
    if(registers == NULL)
      {
	printf("<%p> Register %u has value %d, %u, %f\n",
	       thread, ins.args[0], arg.idata, arg.udata, arg.fdata);
      }
    else
      {
	printf("<Thread: %p> Register %u (\"%s\") has value %d, %u, %f\n",
	       thread, ins.args[0], registers->at(ins.args[0])->names[0], arg.idata, arg.udata, arg.fdata);
      }
  }
  return true;
}

// From HardwareModule
void DebugUnit::ClockRise() {
  // We do nothing on rise (or read from register file on first cycle, but
  // we can probably claim that this was done already)
}
void DebugUnit::ClockFall() {
}
void DebugUnit::print() {
}
