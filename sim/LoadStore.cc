#include "LoadStore.h"

LoadStore::LoadStore() :
    FunctionalUnit(0)
{
}

bool LoadStore::SupportsOp(Instruction::Opcode op) const
{
  return false;
}

bool LoadStore::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread)
{
  return false;
}

void LoadStore::ClockRise()
{
}

void LoadStore::ClockFall()
{
}
