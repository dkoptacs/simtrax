#include "LocalStore.h"
#include "IssueUnit.h"

#define LOCAL_SIZE 16384

LocalStore::LocalStore(int _latency, int _width) :
  FunctionalUnit(_latency){
  issued_this_cycle = 0;
  width = _width;
  
  storage = new FourByte*[width];
  for (int i = 0; i < width; ++i) {
    storage[i] = new FourByte[LOCAL_SIZE];
  }
}

LocalStore::~LocalStore() {
  for (int i = 0; i < width; ++i) {
    delete [] storage[i];
  }
  delete [] storage;
}

bool LocalStore::SupportsOp(Instruction::Opcode op) const {
  if (op == Instruction::LWI || 
      op == Instruction::LW ||
      op == Instruction::lbu ||
      op == Instruction::lbui ||
      op == Instruction::SWI ||
      op == Instruction::SW ||
      op == Instruction::sh ||
      op == Instruction::shi ||
      op == Instruction::sb ||
      op == Instruction::sbi )
    return true;
  else
    return false;
}

#define BITMASK 0x000000FF
#define HALFMASK 0x0000FFFF

bool LocalStore::IssueLoad(int write_reg, int address, ThreadState* thread, long long int write_cycle, Instruction& ins) {
  reg_value result;
  if (address < 0 || address > LOCAL_SIZE) {
    printf("Error Issuing load in LocalStore. 0x%8x not in [0x%8x, 0x%8x].\n", address, 0, LOCAL_SIZE);
  }

  // check for byte loads
  if (ins.op == Instruction::lbu || ins.op == Instruction::lbui) {
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id])) + address)->uvalue & BITMASK;
  } else {
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id])) + address)->uvalue;
  }
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op)) {
    // pipeline hazzard
    return false;
  }
  return true;
}

bool LocalStore::IssueStore(reg_value write_val, int address, ThreadState* thread, long long int write_cycle, Instruction& ins) {
  if (address < 0 || address > LOCAL_SIZE) {
    printf("Error Issuing load in LocalStore. 0x%8x not in [0x%8x, 0x%8x].\n", address, 0, LOCAL_SIZE);
  }
  static int max_addr = 0;
  if (address > max_addr) {
    max_addr = address;
    // This is not as interesting as I'd initially thought since stacks grow down and not up.
    //printf("Max LocalStore Address: 0x%8x\n", max_addr);
  }
  // write write_val to mem[address] for thread
  if (ins.op == Instruction::sb || ins.op == Instruction::sbi) {
    ((FourByte *)((char *)(storage[thread->thread_id])) + address)->uvalue = 
      (((FourByte *)((char *)(storage[thread->thread_id])) + address)->uvalue & ~(BITMASK)) +
      (write_val.udata & BITMASK);
  } else if (ins.op == Instruction::sh || ins.op == Instruction::shi) {
    ((FourByte *)((char *)(storage[thread->thread_id])) + address)->uvalue = 
      (((FourByte *)((char *)(storage[thread->thread_id])) + address)->uvalue & ~(HALFMASK)) +
      (write_val.udata & HALFMASK);
  } else {
    //     storage[thread->thread_id][address].uvalue = write_val.udata;
    ((FourByte *)((char *)(storage[thread->thread_id])) + address)->uvalue = write_val.udata;
  }
  return true;
}

bool LocalStore::AcceptInstruction(Instruction& ins, IssueUnit* issuer, ThreadState* thread) {
  if (issued_this_cycle >= width) return false;
  
  Instruction::Opcode failop;
  long long int write_cycle = issuer->current_cycle + latency;
  reg_value arg0, arg1, arg2;
  int address;
  int write_reg;
  switch (ins.op) {
  case Instruction::LW:
  case Instruction::lbu:
    if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
      // bad times
      printf("Error in LocalStore.\n");
    }
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // something bad
      printf("Error in LocalStore.\n");
    }
    address = arg1.idata + arg2.idata;
    write_reg = ins.args[0];
    if (!IssueLoad(write_reg, address, thread, write_cycle, ins))
      return false;
    break;
  case Instruction::LWI:
  case Instruction::lbui:
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // something bad
      printf("Error in LocalStore.\n");
    }
    address = arg1.idata + ins.args[2];
    write_reg = ins.args[0];
    if (!IssueLoad(write_reg, address, thread, write_cycle, ins))
      return false;
    break;
  case Instruction::SWI:
  case Instruction::sbi:
  case Instruction::shi:
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop)) {
      // something bad
      printf("Error in LocalStore.\n");
    }
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // something bad
      printf("Error in LocalStore.\n");
    }
    address = arg1.idata + ins.args[2];
    if (!IssueStore(arg0, address, thread, write_cycle, ins))
      return false;
    break;
  case Instruction::SW:
  case Instruction::sb:
  case Instruction::sh:
    if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop)) {
      // something bad
      printf("Error in LocalStore.\n");
    }
    if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop)) {
      // something bad
      printf("Error in LocalStore.\n");
    }
    if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop)) {
      // something bad
      printf("Error in LocalStore.\n");
    }
    address = arg1.idata + arg2.idata;
    if (!IssueStore(arg0, address, thread, write_cycle, ins))
      return false;
    break;
  default:
    fprintf(stderr, "ERROR LOCALSTORE FOUND SOME OTHER OP\n");
    return false;
  }
  issued_this_cycle++;
  return true;
}

void LocalStore::ClockRise() {
  issued_this_cycle = 0;
}
void LocalStore::ClockFall() {
}
void LocalStore::print() {
  printf("%d instructions issued this cycle",issued_this_cycle);
}

double LocalStore::Utilization() {
  return static_cast<double>(issued_this_cycle)/static_cast<double>(width);
}

// load jump table in to low order words of stack (top of stack space)
//TODO: Add error checking for stomping on these values
void LocalStore::LoadJumpTable(std::vector<int> jump_table)
{
  for(int i=0; i < width; i++)
    for(size_t j=0; j < jump_table.size(); j++)
      {
	((FourByte *)((char *)(storage[i])) + (j*4))->uvalue = jump_table[j];
      }
}
