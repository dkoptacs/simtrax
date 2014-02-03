#ifndef _SIMHWRT_INSTRUCTION_H_
#define _SIMHWRT_INSTRUCTION_H_

#include <stdio.h>
// An Instruction is an Opcode
// along with:
// 1 dest register and 2 source registers (ADD/SUB, MUL)
// 1 dest register and 1 source register mov, fconv)
// 1 dest register and 1 immediate value (load_immediate)
// 1 dest, 1, source, 1 immediate (load)
// no dest, 2 source and 1 immediate (BLT, BET)
// no dest, 2 source, 1 immediate (Store)
// so always three ints ;)

// Conventions are:
// dest, source0, source1
// dest, source, IGNORE
// dest, imm, IGNORE
// source0, source1, imm

#include <deque>
#include <string>
#include <vector>

// profiling arguments
#define BEGIN_COUNT_CYCLES 0
#define END_COUNT_CYCLES 1

class Instruction {
public:
  int pc_address;
  long long int id;
  long long int depends[2];
  // When adding a new Opcode don't forget the string in Instruction.cc
  enum Opcode {
    ADD = 0,     // dst, source1, source2
    SUB,         // dst, source1, source2
    MUL,         // dst, source1, source2
    BITOR,       // dst, source1, source2
    BITAND,      // dst, source1, source2
    BITSLEFT,    // dst, source1, source2
    BITSRIGHT,    // dst, source1, source2
    FPADD,       // dst, source1, source2
    FPSUB,       // dst, source1, source2
    FPMUL,       // dst, source1, source2
    FPCMPLT,     // dst, source1, source2
    FPMIN,       // dst, source1, source2
    FPMAX,       // dst, source1, source2
    LOAD,        // dst, addr(reg), offset(imm)
    // end 14 arithmetic ops
    INTCONV,     // dst, source (float->int)
    ATOMIC_INC,  // dst, source(global), offset(imm -- ignored)
    INC_RESET,   // dst, source(global)
    BARRIER,     // source(global)
    GLOBAL_READ, // dest, source(global)
    ATOMIC_ADD,  // dst, source1(global), source2 (dst = source1 += source2)
    ATOMIC_FPADD, // addr(reg), source, offset(imm)
    FPINVSQRT,   // dst, source
    FPINV,       // dst, source
    FPCONV,      // dst, source (int->float)

    // Stream queue ops
    STARTSW,     // source 
    STREAMW,     // source
    ENDSW,
    STARTSR,     // dst
    STREAMR,     // dst
    ENDSR,
    STRSIZE,     // dst, source
    STRSCHED,    // dst, source
    SETSTRID,    // source
    GETSTRID,    // dst

    // Chained FU pipeline ops
    BOXTEST,     // dst
    TRITEST,     // dst
    SETBOXPIPE,  // void
    SETTRIPIPE,  // void
    // load special pipeline input registers
    LOADPIPEGLB, // dst, source (global address)
    LOADPIPELOC, // dst, source (localstore address)

    // old compares
    FPEQ,        // dst, source1, source2
    FPNE,        // dst, source1, source2
    FPLT,        // dst, source1, source2
    FPLE,        // dst, source1, source2
    EQ,          // dst, source1, source2
    NE,          // dst, source1, source2
    LT,          // dst, source1, source2
    LE,          // dst, source1, source2
    BNZ,         // source, unused, jmp_addr(imm)

    LOADL1,      // dst, addr(reg), offset(imm)
    STORE,       // addr, value(reg), offset(imm)
    LOADIMM,     // dst, value(imm)
    SPHERE_TEST, // dst(unused), SPHERE, RAY
    MOV,         // dst, source
    MOVINDRD,    // dst, base source, offset reg
    MOVINDWR,    // base dst, source, offset reg
    BLT,         // source1, source2, jmp(imm)
    BET,         // source1, source2, jmp(imm)
    JMP,         // unused, unused, jmp_addr(imm)
    JMPREG,      // source, unused, unused
    JAL,         // dst, unused, jmp_addr(imm)
    RAND,        // dst
    COS,         // dst, src
    SIN,         // dst, src

