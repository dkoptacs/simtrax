#include "Instruction.h"
#include "WriteRequest.h"
#include <stdlib.h>

Instruction::Instruction(Opcode code,
                         int arg0, int arg1, int arg2,
                         int pc_addr) {
  op = code;
  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  pc_address = pc_addr;
  id = 0;
  depends[0] = depends[1] = -1;
}

Instruction::Instruction(const Instruction& ins) {
  op = ins.op;
  for (int i = 0; i < 3; i++) {
    args[i] = ins.args[i];
  }
  id = ins.id;
  depends[0] = ins.depends[0];
  depends[1] = ins.depends[1];
}

bool Instruction::RayReady(int ray_start, long long int* writes_in_flight, int kNoBlock) const {
  // check all origin and direction
  // A ray is Origin, Direction, Inverse Direction, tmin, tmax, object_id, normal, texture_uv = 14 elements
  const int num_elements = 17;
  for (int i = 0; i < num_elements; i++) {
    if (writes_in_flight[ray_start + i] != kNoBlock)
      return false;
  }
  return true;
}

bool Instruction::ReadyToIssue(long long int* register_ready, int* fail_reg, long long int cur_cycle) const {
  // This function needs to check if the registers that would be read by 
  // the given op would be ready by the given cycle
  const long long int kNoBlock = 0;
  switch (op) {
  case Instruction::ADD:
  case Instruction::SUB:
  case Instruction::MUL:
  case Instruction::BITOR:
  case Instruction::BITXOR:
  case Instruction::BITAND:
  case Instruction::BITSLEFT:
  case Instruction::BITSRIGHT:
  case Instruction::ANDN:
  case Instruction::FPADD:
  case Instruction::FPSUB:
  case Instruction::FPRSUB:
  case Instruction::FPMUL:
  case Instruction::FPDIV:
  case Instruction::DIV:
  case Instruction::FPUN:
  case Instruction::FPCMPLT:
  case Instruction::FPMIN:
  case Instruction::FPMAX:
  case Instruction::MOVINDRD:
  case Instruction::MOVINDWR:
  case Instruction::FPEQ:
  case Instruction::FPNE:
  case Instruction::FPLT:
  case Instruction::FPLE:
  case Instruction::FPGT:
  case Instruction::FPGE:
  case Instruction::EQ:
  case Instruction::NE:
  case Instruction::LT:
  case Instruction::LE:
  case Instruction::ADDK:
  case Instruction::RSUB:
  case Instruction::CMP:
  case Instruction::CMPU:
  case Instruction::LW:
  case Instruction::lbu:
    // check args[1] and args[2] and choose the first fail_reg if there is a fail.
    if ( (register_ready[args[1]] <= cur_cycle) )
      if ((register_ready[args[2]] <= cur_cycle))
	return true;
      else
	*fail_reg = args[2];
    else
      *fail_reg = args[1];
    return false;
    break;
  case Instruction::STRSIZE:
  case Instruction::STRSCHED:
  case Instruction::FPCONV:
  case Instruction::INTCONV:
  case Instruction::FPSQRT:
  case Instruction::FPNEG:
  case Instruction::FPINVSQRT:
  case Instruction::FPINV:
  case Instruction::LOAD:
  case Instruction::LOADIMM:
  case Instruction::LOADL1:
  case Instruction::MOV:
  case Instruction::sra:
  case Instruction::srl:
  case Instruction::bslli:
  case Instruction::bsrli:
  case Instruction::bsrai:
  case Instruction::brld:
  case Instruction::brald:
  case Instruction::braid:
  case Instruction::bralid:
  case Instruction::brk:
  case Instruction::ADDI:
  case Instruction::ANDI:
  case Instruction::ORI:
  case Instruction::MULI:
  case Instruction::RSUBI:
  case Instruction::LWI:
  case Instruction::lbui:
  case Instruction::lhui:
    // check args[1] and select it if it fails
    if ( register_ready[args[1]] <= cur_cycle )
      return true;
    else {
      *fail_reg = args[1];
      return false;
    }
    break;
  case Instruction::SW:
  case Instruction::sb:
  case Instruction::sh:
    // check args[0], args[1] and args[2] and choose the first fail_reg if there is a fail.
    if ((register_ready[args[0]] <= cur_cycle)) 
      if ((register_ready[args[1]] <= cur_cycle))
	if ((register_ready[args[2]] <= cur_cycle))
	  return true;
	else
	  *fail_reg = args[2];
      else
	*fail_reg = args[1];
    else
      *fail_reg = args[0];
    return false;
    break;
  case Instruction::rtsd:
  case Instruction::BNZ:
  case Instruction::JMPREG:
  case Instruction::brd:
  case Instruction::brad:
  case Instruction::beqid:
  case Instruction::bgeid:
  case Instruction::bgtid:
  case Instruction::bleid:
  case Instruction::bltid:
  case Instruction::bneid:
  case Instruction::STARTSW:
  case Instruction::STREAMW:
  case Instruction::SETSTRID:
    // check args[0] for read
    if ( (register_ready[args[0]] <= cur_cycle)) {
      return true;
    } else {
      *fail_reg = args[0];
      return false;
    }
    break;
  case Instruction::BLT:
  case Instruction::BET:
  case Instruction::STORE:
  case Instruction::beqd:
  case Instruction::bged:
  case Instruction::bgtd:
  case Instruction::bled:
  case Instruction::bltd:
  case Instruction::bned:
  case Instruction::SWI:
  case Instruction::sbi:
  case Instruction::shi:
    // check args[0] and args[1] for read
    if (register_ready[args[0]] <= cur_cycle) 
      if (register_ready[args[1]] <= cur_cycle)
	return true;
      else
	*fail_reg = args[1];
    else
      *fail_reg = args[0];
    return false;
    break;
  case Instruction::SPHERE_TEST:
    // this one doesn't deal with registers ready or not
    if (!RayReady(args[2], register_ready, kNoBlock))
      return false;
    // Check sphere ready
    // Sphere is center (3), radius (1), id (1)
    for (int i = 0; i < 5; i++) {
      if (register_ready[args[1] + i] > cur_cycle) {
	*fail_reg = args[1]+i;
	return false;
      }
    }
    return true;
    break;

    /*
  case Instruction::TRITEST:
    if (!RayReady(args[2], register_ready, kNoBlock))
      return false;
    // Check triangle ready
    // Triangle is v0, v1, v2 (3 each), id (1)
    for (int i = 0; i < 10; i++) {
      if (register_ready[args[1] + i] > cur_cycle) {
	*fail_reg = args[1]+i;
        return false;
      }
    }
    return true;
    break;
    */

  case Instruction::BOXTEST:
    // TODO: this isn't very clean, assuming box registers are 36 - 47
    for(int i = 36; i < 48; i++)
      if (register_ready[i] > cur_cycle) 
	{
	  *fail_reg = i;
	  return false;
	}
    return true;
    break;

  case Instruction::TRITEST:
    // TODO: this isn't very clean, assuming tri registers are 36 - 50
    for(int i = 36; i < 51; i++)
      if (register_ready[i] > cur_cycle) 
	{
	  *fail_reg = i;
	  return false;
	}
    return true;
    break;

  case Instruction::PRINT:
    if (register_ready[args[0]] <= cur_cycle)
      return true;
    else
      *fail_reg = args[0];
    return false;
    break;

    //TODO: these global atomic instructions are always ready to issue (this is wrong though)
  case Instruction::ATOMIC_INC:
  case Instruction::ATOMIC_ADD:
  case Instruction::ATOMIC_FPADD:
  case Instruction::INC_RESET:
  case Instruction::BARRIER:
  case Instruction::GLOBAL_READ:
    return true;
    break;

    // These instructions don't read normal registers, always ready to issue
  case Instruction::ENDSW:
  case Instruction::ENDSR:
  case Instruction::STARTSR:
  case Instruction::STREAMR:
  case Instruction::brlid:
  case Instruction::brid:
  case Instruction::NOP:
  case Instruction::PROF:
  case Instruction::RAND:
  case Instruction::HALT:
  case Instruction::SETBOXPIPE:
  case Instruction::SETTRIPIPE:
  case Instruction::LOADPIPEGLB:
  case Instruction::LOADPIPELOC:
    return true;
    break;
  default:
    printf("Error: Instruction opcode %d has no ReadyToIssue definition\n", op);
    printf("%s\n", Instruction::Opnames[op].c_str());
    exit(1);
    break;
  };

  printf("Error, Instruction::ReadyToIssue fell through switch\n");
  exit(1);
  return false;
}

