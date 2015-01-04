#include "LocalStore.h"
#include "IssueUnit.h"
#include <stdlib.h> // for gcc (exit)
//#include <string>

#define LOCAL_SIZE 16384

extern std::vector<std::string> source_names;

LocalStore::LocalStore(int _latency, int _width) :
    FunctionalUnit(_latency){

  // Hard-code area and energy since they are unlikely to change
  area = 0.01396329 * _width;
  energy = 0.00692867;

  issued_this_cycle = 0;
  width = _width;
  jtable_size = 0;

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
      op == Instruction::lb ||
      op == Instruction::lbu ||
      op == Instruction::lbui ||
      op == Instruction::lh ||
      op == Instruction::lhu ||
      op == Instruction::lhui ||
      op == Instruction::SWI ||
      op == Instruction::SW ||
      op == Instruction::sh ||
      op == Instruction::shi ||
      op == Instruction::sb ||
      op == Instruction::sbi ||
      op == Instruction::lw ||
      op == Instruction::lwl ||
      op == Instruction::lwr ||
      op == Instruction::sw ||
      op == Instruction::swl ||
      op == Instruction::swr ||
      op == Instruction::lwc1 ||
      op == Instruction::swc1)
    return true;
  else
    return false;
}

#define BITMASK 0x000000FF
#define HALFMASK 0x0000FFFF

bool LocalStore::IssueLoad(int write_reg, int address, ThreadState* thread, IssueUnit* issuer, long long int write_cycle, Instruction& ins) {
  reg_value result;
  if (address < 0 || address > LOCAL_SIZE) {
    printf("Error Issuing load in LocalStore. 0x%8x not in [0x%8x, 0x%8x].\n", address, 0, LOCAL_SIZE);
    printf("Instruction: %s (PC: %d)\n", Instruction::Opnames[ins.op].c_str(), ins.pc_address);
    if(ins.srcInfo.fileNum >= 0)
      printf("%s:%d\n", source_names[ins.srcInfo.fileNum].c_str(), ins.srcInfo.lineNum);
    else
      printf("Compile with -g for more info\n");
    exit(1);
  }
  // check for byte loads
  else if (ins.op == Instruction::lbu || ins.op == Instruction::lbui || ins.op == Instruction::lb) {
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & BITMASK;
  }
  // check for halfword loads
  else if (ins.op == Instruction::lhui || ins.op == Instruction::lh || ins.op == Instruction::lhu) {
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & HALFMASK;
  }
  else {
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue;
  }

  // Sign extend/mask
  if(ins.op == Instruction::lb)
    result.idata = ((result.idata << 24) >> 24);
  if(ins.op == Instruction::lh)
    result.idata = ((result.idata << 16) >> 16);
  if(ins.op == Instruction::lwl)
    result = LoadWordLeft(thread, address, write_reg, issuer->current_cycle);
  if(ins.op == Instruction::lwr)
    result = LoadWordRight(thread, address, write_reg, issuer->current_cycle);


  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op, &ins)) {
    // pipeline hazzard
    return false;
  }
  return true;
}

