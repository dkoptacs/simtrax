#include "Instruction.h"
#include "WriteRequest.h"
#include "SimpleRegisterFile.h"
#include <stdlib.h>

Instruction::Instruction(Opcode code,
                         int arg0, int arg1, int arg2,
                         int pc_addr)
{
  op = code;
  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  pc_address = pc_addr;
  id = 0;
  depends[0] = depends[1] = -1;
  executions = 0;
  data_stalls = 0;

  srcInfo.lineNum = -1;
  srcInfo.colNum = -1;
  srcInfo.fileNum = -1;

}

Instruction::Instruction(Opcode code,
                         int arg0, int arg1, int arg2,
                         SourceInfo _srcInfo,
                         int pc_addr)
{
  op = code;
  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  pc_address = pc_addr;
  id = 0;
  depends[0] = depends[1] = -1;
  executions = 0;
  data_stalls = 0;

  srcInfo = _srcInfo;
}

Instruction::Instruction(const Instruction& ins)
{
  op = ins.op;

  for (int i = 0; i < 3; i++)
    args[i] = ins.args[i];

  id = ins.id;
  depends[0] = ins.depends[0];
  depends[1] = ins.depends[1];
  executions = ins.executions;
  data_stalls = ins.data_stalls;

  srcInfo = ins.srcInfo;

}

bool Instruction::RayReady(int ray_start, long long int* writes_in_flight, int kNoBlock) const
{
  // check all origin and direction
  // A ray is Origin, Direction, Inverse Direction, tmin, tmax, object_id, normal, texture_uv = 14 elements
  const int num_elements = 17;

  for (int i = 0; i < num_elements; i++)
  {
    if (writes_in_flight[ray_start + i] != kNoBlock)
      return false;
  }

  return true;
}

