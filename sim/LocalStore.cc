#include "LocalStore.h"
#include "IssueUnit.h"
#include <stdlib.h> // for gcc (exit)

#define LOCAL_SIZE 16384

LocalStore::LocalStore(int _latency, int _width) :
  FunctionalUnit(_latency){
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
      op == Instruction::lbu ||
      op == Instruction::lbui ||
      op == Instruction::lhui ||
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
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & BITMASK;
  } 
  // check for halfword loads
  else if (ins.op == Instruction::lhui) {
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & HALFMASK;
  }
  else {
    result.udata = ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue;
  }
  if (!thread->QueueWrite(write_reg, result, write_cycle, ins.op)) {
    // pipeline hazzard
    return false;
  }
  return true;
}

bool LocalStore::IssueStore(reg_value write_val, int address, ThreadState* thread, long long int write_cycle, Instruction& ins) {
  if (address < 0 || address > LOCAL_SIZE) {
    printf("Error Issuing store in LocalStore. 0x%8x not in [0x%8x, 0x%8x].\n", address, 0, LOCAL_SIZE);
    exit(1);
  }
  if (address < jtable_size) {
    printf("Error Issuing store in LocalStore. Address 0x%8x overwrites jump table entries (stack overflow)\n", address);
    exit(1);
  }
  static int max_addr = 0;
  if (address > max_addr) {
    max_addr = address;
  }
  // write write_val to mem[address] for thread
  if (ins.op == Instruction::sb || ins.op == Instruction::sbi) {
    ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue = 
      (((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & ~(BITMASK)) +
      (write_val.udata & BITMASK);
  } else if (ins.op == Instruction::sh || ins.op == Instruction::shi) {
    ((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue = 
      (((FourByte *)((char *)(storage[thread->thread_id]) + address))->uvalue & ~(HALFMASK)) +
      (write_val.udata & HALFMASK);
  } else {
    //     storage[thread->thread_id][address].uvalue = write_val.udata;
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
  case Instruction::lhui:
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

// load jump table in to low order words of stack ("top" of stack space)
void LocalStore::LoadJumpTable(std::vector<int> jump_table)
{
  jtable_size = jump_table.size() * 4; // convert from num words to num bytes
  for(int i=0; i < width; i++)
    for(size_t j=0; j < jump_table.size(); j++)
      {
	//storage[i][j].uvalue = jump_table[j];
	((FourByte *)((char *)(storage[i]) + (j*4)))->uvalue = jump_table[j];
      }
}

// load data segment (only ascii literals supported) to stack right after jump table
void LocalStore::LoadAsciiLiterals(std::vector<std::string> ascii_literals)
{
  // TODO: rename jtable_size to data_size

  int byteNum = 0;
  int wordNum = jtable_size / 4;
  FourByte tempWord;
  tempWord.uvalue = 0;

  for(int i=0; i < width; i++)
    {
      wordNum = jtable_size / 4;
      for(size_t j=0; j < ascii_literals.size(); j++)
	{
	  // <= length here so that we get the null terminating char
	  for(int k = 0; k <= ascii_literals.at(j).length(); k++)
	    {
	      // build a word out of up to 4 chars
	      unsigned letter = (unsigned)(ascii_literals.at(j)[k]);
	      int shift = byteNum * 8;
	      letter <<= shift;
	      tempWord.uvalue |= letter;
	      byteNum = (byteNum + 1) % 4;
	      
	      // Check if the word is full
	      if(byteNum == 0)
		{
		  ((FourByte *)((char *)(storage[i]) + (wordNum * 4)))->uvalue = tempWord.uvalue;
		  tempWord.uvalue = 0;
		  wordNum++;
		}
	    }
	}
      // Check if we ended mid-word, write the remaining bytes
      if(byteNum != 0)
	{
	  ((FourByte *)((char *)(storage[i]) + (wordNum * 4)))->uvalue = tempWord.uvalue;      
	}
    }
  
  // Add size of string literals to protected space size
  for(size_t i = 0; i < ascii_literals.size(); i++)
    jtable_size += ascii_literals.at(i).length() + 1;
  
}