void Instruction::print() {
  printf("%d: %s %d %d %d",
	 pc_address,
         Instruction::Opnames[op].c_str(),
         args[0], args[1], args[2]);
}

InstructionRecord::InstructionRecord(Instruction& ins,
                                     long long int cycle,
                                     IssueUnit* issue,
                                     ThreadState* thread, 
				     bool _cache_hit) :
  cycle_arrived(cycle), instruction(ins), issuer(issue), thread(thread),
  cache_hit(_cache_hit) {
}

InstructionRecord::InstructionRecord(const InstructionRecord& record) :
  cycle_arrived(record.cycle_arrived), instruction(record.instruction),
  issuer(record.issuer), thread(record.thread),
  write_requests(record.write_requests)
{
  for (int i = 0; i < 32; i++) {
    ivalues[i] = record.ivalues[i];
  }
}

InstructionQueue::InstructionQueue() {
}

void InstructionQueue::Print() const {
  for (size_t i = 0; i < instructions.size(); i++) {
    printf("Queued Instruction %d arrived in cycle %lld\n",
           int(i), instructions[i]->cycle_arrived);
  }
}
// Surprisingly this is identical to Ready (ready just checks that you
// have a different cycle...)
int InstructionQueue::TopMatches(long long int cycle, int num_check) const {
  int num_found = 0;
  int min_check = std::min(num_check, int(instructions.size()));
  for (int i = 0; i < min_check; i++) {
    int front_cycle = instructions[i]->cycle_arrived;
    if (front_cycle == cycle)
      num_found++;
  }
  return num_found;
}