bool Instruction::ReadyToIssue(long long int* register_ready, int* fail_reg, long long int cur_cycle) const
{
  // This function needs to check if the registers that would be read by
  // the given op would be ready by the given cycle
  const long long int kNoBlock = 0;
  switch (op)
  {
    case Instruction::lb:
    case Instruction::lbu:
    case Instruction::lh:
    case Instruction::lhu:
    case Instruction::lw:
    case Instruction::lwc1:
    case Instruction::FTOD:
      // check args[2] and choose the first fail_reg if there is a fail.

      if ((register_ready[args[2]] <= cur_cycle))
        return true;
      else
        *fail_reg = args[2];

      return false;
      break;

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
    case Instruction::ADDKC:
    case Instruction::RSUB:
    case Instruction::CMP:
    case Instruction::CMPU:
    case Instruction::LW:
    case Instruction::bsrl:
    case Instruction::bsra:
    case Instruction::bsll:
    case Instruction::addu:

    // MIPS
    case Instruction::add_s:
    case Instruction::and_m:
    case Instruction::div_s:
    case Instruction::movn:
    case Instruction::movn_s:
    case Instruction::movz_s:
    case Instruction::mul:
    case Instruction::mul_s:
    case Instruction::nor:
    case Instruction::or_m:
    case Instruction::slt:
    case Instruction::sltu:
    case Instruction::subu:
    case Instruction::sub_s:
    case Instruction::xor_m:
    case Instruction::sllv:
    case Instruction::srav:
    case Instruction::srlv:

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
    case Instruction::neg_s:
    case Instruction::negu:
    case Instruction::FPINVSQRT:
    case Instruction::FPINV:
    case Instruction::LOAD:
    case Instruction::LOADIMM:
    case Instruction::LOADL1:
    case Instruction::MOV:
    case Instruction::move:
    case Instruction::PROF:
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
    case Instruction::XORI:
    case Instruction::MULI:
    case Instruction::RSUBI:
    case Instruction::LWI:
    case Instruction::lbui:
    case Instruction::lhui:
    case Instruction::sext8:

    // MIPS
    case Instruction::addi:
    case Instruction::not_m:
    case Instruction::addiu:
    case Instruction::andi:
    case Instruction::cvt_s_w:
    case Instruction::mfc1:
    case Instruction::movf:
    case Instruction::movt:
    case Instruction::movt_s:
    case Instruction::mov_s:
    case Instruction::ori:
    case Instruction::sll:
    case Instruction::slti:
    case Instruction::sltiu:
    case Instruction::sra_m:
    case Instruction::srl_m:
    case Instruction::trunc_w_s:

      // check args[1] and select it if it fails
      if ( register_ready[args[1]] <= cur_cycle )
        return true;
      else {
        *fail_reg = args[1];
        return false;
      }
      break;

    case Instruction::SW:
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
    case Instruction::SEM_ACQ:
    case Instruction::SEM_REL:

    case Instruction::bgez:
    case Instruction::bgtz:
    case Instruction::blez:
    case Instruction::bltz:
    case Instruction::beqz:
    case Instruction::bnez:
    case Instruction::jr:
    case Instruction::mtc1:

      // check args[0] for read
      if ( (register_ready[args[0]] <= cur_cycle))
      {
        return true;
      }
      else
      {
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

    // MIPS
    case Instruction::beq:
    case Instruction::bne:
    case Instruction::c_eq_s:
    case Instruction::c_ole_s:
    case Instruction::c_olt_s:
    case Instruction::c_ule_s:
    case Instruction::c_ult_s:
    case Instruction::div:
    case Instruction::teq:
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

    // MIPS!
    case Instruction::sw:
    case Instruction::swc1:
    case Instruction::lwl:
    case Instruction::lwr:
    case Instruction::swl:
    case Instruction::swr:
    case Instruction::sb:
    case Instruction::sh:
      // check args[0] and args[2] for read
      if (register_ready[args[0]] <= cur_cycle)
        if (register_ready[args[2]] <= cur_cycle)
          return true;
        else
          *fail_reg = args[2];
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
      for (int i = 0; i < 5; i++)
      {
        if (register_ready[args[1] + i] > cur_cycle)
        {
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
      {
        if (register_ready[i] > cur_cycle)
        {
          *fail_reg = i;
          return false;
        }
      }

      return true;
      break;

    case Instruction::TRITEST:
      // TODO: this isn't very clean, assuming tri registers are 36 - 50
      for(int i = 36; i < 51; i++)
      {
        if (register_ready[i] > cur_cycle)
        {
          *fail_reg = i;
          return false;
        }
      }

      return true;
      break;

    case Instruction::PRINT:
    case Instruction::PRINTF:
      if (register_ready[args[0]] <= cur_cycle)
        return true;
      else
        *fail_reg = args[0];
      return false;
      break;

      // Read special mips registers
    case Instruction::mfhi:
      if (register_ready[HI_REG] <= cur_cycle)
        return true;
      else
        *fail_reg = HI_REG;

      return false;
      break;

    case Instruction::mflo:
      if (register_ready[LO_REG] <= cur_cycle)
        return true;
      else
        *fail_reg = LO_REG;
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
    case Instruction::RAND:
    case Instruction::HALT:
    case Instruction::SETBOXPIPE:
    case Instruction::SETTRIPIPE:
    case Instruction::LOADPIPEGLB:
    case Instruction::LOADPIPELOC:

    // MIPS
    case Instruction::nop:
    case Instruction::bal:
    case Instruction::bc1f:
    case Instruction::bc1t:
    case Instruction::j:
    case Instruction::jal:
    case Instruction::lui:
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

void Instruction::print()
{
  printf("%d: %s %d %d %d",
         pc_address,
         Instruction::Opnames[op].c_str(),
         args[0], args[1], args[2]);
}

InstructionRecord::InstructionRecord(Instruction& ins,
                                     long long int cycle,
                                     IssueUnit* issue,
                                     ThreadState* thread,
                                     bool _cache_hit)
    : cycle_arrived(cycle),
      instruction(ins),
      issuer(issue),
      thread(thread),
      cache_hit(_cache_hit)
{
}

InstructionRecord::InstructionRecord(const InstructionRecord& record)
    : cycle_arrived(record.cycle_arrived),
      instruction(record.instruction),
      issuer(record.issuer),
      thread(record.thread),
      write_requests(record.write_requests)
{
  for (int i = 0; i < 32; i++)
    ivalues[i] = record.ivalues[i];
}

InstructionQueue::InstructionQueue()
{
}

void InstructionQueue::Print() const
{
  for (size_t i = 0; i < instructions.size(); i++)
  {
    printf("Queued Instruction %d arrived in cycle %lld\n",
           int(i), instructions[i]->cycle_arrived);
  }
}
// Surprisingly this is identical to Ready (ready just checks that you
// have a different cycle...)
int InstructionQueue::TopMatches(long long int cycle, int num_check) const
{
  int num_found = 0;
  int min_check = std::min(num_check, int(instructions.size()));
  for (int i = 0; i < min_check; i++)
  {
    int front_cycle = instructions[i]->cycle_arrived;
    if (front_cycle == cycle)
      num_found++;
  }

  return num_found;
}

int InstructionQueue::LastMatches(long long int cycle, int num_check) const
{
  int num_found = 0;
  int num_instructions = int(instructions.size());
  int start_check = std::max(0, num_instructions - num_check);
  for (int i = start_check; i < num_instructions; i++)
  {
    int front_cycle = instructions[i]->cycle_arrived;
    if (front_cycle == cycle)
      num_found++;
  }

  return num_found;
}

bool InstructionQueue::Ready(long long int required_cycle) const
{
  return TopMatches(required_cycle, 1) == 1;
}

InstructionRecord* InstructionQueue::Pop()
{
  InstructionRecord* result = NULL;
  if (instructions.size())
  {
    result = instructions.front();
    instructions.pop_front();
  }
  return result;
}

void InstructionQueue::Push(InstructionRecord* record)
{
  instructions.push_back(record);
}

int InstructionQueue::Size() const
{
  return static_cast<int>(instructions.size());
}

InstructionPriorityQueue::InstructionPriorityQueue()
{
}

void InstructionPriorityQueue::Push(InstructionRecord* record)
{
  if (instructions.empty())
  {
    instructions.push_back(record);
  }
  else {
    // find the insert position
    std::deque<InstructionRecord*>::iterator pos = instructions.end();
    std::deque<InstructionRecord*>::iterator i;
    for (i = instructions.begin(); i != instructions.end(); ++i)
    {
      if (record->cycle_arrived < (*i)->cycle_arrived)
      {
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
  std::string("SEM_ACQ"),
  std::string("SEM_REL"),
  std::string("GLOBAL_READ"),
  std::string("ATOMIC_ADD"),
  std::string("ATOMIC_FPADD"),
  std::string("FPINVSQRT"),
  std::string("FPINV"),
  std::string("FPCONV"),
  std::string("FTOD"),

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
  std::string("move"),
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
  std::string("sext8"),
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
  std::string("lb"),
  std::string("lbu"),
  std::string("lbui"),
  std::string("lh"),
  std::string("lhu"),
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
  std::string("bsll"),  // barrel shift left logical
  std::string("bsrli"), // barrel shift right logical immediate
  std::string("bsrl"),  // barrel shift right logical
  std::string("bsrai"), // barrel shift right arithmetical immediate
  std::string("bsra"),  // barrel shift right arithmetical

  // End MBlaze stuff
  std::string("FPDIV"),
  std::string("DIV"),
  std::string("FPUN"),
  std::string("FPRSUB"),
  std::string("FPSQRT"),
  std::string("FPNEG"),
  std::string("neg_s"),
  std::string("negu"),
  std::string("FPGT"),
  std::string("FPGE"),

  std::string("SLEEP"),
  std::string("SYNC"),
  std::string("NOP"),
  std::string("HALT"),
  std::string("PRINT"),
  std::string("PRINTF"),
  std::string("PROF"),

  // MIPS
  std::string("addi"),
  std::string("addiu"),
  std::string("addu"),
  std::string("add_s"),
  std::string("andi"),
  std::string("and_m"),
  std::string("bal"),
  std::string("bc1f"),
  std::string("bc1t"),
  std::string("beq"),
  std::string("beqz"),
  std::string("bnez"),
  std::string("bgez"),
  std::string("bgtz"),
  std::string("blez"),
  std::string("bltz"),
  std::string("bne"),
  std::string("cvt_s_w"),
  std::string("c_eq_s"),
  std::string("c_ole_s"),
  std::string("c_olt_s"),
  std::string("c_ule_s"),
  std::string("c_ult_s"),
  std::string("div"),
  std::string("div_s"),
  std::string("lui"),
  std::string("lw"),
  std::string("lwc1"),
  std::string("lwl"),
  std::string("lwr"),
  std::string("mfc1"),
  std::string("mfhi"),
  std::string("mflo"),
  std::string("movf"),
  std::string("movn"),
  std::string("movt"),
  std::string("mov_s"),
  std::string("movn_s"),
  std::string("movz_s"),
  std::string("movt_s"),
  std::string("mtc1"),
  std::string("mul"),
  std::string("mul_s"),
  std::string("j"),
  std::string("jal"),
  std::string("jr"),
  std::string("nop"),
  std::string("nor"),
  std::string("not_m"),
  std::string("ori"),
  std::string("or_m"),
  std::string("sll"),
  std::string("sllv"),
  std::string("slt"),
  std::string("slti"),
  std::string("sltu"),
  std::string("sltiu"),
  std::string("sra_m"),
  std::string("srl_m"),
  std::string("srav"),
  std::string("srlv"),
  std::string("subu"),
  std::string("sub_s"),
  std::string("sw"),
  std::string("swl"),
  std::string("swr"),
  std::string("swc1"),
  std::string("teq"),
  std::string("trunc_w_s"),
  std::string("xor_m"),
  // End MIPS
};