    // Adding MBlaze inspired instructions here
    ADDC, //mblaze addck
    ADDK, //mblaze add
    ADDKC,//mblaze addc
    BITXOR,
    ANDN,
    CMP,  //sub
    CMPU, //usub
    // maybe remove/switch these to SUBs
    RSUB, // again switch k and non-k from mblaze
    RSUBC,
    RSUBK,
    RSUBKC,
    // continue normal
    MULH,
    MULHU,
    sra,
    srl,
    sext8, // sign extend byte
    // immediate versions of ops
    ADDI,  //mblaze addik
    ADDIC, //mblaze addick
    ADDIK, //mblaze addi
    ADDIKC,//mblaze addic
    RSUBI, // again switch k and non-k from mblaze
    RSUBIC,
    RSUBIK,
    RSUBIKC,
    ANDNI,
    ANDI,
    ORI,
    XORI,
    MULI,
    // local read/writes
    LW,          // dst, addr(reg), offset(reg)
    LWI,         // dst, addr(reg), offset(imm)
    lbu,         // dst, addr(reg) -- load byte unsigned
    lbui,        // dst, addr(reg), offset(imm) -- load byte unsigned immediate
    lhui,        // dst, addr(reg), offset(imm) -- load halfword unsigned immediate
    SW,          // value(reg), addr(reg), offset(reg)
    SWI,         // value(reg), addr(reg), offset(imm)
    sh,
    shi,
    sb,
    sbi,
    // MBlaze branches
    beqd,
    beqid,
    bged,
    bgeid,
    bgtd,
    bgtid,
    bled,
    bleid,
    bltd,
    bltid,
    bned,
    bneid,
    brd,
    brad,
    brld,
    brald,
    brid,
    braid,
    brlid,
    bralid,
    brk,
    brki,
    rtsd,
    bslli,
    bsrli,
    bsrai,
    // extra FP instructions
    FPDIV,
    DIV,
    FPUN,
    FPRSUB,
    FPSQRT,
    FPNEG,
    FPGT,
    FPGE,
    // End MBlaze additions

    SLEEP,       // src (num sleep cycles)
    SYNC,        // 
    NOP,         //
    HALT,        //
    PRINT,       // which_reg
    PRINTF,      // src (containing addr to format string)
    PROF,        // kernel_id, unused, unused
    // end profiling instructions
    NUM_OPS      //
  };

  static std::string Opnames[NUM_OPS];

  Instruction(Opcode code,
              int arg0, int arg1, int arg2,
              int pc_addr = 0);
  Instruction(const Instruction& ins);

  // Helper function to see if all the ray data is stable
  bool RayReady(int ray_start, long long int* writes_in_flight, int kNoBlock) const;
  bool ReadyToIssue(long long int* register_ready, int* fail_reg, long long int cur_cycle) const;
  void print();
  Opcode op;
  int args[3];

  // For "profiling"
  // Keep track of performance data
  long long int executions;
  long long int data_stalls;
  
  
  // Since we're assuming an in-order processor, we never have to wait
  // for the destination register (since anyone that depended on it
  // before would have blocked the processor)
  
};

// A helper class that contains an instruction and
// the time it was receieved

class IssueUnit;
class ThreadState;
class WriteRequest;

class InstructionRecord {
public:
  long long int cycle_arrived;
  InstructionRecord(Instruction& ins,
                    long long int cycle,
                    IssueUnit* issue,
                    ThreadState* thread,
		    bool cache_hit = false);

  InstructionRecord(const InstructionRecord& record);

  Instruction instruction;
  
  IssueUnit* issuer;
  ThreadState* thread;
  bool cache_hit;

  std::vector<WriteRequest*> write_requests;
  // If the user so wishes, you can use this space to store the
  // instructions possible values (instead of just pointers)
  union {
    int ivalues[32];
    unsigned int uvalues[32];
    float fvalues[32];
  };
};

class InstructionQueue {
public:
  InstructionQueue();
  int TopMatches(long long int cycle, int num_check) const;
  int LastMatches(long long int cycle, int num_check) const;
  bool Ready(long long int cycle) const;
  InstructionRecord* Pop();
  void Push(InstructionRecord* record);
  int Size() const;
  void Print() const;

  std::deque<InstructionRecord*> instructions;
};

// Same as above, but sorted on push based on cycle_arrived
class InstructionPriorityQueue : public InstructionQueue {
public:
  InstructionPriorityQueue();
  void Push(InstructionRecord* record);

};

#endif // _SIMHWRT_INSTRUCTION_H_