int InstructionQueue::LastMatches(long long int cycle, int num_check) const {
  int num_found = 0;
  int num_instructions = int(instructions.size());
  int start_check = std::max(0, num_instructions - num_check);
  for (int i = start_check; i < num_instructions; i++) {
    int front_cycle = instructions[i]->cycle_arrived;
    if (front_cycle == cycle)
      num_found++;
  }
  return num_found;
}

bool InstructionQueue::Ready(long long int required_cycle) const {
  return TopMatches(required_cycle, 1) == 1;
}

InstructionRecord* InstructionQueue::Pop() {
  InstructionRecord* result = NULL;
  if (instructions.size()) {
    result = instructions.front();
    instructions.pop_front();
  }
  return result;
}

void InstructionQueue::Push(InstructionRecord* record) {
  instructions.push_back(record);
}

int InstructionQueue::Size() const {
  return static_cast<int>(instructions.size());
}

InstructionPriorityQueue::InstructionPriorityQueue() {
}

void InstructionPriorityQueue::Push(InstructionRecord* record) {
  if (instructions.empty()) {
    instructions.push_back(record);
  }
  else {
    // find the insert position
    std::deque<InstructionRecord*>::iterator pos = instructions.end();
    for (std::deque<InstructionRecord*>::iterator i = instructions.begin();
	 i != instructions.end(); ++i) {
      if (record->cycle_arrived < (*i)->cycle_arrived) {
	pos = i;
	break;
      }
    }
    instructions.insert(pos, record);
  }
}