bool LocalStore::IssueStore(reg_value write_val, int address, ThreadState* thread, long long int write_cycle, Instruction& ins) {
  if (address < 0 || address > LOCAL_SIZE) {
    printf("Error Issuing store in LocalStore. 0x%8x not in [0x%8x, 0x%8x].\n", address, 0, LOCAL_SIZE);
    printf("Instruction: %s (PC: %d)\n", Instruction::Opnames[ins.op].c_str(), ins.pc_address);
    if(ins.srcInfo.fileNum >= 0)
      printf("%s:%d\n", source_names[ins.srcInfo.fileNum].c_str(), ins.srcInfo.lineNum);
    else
      printf("Compile with -g for more info\n");
    exit(1);
  }
  
  // TODO: Arrange globals differently since they can be written. Currently globals go in the data segment, so we can't
  //       perform this check below
  /*if (address < jtable_size) {
    printf("Error issuing store in LocalStore. Address 0x%8x overwrites data segment (stack overflow)\n", address);
    if(ins.srcInfo.fileNum >= 0)
      printf("%s:%d\n", source_names[ins.srcInfo.fileNum].c_str(), ins.srcInfo.lineNum);
    exit(1);
    }*/
  
  static int max_addr = 0;
  if (address > max_addr) 
    max_addr = address;

  if(ins.op == Instruction::swl)
    StoreWordLeft(thread, address, write_val);
  else if(ins.op == Instruction::swr)
    StoreWordRight(thread, address, write_val);
  // write write_val to mem[address] for thread
  else if (ins.op == Instruction::sb || ins.op == Instruction::sbi) 
    {
      ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue =
        (((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & ~(BITMASK)) +
        (write_val.udata & BITMASK);
    } 
  else if (ins.op == Instruction::sh || ins.op == Instruction::shi) 
    {
      ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue =
        (((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & ~(HALFMASK)) +
        (write_val.udata & HALFMASK);
    } 
  else 
    {
      ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue = write_val.udata;
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
  switch (ins.op) 
    {
      
    case Instruction::lb:
    case Instruction::lbu:
    case Instruction::lh:
    case Instruction::lhu:
    case Instruction::lw:
    case Instruction::lwl:
    case Instruction::lwr:
    case Instruction::lwc1:
      if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }

      address = arg2.udata + ins.args[1];
      
      write_reg = ins.args[0];
      if (!IssueLoad(write_reg, address, thread, issuer, write_cycle, ins))
        return false;
      break;



    case Instruction::sb:
    case Instruction::sh:
    case Instruction::swc1:
    case Instruction::sw:
    case Instruction::swl:
    case Instruction::swr:
      if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg1, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      
      address = arg1.udata + ins.args[1];
      if (!IssueStore(arg0, address, thread, write_cycle, ins))
        return false;
      break;
    
    case Instruction::LW:
      if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // bad times
        printf("Error in LocalStore.\n");
      }
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      address = arg1.idata + arg2.idata;
      write_reg = ins.args[0];
      if (!IssueLoad(write_reg, address, thread, issuer, write_cycle, ins))
        return false;
      break;
      
    case Instruction::LWI:
    case Instruction::lbui:
    case Instruction::lhui:
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      address = arg1.idata + ins.args[2];
      write_reg = ins.args[0];
      if (!IssueLoad(write_reg, address, thread, issuer, write_cycle, ins))
        return false;
      break;

      /*
    case Instruction::lw:
      if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      
      address = arg2.idata + ins.args[1];
      write_reg = ins.args[0];
      if (!IssueLoad(write_reg, address, thread, write_cycle, ins))
        return false;
      break;
      */
      /*
    case Instruction::sw:
      if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      
      address = arg2.idata + ins.args[1];
      if (!IssueStore(arg0, address, thread, write_cycle, ins))
        return false;
      break;
      */

    case Instruction::SWI:
    case Instruction::sbi:
    case Instruction::shi:
      if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      address = arg1.idata + ins.args[2];
      if (!IssueStore(arg0, address, thread, write_cycle, ins))
        return false;
      break;
      
    case Instruction::SW:
      if (!thread->ReadRegister(ins.args[0], issuer->current_cycle, arg0, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      if (!thread->ReadRegister(ins.args[1], issuer->current_cycle, arg1, failop))
      {
        // something bad
        printf("Error in LocalStore.\n");
      }
      if (!thread->ReadRegister(ins.args[2], issuer->current_cycle, arg2, failop))
      {
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

// load jump table in to low order words of stack ("top" of stack space)
void LocalStore::LoadJumpTable(char* jump_table, int _size)
{
  jtable_size = _size;
  for(int i=0; i < width; i++)
    memcpy(storage[i], jump_table, jtable_size);
}

// Handle masking for a load word left instruction
reg_value LocalStore::LoadWordLeft(ThreadState* thread, int address, int write_reg, long long int current_cycle)
{
  // lwl must not change some of the previous contents of the dst register, so we must first read it
  reg_value result;
  reg_value oldVal;
  Instruction::Opcode failop;
  if (!thread->ReadRegister(write_reg, current_cycle, oldVal, failop))
    {
      printf("Error in LocalStore.\n");
      exit(1);
    }
  
  // address is most significant of 4 consecutive bytes
  int wordBoundary = address - (address % 4); // find the aligned word boundary
  result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + wordBoundary))->uvalue;

  unsigned int discardBits = (3 - (address - wordBoundary)) * 8;
  unsigned int regMask = ~(0xFFFFFFFFu << discardBits);
  // Discard most significant bytes from loaded data, promote remaining bytes to MS
  result.udata = result.udata << discardBits;
  // Keep least significant bytes from register data
  result.udata |= (oldVal.udata & regMask); 
  return result;
}

// Handle masking for a load word right instruction
reg_value LocalStore::LoadWordRight(ThreadState* thread, int address, int write_reg, long long int current_cycle)
{
  // lwr must not change some of the previous contents of the dst register, so we must first read it
  reg_value result;
  reg_value oldVal;
  Instruction::Opcode failop;
  if (!thread->ReadRegister(write_reg, current_cycle, oldVal, failop))
    {
      printf("Error in LocalStore.\n");
      exit(1);
    }
  
  int wordBoundary = address - (address % 4); // find the aligned word boundary
  result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + wordBoundary))->uvalue;

  // mask off any bytes after the word boundary
  unsigned int discardBits = (address - wordBoundary) * 8;
  unsigned int regMask = ~(0xFFFFFFFFu >> discardBits);
  // Discard least significant bytes from loaded data, demote remaining bytes to LS
  result.udata = result.udata >> discardBits;
  // Keep most significant bytes from register data
  result.udata |= (oldVal.udata & regMask); 
  return result;
}


void LocalStore::StoreWordLeft(ThreadState* thread, int address, reg_value write_val)
{
  reg_value result;
  reg_value oldVal;
  // address is most significant of 4 consecutive bytes
  int wordBoundary = address - (address % 4); // find the aligned word boundary
  oldVal.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + wordBoundary))->uvalue;

  unsigned int discardBits = (3 - (address - wordBoundary)) * 8;
  unsigned int memMask = ~(0xFFFFFFFFu >> discardBits);
  // Shift register data to proper word alignment
  write_val.udata = write_val.udata >> discardBits;
  // Keep remaining bytes from memory data
  oldVal.udata &= memMask; 

  // Combine the two
  result.udata = write_val.udata | oldVal.udata;
  // Store it
  ((FourByte *)((char *)(storage[thread->thread_id]) + wordBoundary))->uvalue = result.udata;

}

void LocalStore::StoreWordRight(ThreadState* thread, int address, reg_value write_val)
{
  reg_value result;
  reg_value oldVal;
  // address is most significant of 4 consecutive bytes
  int wordBoundary = address - (address % 4); // find the aligned word boundary
  oldVal.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + wordBoundary))->uvalue;

  // mask off any bytes after the word boundary
  unsigned int discardBits = (address - wordBoundary) * 8;
  unsigned int memMask = ~(0xFFFFFFFFu << discardBits);
  // Shift register data to word alignment
  write_val.udata = write_val.udata << discardBits;
  // Keep remaining bytes from memory data
  oldVal.udata &= memMask;

    // Combine the two
  result.udata = write_val.udata | oldVal.udata;
  // Store it
  ((FourByte *)((char *)(storage[thread->thread_id]) + wordBoundary))->uvalue = result.udata;

}