std::string Instruction::Opnames[NUM_OPS] = {
  std::string("ADD"),
  std::string("SUB"),
  std::string("MUL"),
  std::string("BITOR"),
  std::string("BITAND"),
  std::string("BITSLEFT"),
  std::string("BITSRIGHT"),
  std::string("FPADD"),
  std::string("FPSUB"),
  std::string("FPMUL"),
  std::string("FPCMPLT"),
  std::string("FPMIN"),
  std::string("FPMAX"),
  std::string("LOAD"),
  std::string("INTCONV"),
  std::string("ATOMIC_INC"),
  std::string("INC_RESET"),
  std::string("BARRIER"),
  std::string("GLOBAL_READ"),
  std::string("ATOMIC_ADD"),
  std::string("ATOMIC_FPADD"),
  std::string("FPINVSQRT"),
  std::string("FPINV"),
  std::string("FPCONV"),

  // Stream ops
  std::string("STARTSW"), 
  std::string("STREAMW"), 
  std::string("ENDSW"),
  std::string("STARTSR"), 
  std::string("STREAMR"), 
  std::string("ENDSR"),
  std::string("STRSIZE"), 
  std::string("STRSCHED"),
  std::string("SETSTRID"),
  std::string("GETSTRID"),

  // Chained FU pipeline ops
  std::string("BOXTEST"),
  std::string("TRITEST"),
  std::string("SETBOXPIPE"),
  std::string("SETTRIPIPE"),
  std::string("LOADPIPEGLB"),
  std::string("LOADPIPELOC"),

  std::string("FPEQ"),
  std::string("FPNE"),
  std::string("FPLT"),
  std::string("FPLE"),
  std::string("EQ"),  
  std::string("NE"),  
  std::string("LT"),  
  std::string("LE"),  
  std::string("BNZ"), 
  std::string("LOADL1"),
  std::string("STORE"),
  std::string("LOADIMM"),
  std::string("SPHERE_TEST"),
  //std::string("TRITEST"),
  std::string("MOV"),
  std::string("MOVINDRD"),
  std::string("MOVINDWR"),
  std::string("BLT"),
  std::string("BET"),
  std::string("JMP"),
  std::string("JMPREG"),
  std::string("JAL"),
  std::string("RAND"),
  std::string("COS"),
  std::string("SIN"),

  // MBlaze stuff
  std::string("ADDC"),
  std::string("ADDK"),
  std::string("ADDKC"),
  std::string("BITXOR"), //
  std::string("ANDN"), //
  std::string("CMP"), //
  std::string("CMPU"),
  // maybe remove/switch these to SUBs
  std::string("RSUB"), //
  std::string("RSUBC"),
  std::string("RSUBK"),
  std::string("RSUBKC"),
  // continue normal
  std::string("MULH"),
  std::string("MULHU"),
  std::string("sra"),
  std::string("srl"),
  // immediates
  std::string("ADDI"), //
  std::string("ADDIC"),
  std::string("ADDIK"),
  std::string("ADDIKC"),
  std::string("RSUBI"), //
  std::string("RSUBIC"),
  std::string("RSUBIK"),
  std::string("RSUBIKC"),
  std::string("ANDNI"), //
  std::string("ANDI"), //
  std::string("ORI"), //
  std::string("XORI"), //
  std::string("MULI"), //
  // local read/writes
  std::string("LW"), //
  std::string("LWI"), //
  std::string("lbu"),
  std::string("lbui"),
  std::string("lhui"),
  std::string("SW"), //
  std::string("SWI"), //
  std::string("sh"),
  std::string("shi"),
  std::string("sb"),
  std::string("sbi"),
  // MBlaze branches
  std::string("beqd"),
  std::string("beqid"),
  std::string("bged"),
  std::string("bgeid"),
  std::string("bgtd"),
  std::string("bgtid"),
  std::string("bled"),
  std::string("bleid"),
  std::string("bltd"),
  std::string("bltid"),
  std::string("bned"),
  std::string("bneid"),
  std::string("brd"), //
  std::string("brad"), //
  std::string("brld"), //
  std::string("brald"), //
  std::string("brid"), //
  std::string("braid"), //
  std::string("brlid"), //
  std::string("bralid"), //
  std::string("brk"), //
  std::string("brki"), //
  std::string("rtsd"),
  std::string("bslli"), // barrel shift left logical immediate
  std::string("bsrli"), // barrel shift right logical immediate
  std::string("bsrai"), // barrel shift right arithmetical immediate

  // End MBlaze stuff
  std::string("FPDIV"),
  std::string("DIV"),
  std::string("FPUN"),
  std::string("FPRSUB"),
  std::string("FPSQRT"),
  std::string("FPNEG"),
  std::string("FPGT"),
  std::string("FPGE"),

  std::string("SLEEP"),
  std::string("SYNC"),
  std::string("NOP"),
  std::string("HALT"),
  std::string("PRINT"),
  std::string("PROF"),
};

