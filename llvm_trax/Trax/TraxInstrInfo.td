//===- TraxInstrInfo.td - Trax Instruction defs --------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//
include "TraxInstrFormats.td"

//===----------------------------------------------------------------------===//
// Trax type profiles
//===----------------------------------------------------------------------===//

// def SDTTraxSelectCC : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>]>;
def SDT_TraxRet     : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def SDT_TraxIRet    : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def SDT_TraxJmpLink : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_MBCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>]>;
def SDT_MBCallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;

//===----------------------------------------------------------------------===//
// Trax specific nodes
//===----------------------------------------------------------------------===//

def TraxRet     : SDNode<"TraxISD::Ret", SDT_TraxRet,
                           [SDNPHasChain, SDNPOptInGlue]>;
def TraxIRet    : SDNode<"TraxISD::IRet", SDT_TraxIRet,
                           [SDNPHasChain, SDNPOptInGlue]>;

def TraxJmpLink : SDNode<"TraxISD::JmpLink",SDT_TraxJmpLink,
                           [SDNPHasChain,SDNPOptInGlue,SDNPOutGlue,
                            SDNPVariadic]>;

def MBWrapper   : SDNode<"TraxISD::Wrap", SDTIntUnaryOp>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_MBCallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;

def callseq_end   : SDNode<"ISD::CALLSEQ_END", SDT_MBCallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

//===----------------------------------------------------------------------===//
// Trax Instruction Predicate Definitions.
//===----------------------------------------------------------------------===//
def HasPipe3     : Predicate<"Subtarget.hasPipe3()">;
def HasBarrel    : Predicate<"Subtarget.hasBarrel()">;
def NoBarrel     : Predicate<"!Subtarget.hasBarrel()">;
def HasDiv       : Predicate<"Subtarget.hasDiv()">;
def HasMul       : Predicate<"Subtarget.hasMul()">;
def HasFSL       : Predicate<"Subtarget.hasFSL()">;
def HasEFSL      : Predicate<"Subtarget.hasEFSL()">;
def HasMSRSet    : Predicate<"Subtarget.hasMSRSet()">;
def HasException : Predicate<"Subtarget.hasException()">;
def HasPatCmp    : Predicate<"Subtarget.hasPatCmp()">;
def HasFPU       : Predicate<"Subtarget.hasFPU()">;
def HasESR       : Predicate<"Subtarget.hasESR()">;
def HasPVR       : Predicate<"Subtarget.hasPVR()">;
def HasMul64     : Predicate<"Subtarget.hasMul64()">;
def HasSqrt      : Predicate<"Subtarget.hasSqrt()">;
def HasMMU       : Predicate<"Subtarget.hasMMU()">;

//===----------------------------------------------------------------------===//
// Trax Operand, Complex Patterns and Transformations Definitions.
//===----------------------------------------------------------------------===//

def TraxMemAsmOperand : AsmOperandClass {
  let Name = "Mem";
  let SuperClasses = [];
}

def TraxFslAsmOperand : AsmOperandClass {
  let Name = "Fsl";
  let SuperClasses = [];
}

// Instruction operand types
def brtarget    : Operand<OtherVT>;
def calltarget  : Operand<i32>;
def simm16      : Operand<i32>;
def uimm5       : Operand<i32>;
def uimm15      : Operand<i32>;
def fimm        : Operand<f32>;

// Unsigned Operand
def uimm16      : Operand<i32> {
  let PrintMethod = "printUnsignedImm";
}

// FSL Operand
def fslimm      : Operand<i32> {
  let PrintMethod = "printFSLImm";
  let ParserMatchClass = TraxFslAsmOperand;
}

// Address operand
def memri : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR, simm16);
  let ParserMatchClass = TraxMemAsmOperand;
}

def memrr : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR, GPR);
  let ParserMatchClass = TraxMemAsmOperand;
}

// Node immediate fits as 16-bit sign extended on target immediate.
def immSExt16  : PatLeaf<(imm), [{
  return (N->getZExtValue() >> 16) == 0;
}]>;

// Node immediate fits as 16-bit zero extended on target immediate.
// The LO16 param means that only the lower 16 bits of the node
// immediate are caught.
// e.g. addiu, sltiu
def immZExt16  : PatLeaf<(imm), [{
  return (N->getZExtValue() >> 16) == 0;
}]>;

// FSL immediate field must fit in 4 bits.
def immZExt4 : PatLeaf<(imm), [{
  return N->getZExtValue() == ((N->getZExtValue()) & 0xf) ;
}]>;

// shamt field must fit in 5 bits.
def immZExt5 : PatLeaf<(imm), [{
  return N->getZExtValue() == ((N->getZExtValue()) & 0x1f) ;
}]>;

// Trax Address Mode. SDNode frameindex could possibily be a match
// since load and store instructions from stack used it.
def iaddr : ComplexPattern<i32, 2, "SelectAddrRegImm", [frameindex], []>;
def xaddr : ComplexPattern<i32, 2, "SelectAddrRegReg", [], []>;

//===----------------------------------------------------------------------===//
// Pseudo instructions
//===----------------------------------------------------------------------===//

// As stack alignment is always done with addiu, we need a 16-bit immediate
let Defs = [R1], Uses = [R1] in {
def ADJCALLSTACKDOWN : TraxPseudo<(outs), (ins simm16:$amt),
                                  "#ADJCALLSTACKDOWN $amt",
                                  [(callseq_start timm:$amt)]>;
def ADJCALLSTACKUP   : TraxPseudo<(outs),
                                  (ins uimm16:$amt1, simm16:$amt2),
                                  "#ADJCALLSTACKUP $amt1",
                                  [(callseq_end timm:$amt1, timm:$amt2)]>;
}

//===----------------------------------------------------------------------===//
// Instructions specific format
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Arithmetic Instructions
//===----------------------------------------------------------------------===//
class Arith<bits<6> op, bits<11> flags, string instr_asm, SDNode OpNode,
            InstrItinClass itin> :
            TA<op, flags, (outs GPR:$dst), (ins GPR:$b, GPR:$c),
               !strconcat(instr_asm, "   $dst, $b, $c"),
               [(set GPR:$dst, (OpNode GPR:$b, GPR:$c))], itin>;

class ArithI<bits<6> op, string instr_asm, SDNode OpNode,
             Operand Od, PatLeaf imm_type> :
             TB<op, (outs GPR:$dst), (ins GPR:$b, Od:$c),
                !strconcat(instr_asm, "   $dst, $b, $c"),
                [(set GPR:$dst, (OpNode GPR:$b, imm_type:$c))], IIAlu>;

class ArithI32<bits<6> op, string instr_asm,Operand Od, PatLeaf imm_type> :
               TB<op, (outs GPR:$dst), (ins GPR:$b, Od:$c),
                  !strconcat(instr_asm, "   $dst, $b, $c"),
                  [], IIAlu>;

class ShiftI<bits<6> op, bits<2> flags, string instr_asm, SDNode OpNode,
             Operand Od, PatLeaf imm_type> :
             SHT<op, flags, (outs GPR:$dst), (ins GPR:$b, Od:$c),
                 !strconcat(instr_asm, "   $dst, $b, $c"),
                 [(set GPR:$dst, (OpNode GPR:$b, imm_type:$c))], IIAlu>;

class ArithR<bits<6> op, bits<11> flags, string instr_asm, SDNode OpNode,
            InstrItinClass itin> :
            TAR<op, flags, (outs GPR:$dst), (ins GPR:$b, GPR:$c),
                !strconcat(instr_asm, "   $dst, $c, $b"),
                [(set GPR:$dst, (OpNode GPR:$b, GPR:$c))], itin>;

class ArithRI<bits<6> op, string instr_asm, SDNode OpNode,
             Operand Od, PatLeaf imm_type> :
             TBR<op, (outs GPR:$dst), (ins Od:$b, GPR:$c),
                 !strconcat(instr_asm, "   $dst, $c, $b"),
                 [(set GPR:$dst, (OpNode imm_type:$b, GPR:$c))], IIAlu>;

class ArithN<bits<6> op, bits<11> flags, string instr_asm,
            InstrItinClass itin> :
            TA<op, flags, (outs GPR:$dst), (ins GPR:$b, GPR:$c),
               !strconcat(instr_asm, "   $dst, $b, $c"),
               [], itin>;

class ArithNI<bits<6> op, string instr_asm,Operand Od, PatLeaf imm_type> :
             TB<op, (outs GPR:$dst), (ins GPR:$b, Od:$c),
                !strconcat(instr_asm, "   $dst, $b, $c"),
                [], IIAlu>;

class ArithRN<bits<6> op, bits<11> flags, string instr_asm,
            InstrItinClass itin> :
            TAR<op, flags, (outs GPR:$dst), (ins GPR:$c, GPR:$b),
                !strconcat(instr_asm, "   $dst, $b, $c"),
                [], itin>;

class ArithRNI<bits<6> op, string instr_asm,Operand Od, PatLeaf imm_type> :
             TBR<op, (outs GPR:$dst), (ins Od:$c, GPR:$b),
                 !strconcat(instr_asm, "   $dst, $b, $c"),
                 [], IIAlu>;

//===----------------------------------------------------------------------===//
// Misc Arithmetic Instructions
//===----------------------------------------------------------------------===//

class Logic<bits<6> op, bits<11> flags, string instr_asm, SDNode OpNode> :
            TA<op, flags, (outs GPR:$dst), (ins GPR:$b, GPR:$c),
               !strconcat(instr_asm, "   $dst, $b, $c"),
               [(set GPR:$dst, (OpNode GPR:$b, GPR:$c))], IIAlu>;

class LogicI<bits<6> op, string instr_asm, SDNode OpNode> :
             TB<op, (outs GPR:$dst), (ins GPR:$b, uimm16:$c),
                !strconcat(instr_asm, "   $dst, $b, $c"),
                [(set GPR:$dst, (OpNode GPR:$b, immZExt16:$c))],
                IIAlu>;

class LogicI32<bits<6> op, string instr_asm> :
               TB<op, (outs GPR:$dst), (ins GPR:$b, uimm16:$c),
                  !strconcat(instr_asm, "   $dst, $b, $c"),
                  [], IIAlu>;

class PatCmp<bits<6> op, bits<11> flags, string instr_asm> :
             TA<op, flags, (outs GPR:$dst), (ins GPR:$b, GPR:$c),
                !strconcat(instr_asm, "   $dst, $b, $c"),
                 [], IIAlu>;

//===----------------------------------------------------------------------===//
// Memory Access Instructions
//===----------------------------------------------------------------------===//
class LoadM<bits<6> op, bits<11> flags, string instr_asm> :
            TA<op, flags, (outs GPR:$dst), (ins memrr:$addr),
               !strconcat(instr_asm, "   $dst, $addr"),
               [], IILoad>;

class LoadMI<bits<6> op, string instr_asm, PatFrag OpNode> :
             TB<op, (outs GPR:$dst), (ins memri:$addr),
                !strconcat(instr_asm, "   $dst, $addr"),
                [(set (i32 GPR:$dst), (OpNode iaddr:$addr))], IILoad>;

class StoreM<bits<6> op, bits<11> flags, string instr_asm> :
             TA<op, flags, (outs), (ins GPR:$dst, memrr:$addr),
                !strconcat(instr_asm, "   $dst, $addr"),
                [], IIStore>;

class StoreMI<bits<6> op, string instr_asm, PatFrag OpNode> :
              TB<op, (outs), (ins GPR:$dst, memri:$addr),
                 !strconcat(instr_asm, "   $dst, $addr"),
                 [(OpNode (i32 GPR:$dst), iaddr:$addr)], IIStore>;

//===----------------------------------------------------------------------===//
// Branch Instructions
//===----------------------------------------------------------------------===//
class Branch<bits<6> op, bits<5> br, bits<11> flags, string instr_asm> :
             TA<op, flags, (outs), (ins GPR:$target),
                !strconcat(instr_asm, "   $target"),
                [], IIBranch> {
  let rd = 0x0;
  let ra = br;
  let Form = FCCR;
}

class BranchI<bits<6> op, bits<5> br, string instr_asm> :
              TB<op, (outs), (ins brtarget:$target),
                 !strconcat(instr_asm, "   $target"),
                 [], IIBranch> {
  let rd = 0;
  let ra = br;
  let Form = FCCI;
}

//===----------------------------------------------------------------------===//
// Branch and Link Instructions
//===----------------------------------------------------------------------===//
class BranchL<bits<6> op, bits<5> br, bits<11> flags, string instr_asm> :
              TA<op, flags, (outs), (ins GPR:$link, GPR:$target, variable_ops),
                 !strconcat(instr_asm, "   $link, $target"),
                 [], IIBranch> {
  let ra = br;
  let Form = FRCR;
}

class BranchLI<bits<6> op, bits<5> br, string instr_asm> :
               TB<op, (outs), (ins GPR:$link, calltarget:$target, variable_ops),
                  !strconcat(instr_asm, "   $link, $target"),
                  [], IIBranch> {
  let ra = br;
  let Form = FRCI;
}

//===----------------------------------------------------------------------===//
// Conditional Branch Instructions
//===----------------------------------------------------------------------===//
class BranchC<bits<6> op, bits<5> br, bits<11> flags, string instr_asm> :
              TA<op, flags, (outs),
                 (ins GPR:$a, GPR:$b),
                 !strconcat(instr_asm, "   $a, $b"),
                 [], IIBranch> {
  let rd = br;
  let Form = FCRR;
}

class BranchCI<bits<6> op, bits<5> br, string instr_asm> :
               TB<op, (outs), (ins GPR:$a, brtarget:$offset),
                  !strconcat(instr_asm, "   $a, $offset"),
                  [], IIBranch> {
  let rd = br;
  let Form = FCRI;
}

//===----------------------------------------------------------------------===//
// Trax arithmetic instructions
//===----------------------------------------------------------------------===//

let isCommutable = 1, isAsCheapAsAMove = 1 in {
  def ADDK   :  Arith<0x04, 0x000, "ADD    ", add,  IIAlu>;
  def AND    :  Logic<0x21, 0x000, "BITAND ", and>;
  def OR     :  Logic<0x20, 0x000, "BITOR  ", or>;
  def XOR    :  Logic<0x22, 0x000, "BITXOR ", xor>;
  def PCMPBF : PatCmp<0x20, 0x400, "pcmpbf ">;
  def PCMPEQ : PatCmp<0x22, 0x400, "pcmpeq ">;
  def PCMPNE : PatCmp<0x23, 0x400, "pcmpne ">;

  let Defs = [CARRY] in {
    def ADD    :  Arith<0x00, 0x000, "ADDK   ", addc, IIAlu>;

    let Uses = [CARRY] in {
      def ADDC   :  Arith<0x02, 0x000, "ADDKC   ", adde, IIAlu>;
    }
  }

  let Uses = [CARRY] in {
    def ADDKC  : ArithN<0x06, 0x000, "ADDC   ", IIAlu>;
  }
}

let isAsCheapAsAMove = 1 in {
  def ANDN   :  ArithN<0x23, 0x000, "ANDN   ", IIAlu>;
  def CMP    :  ArithN<0x05, 0x001, "CMP    ", IIAlu>;
  def CMPU   :  ArithN<0x05, 0x003, "CMPU   ", IIAlu>;
  def RSUBK  :  ArithR<0x05, 0x000, "RSUB   ", sub,  IIAlu>;

  let Defs = [CARRY] in {
    def RSUB   :  ArithR<0x01, 0x000, "RSUBK  ", subc, IIAlu>;

    let Uses = [CARRY] in {
      def RSUBC  :  ArithR<0x03, 0x000, "RSUBKC ", sube, IIAlu>;
    }
  }

  let Uses = [CARRY] in {
    def RSUBKC : ArithRN<0x07, 0x000, "RSUBC  ", IIAlu>;
  }
}

let isCommutable = 1, Predicates=[HasMul] in {
  def MUL    : Arith<0x10, 0x000, "MUL    ", mul,   IIAlu>;
}

let isCommutable = 1, Predicates=[HasMul,HasMul64] in {
  def MULH   : Arith<0x10, 0x001, "mulh   ", mulhs, IIAlu>;
  def MULHU  : Arith<0x10, 0x003, "mulhu  ", mulhu, IIAlu>;
}

let Predicates=[HasMul,HasMul64] in {
  def MULHSU : ArithN<0x10, 0x002, "mulhsu ", IIAlu>;
}

let Predicates=[HasBarrel] in {
  def BSRL   :   Arith<0x11, 0x000, "bsrl   ", srl, IIAlu>;
  def BSRA   :   Arith<0x11, 0x200, "bsra   ", sra, IIAlu>;
  def BSLL   :   Arith<0x11, 0x400, "bsll   ", shl, IIAlu>;
  def BSRLI  :  ShiftI<0x19, 0x0, "bsrli  ", srl, uimm5, immZExt5>;
  def BSRAI  :  ShiftI<0x19, 0x1, "bsrai  ", sra, uimm5, immZExt5>;
  def BSLLI  :  ShiftI<0x19, 0x2, "bslli  ", shl, uimm5, immZExt5>;
}

let Predicates=[HasDiv] in {
  def IDIV   :  ArithR<0x12, 0x000, "DIV    ", sdiv, IIAlu>;
  def IDIVU  :  ArithR<0x12, 0x002, "idivu  ", udiv, IIAlu>;
}

//===----------------------------------------------------------------------===//
// Trax immediate mode arithmetic instructions
//===----------------------------------------------------------------------===//

let isAsCheapAsAMove = 1 in {
  def ADDIK   :   ArithI<0x0C, "ADDI   ", add,  simm16, immSExt16>;
  def RSUBIK  :  ArithRI<0x0D, "RSUBI  ", sub, simm16, immSExt16>;
  def ANDNI   :  ArithNI<0x2B, "ANDNI  ", uimm16, immZExt16>;
  def ANDI    :   LogicI<0x29, "ANDI   ", and>;
  def ORI     :   LogicI<0x28, "ORI    ", or>;
  def XORI    :   LogicI<0x2A, "XORI   ", xor>;

  let Defs = [CARRY] in {
    def ADDI    :   ArithI<0x08, "ADDIK  ", addc, simm16, immSExt16>;
    def RSUBI   :  ArithRI<0x09, "rsubi  ", subc,  simm16, immSExt16>;

    let Uses = [CARRY] in {
      def ADDIC   :   ArithI<0x0A, "ADDIKC ", adde, simm16, immSExt16>;
      def RSUBIC  :  ArithRI<0x0B, "rsubic ", sube, simm16, immSExt16>;
    }
  }

  let Uses = [CARRY] in {
    def ADDIKC  :  ArithNI<0x0E, "ADDIC  ", simm16, immSExt16>;
    def RSUBIKC : ArithRNI<0x0F, "rsubikc", simm16, immSExt16>;
  }
}

let Predicates=[HasMul] in {
  def MULI    :   ArithI<0x18, "MULI   ", mul, simm16, immSExt16>;
}

//===----------------------------------------------------------------------===//
// Trax memory access instructions
//===----------------------------------------------------------------------===//

let canFoldAsLoad = 1, isReMaterializable = 1 in {
  def LBU  :  LoadM<0x30, 0x000, "lbu    ">;
  def LBUR :  LoadM<0x30, 0x200, "lbur   ">;

  def LHU  :  LoadM<0x31, 0x000, "lhu    ">;
  def LHUR :  LoadM<0x31, 0x200, "lhur   ">;

  def LW   :  LoadM<0x32, 0x000, "LW     ">;
  def LWR  :  LoadM<0x32, 0x200, "lwr    ">;

  let Defs = [CARRY] in {
    def LWX  :  LoadM<0x32, 0x400, "lwx    ">;
  }

  def LBUI : LoadMI<0x38, "lbui   ", zextloadi8>;
  def LHUI : LoadMI<0x39, "lhui   ", zextloadi16>;
  def LWI  : LoadMI<0x3A, "LWI    ", load>;
}

def SB  :  StoreM<0x34, 0x000, "sb     ">;
def SBR :  StoreM<0x34, 0x200, "sbr    ">;

def SH  :  StoreM<0x35, 0x000, "sh     ">;
def SHR :  StoreM<0x35, 0x200, "shr    ">;

def SW  :  StoreM<0x36, 0x000, "SW     ">;
def SWR :  StoreM<0x36, 0x200, "swr    ">;

let Defs = [CARRY] in {
  def SWX :  StoreM<0x36, 0x400, "swx    ">;
}

def SBI : StoreMI<0x3C, "sbi    ", truncstorei8>;
def SHI : StoreMI<0x3D, "shi    ", truncstorei16>;
def SWI : StoreMI<0x3E, "SWI    ", store>;

//===----------------------------------------------------------------------===//
// Trax branch instructions
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1, hasCtrlDep = 1, isBarrier = 1 in {
  def BRI    :  BranchI<0x2E, 0x00, "bri    ">;
  def BRAI   :  BranchI<0x2E, 0x08, "brai   ">;
}

let isBranch = 1, isTerminator = 1, hasCtrlDep = 1 in {
  def BEQI   : BranchCI<0x2F, 0x00, "beqi   ">;
  def BNEI   : BranchCI<0x2F, 0x01, "bnei   ">;
  def BLTI   : BranchCI<0x2F, 0x02, "blti   ">;
  def BLEI   : BranchCI<0x2F, 0x03, "blei   ">;
  def BGTI   : BranchCI<0x2F, 0x04, "bgti   ">;
  def BGEI   : BranchCI<0x2F, 0x05, "bgei   ">;
}

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, hasCtrlDep = 1,
    isBarrier = 1 in {
  def BR     :   Branch<0x26, 0x00, 0x000, "br     ">;
  def BRA    :   Branch<0x26, 0x08, 0x000, "bra    ">;
}

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, hasCtrlDep = 1 in {
  def BEQ    :  BranchC<0x27, 0x00, 0x000, "beq    ">;
  def BNE    :  BranchC<0x27, 0x01, 0x000, "bne    ">;
  def BLT    :  BranchC<0x27, 0x02, 0x000, "blt    ">;
  def BLE    :  BranchC<0x27, 0x03, 0x000, "ble    ">;
  def BGT    :  BranchC<0x27, 0x04, 0x000, "bgt    ">;
  def BGE    :  BranchC<0x27, 0x05, 0x000, "bge    ">;
}

let isBranch = 1, isTerminator = 1, hasDelaySlot = 1, hasCtrlDep = 1,
    isBarrier = 1 in {
  def BRID   :  BranchI<0x2E, 0x10, "brid   ">;
  def BRAID  :  BranchI<0x2E, 0x18, "braid  ">;
}

let isBranch = 1, isTerminator = 1, hasDelaySlot = 1, hasCtrlDep = 1 in {
  def BEQID  : BranchCI<0x2F, 0x10, "beqid  ">;
  def BNEID  : BranchCI<0x2F, 0x11, "bneid  ">;
  def BLTID  : BranchCI<0x2F, 0x12, "bltid  ">;
  def BLEID  : BranchCI<0x2F, 0x13, "bleid  ">;
  def BGTID  : BranchCI<0x2F, 0x14, "bgtid  ">;
  def BGEID  : BranchCI<0x2F, 0x15, "bgeid  ">;
}

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1,
    hasDelaySlot = 1, hasCtrlDep = 1, isBarrier = 1 in {
  def BRD    :   Branch<0x26, 0x10, 0x000, "brd    ">;
  def BRAD   :   Branch<0x26, 0x18, 0x000, "brad   ">;
}

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1,
    hasDelaySlot = 1, hasCtrlDep = 1 in {
  def BEQD   :  BranchC<0x27, 0x10, 0x000, "beqd   ">;
  def BNED   :  BranchC<0x27, 0x11, 0x000, "bned   ">;
  def BLTD   :  BranchC<0x27, 0x12, 0x000, "bltd   ">;
  def BLED   :  BranchC<0x27, 0x13, 0x000, "bled   ">;
  def BGTD   :  BranchC<0x27, 0x14, 0x000, "bgtd   ">;
  def BGED   :  BranchC<0x27, 0x15, 0x000, "bged   ">;
}

let isCall =1, hasDelaySlot = 1,
    Defs = [R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,CARRY],
    Uses = [R1] in {
  def BRLID  : BranchLI<0x2E, 0x14, "brlid  ">;
  def BRALID : BranchLI<0x2E, 0x1C, "bralid ">;
}

let isCall = 1, hasDelaySlot = 1,
    Defs = [R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,CARRY],
    Uses = [R1] in {
  def BRLD   : BranchL<0x26, 0x14, 0x000, "brld   ">;
  def BRALD  : BranchL<0x26, 0x1C, 0x000, "brald  ">;
}

let isReturn=1, isTerminator=1, hasDelaySlot=1, isBarrier=1,
    rd=0x10, Form=FCRI in {
  def RTSD   : TB<0x2D, (outs), (ins GPR:$target, simm16:$imm),
                  "rtsd      $target, $imm",
                  [],
                  IIBranch>;
}

let isReturn=1, isTerminator=1, hasDelaySlot=1, isBarrier=1,
    rd=0x11, Form=FCRI in {
  def RTID   : TB<0x2D, (outs), (ins GPR:$target, simm16:$imm),
                  "rtid      $target, $imm",
                  [],
                  IIBranch>;
}

let isReturn=1, isTerminator=1, hasDelaySlot=1, isBarrier=1,
    rd=0x12, Form=FCRI in {
  def RTBD   : TB<0x2D, (outs), (ins GPR:$target, simm16:$imm),
                  "rtbd      $target, $imm",
                  [],
                  IIBranch>;
}

let isReturn=1, isTerminator=1, hasDelaySlot=1, isBarrier=1,
    rd=0x14, Form=FCRI in {
  def RTED   : TB<0x2D, (outs), (ins GPR:$target, simm16:$imm),
                  "rted      $target, $imm",
                  [],
                  IIBranch>;
}


//===----------------------------------------------------------------------===//
// Trax intrinsic instructions *****jbs*****
//===----------------------------------------------------------------------===//

def PRINT :  TraxInst< 0x20, FC, (outs), (ins GPR:$target), 
		       "PRINT     $target", [], IIAlu>;
def PROFILE :  TraxInst< 0x20, FC, (outs), (ins simm16:$imm), 
		       "PROF      $imm", [], IIAlu>;
def LOAD :  TraxInst< 0x20, FC, (outs GPR:$dst), (ins GPR:$addr, simm16:$imm),
		      "LOAD      $dst, $addr, $imm", [], IILoad>;
// for some reason the $addr and $dst are in this order for stores
def STORE : TraxInst< 0x20, FC, (outs), (ins GPR:$dst, GPR:$addr, simm16:$imm),
		      "STORE     $addr, $dst, $imm", [], IIStore>;
def ATOMINC :  TraxInst< 0x20, FC, (outs GPR:$dst), (ins simm16:$imm),
			 "ATOMIC_INC $dst, $imm", [], IILoad>;
def GLOBALRRD : TraxInst< 0x20, FC, (outs GPR:$dst), (ins simm16:$imm),
			 "GLOBAL_READ $dst, $imm", [], IILoad>;
def INCRESET :  TraxInst< 0x20, FC, (outs GPR:$dst), (ins simm16:$imm),
			 "INC_RESET  $dst, $imm", [], IILoad>;
def BARRIER :  TraxInst< 0x20, FC, (outs), (ins simm16:$imm),
			 "BARRIER    $imm", [], IILoad>;
def MIN : TraxInst< 0x20, FC, (outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
		    "FPMIN     $dst, $src1, $src2", [], IIAlu>;
def MAX : TraxInst< 0x20, FC, (outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
		    "FPMAX     $dst, $src1, $src2", [], IIAlu>;
def INVSQRT :  TraxInst< 0x20, FC, (outs GPR:$dst), (ins GPR:$src),
			 "FPINVSQRT $dst, $src", [], IIAlu>;
def RAND :  TraxInst< 0x20, FC, (outs GPR:$dst), (ins),
			 "RAND      $dst", [], IIAlu>;
def WRITESR : TraxInst< 0x20, FC, (outs), (ins GPR:$src1, GPR:$src2, GPR:$src3),
		    "WRITESR   $src1, $src2, $src3", [], IIAlu>;
def CALLNOISE :  TraxInst< 0x20, FC, (outs GPR:$dst), (ins),
			 "CALLNOISE $dst", [], IIAlu>;
// Stream queue intrinsics
def STARTSW : TraxInst< 0x20, FC, (outs), (ins GPR:$src1),
			"STARTSW   $src1", [], IIAlu>;
def STREAMW : TraxInst< 0x20, FC, (outs), (ins GPR:$src1),
			"STREAMW   $src1", [], IIAlu>;
def ENDSW : TraxInst< 0x20, FC, (outs), (ins),
		      "ENDSW     ", [], IIAlu>;
def STARTSR : TraxInst< 0x20, FC, (outs GPR:$dst), (ins),
			"STARTSR   $dst", [], IIAlu>;
def STREAMR : TraxInst< 0x20, FC, (outs GPR:$dst), (ins),
			"STREAMR   $dst", [], IIAlu>;
def ENDSR : TraxInst< 0x20, FC, (outs), (ins),
		      "ENDSR     ", [], IIAlu>;
def STRSIZE : TraxInst< 0x20, FC, (outs GPR:$dst), (ins GPR:$src1),
		      "STRSIZE   $dst, $src1", [], IIAlu>;
def STRSCHED : TraxInst< 0x20, FC, (outs GPR:$dst), (ins GPR:$src1),
		      "STRSCHED  $dst, $src1", [], IIAlu>;
def SETSTRID : TraxInst< 0x20, FC, (outs), (ins GPR:$src1),
		      "SETSTRID  $src1", [], IIAlu>;

// Turns out we just needed an exposed MOV with an immediate
def GETID : TraxInst< 0x20, FC, (outs GPR:$dst), (ins simm16:$imm),
		    "MOV       $dst, $imm", [], IIAlu>;


//===----------------------------------------------------------------------===//
// Trax misc instructions
//===----------------------------------------------------------------------===//

let neverHasSideEffects = 1 in {
  def NOP :  TraxInst< 0x20, FC, (outs), (ins), "NOP    ", [], IIAlu>;
}

let usesCustomInserter = 1 in {
  def Select_CC : TraxPseudo<(outs GPR:$dst),
    (ins GPR:$T, GPR:$F, GPR:$CMP, i32imm:$CC), // F T reversed
    "; SELECT_CC PSEUDO!",
    []>;

  def ShiftL : TraxPseudo<(outs GPR:$dst),
    (ins GPR:$L, GPR:$R),
    "; ShiftL PSEUDO!",
    []>;

  def ShiftRA : TraxPseudo<(outs GPR:$dst),
    (ins GPR:$L, GPR:$R),
    "; ShiftRA PSEUDO!",
    []>;

  def ShiftRL : TraxPseudo<(outs GPR:$dst),
    (ins GPR:$L, GPR:$R),
    "; ShiftRL PSEUDO!",
    []>;
}

let rb = 0 in {
  def SEXT16 : TA<0x24, 0x061, (outs GPR:$dst), (ins GPR:$src),
                  "sext16    $dst, $src", [], IIAlu>;
  def SEXT8  : TA<0x24, 0x060, (outs GPR:$dst), (ins GPR:$src),
                  "sext8     $dst, $src", [], IIAlu>;
  let Defs = [CARRY] in {
    def SRL    : TA<0x24, 0x041, (outs GPR:$dst), (ins GPR:$src),
                    "srl       $dst, $src", [], IIAlu>;
    def SRA    : TA<0x24, 0x001, (outs GPR:$dst), (ins GPR:$src),
                    "sra       $dst, $src", [], IIAlu>;
    let Uses = [CARRY] in {
      def SRC    : TA<0x24, 0x021, (outs GPR:$dst), (ins GPR:$src),
                      "src       $dst, $src", [], IIAlu>;
    }
  }
}

let isCodeGenOnly=1 in {
  def ADDIK32 : ArithI32<0x08, "ADDI   ", simm16, immSExt16>;
  def ORI32   : LogicI32<0x28, "ORI    ">;
  def BRLID32 : BranchLI<0x2E, 0x14, "brlid  ">;
}

//===----------------------------------------------------------------------===//
// Misc. instructions
//===----------------------------------------------------------------------===//
let Form=FRCS in {
  def MFS : SPC<0x25, 0x2, (outs GPR:$dst), (ins SPR:$src),
                "mfs       $dst, $src", [], IIAlu>;
}

let Form=FCRCS in {
  def MTS : SPC<0x25, 0x3, (outs SPR:$dst), (ins GPR:$src),
                "mts       $dst, $src", [], IIAlu>;
}

def MSRSET : MSR<0x25, 0x20, (outs GPR:$dst), (ins uimm15:$set),
                 "msrset    $dst, $set", [], IIAlu>;

def MSRCLR : MSR<0x25, 0x22, (outs GPR:$dst), (ins uimm15:$clr),
                 "msrclr    $dst, $clr", [], IIAlu>;

let rd=0x0, Form=FCRR in {
  def WDC  : TA<0x24, 0x64, (outs), (ins GPR:$a, GPR:$b),
                "wdc       $a, $b", [], IIAlu>;
  def WDCF : TA<0x24, 0x74, (outs), (ins GPR:$a, GPR:$b),
                "wdc.flush $a, $b", [], IIAlu>;
  def WDCC : TA<0x24, 0x66, (outs), (ins GPR:$a, GPR:$b),
                "wdc.clear $a, $b", [], IIAlu>;
  def WIC  : TA<0x24, 0x68, (outs), (ins GPR:$a, GPR:$b),
                "wic       $a, $b", [], IIAlu>;
}

def BRK  :  BranchL<0x26, 0x0C, 0x000, "brk    ">;
def BRKI : BranchLI<0x2E, 0x0C, "brki   ">;

def IMM : TraxInst<0x2C, FCCI, (outs), (ins simm16:$imm),
                     "imm       $imm", [], IIAlu>;

//===----------------------------------------------------------------------===//
// Pseudo instructions for atomic operations
//===----------------------------------------------------------------------===//
let usesCustomInserter=1 in {
  def CAS32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$cmp, GPR:$swp),
    "# atomic compare and swap",
    [(set GPR:$dst, (atomic_cmp_swap_32 GPR:$ptr, GPR:$cmp, GPR:$swp))]>;

  def SWP32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$swp),
    "# atomic swap",
    [(set GPR:$dst, (atomic_swap_32 GPR:$ptr, GPR:$swp))]>;

  def LAA32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$val),
    "# atomic load and add",
    [(set GPR:$dst, (atomic_load_add_32 GPR:$ptr, GPR:$val))]>;

  def LAS32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$val),
    "# atomic load and sub",
    [(set GPR:$dst, (atomic_load_sub_32 GPR:$ptr, GPR:$val))]>;

  def LAD32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$val),
    "# atomic load and and",
    [(set GPR:$dst, (atomic_load_and_32 GPR:$ptr, GPR:$val))]>;

  def LAO32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$val),
    "# atomic load and or",
    [(set GPR:$dst, (atomic_load_or_32 GPR:$ptr, GPR:$val))]>;

  def LAX32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$val),
    "# atomic load and xor",
    [(set GPR:$dst, (atomic_load_xor_32 GPR:$ptr, GPR:$val))]>;

  def LAN32 : TraxPseudo<(outs GPR:$dst), (ins GPR:$ptr, GPR:$val),
    "# atomic load and nand",
    [(set GPR:$dst, (atomic_load_nand_32 GPR:$ptr, GPR:$val))]>;

  def MEMBARRIER : TraxPseudo<(outs), (ins),
    "# memory barrier",
    [(membarrier (i32 imm), (i32 imm), (i32 imm), (i32 imm), (i32 imm))]>;
}

//===----------------------------------------------------------------------===//
//  Arbitrary patterns that map to one or more instructions
//===----------------------------------------------------------------------===//

// Small immediates
def : Pat<(i32 0), (ADDK (i32 R0), (i32 R0))>;
def : Pat<(i32 immSExt16:$imm), (ADDIK (i32 R0), imm:$imm)>;
def : Pat<(i32 immZExt16:$imm), (ORI (i32 R0), imm:$imm)>;

// Arbitrary immediates
def : Pat<(i32 imm:$imm), (ADDIK (i32 R0), imm:$imm)>;

// In register sign extension
def : Pat<(sext_inreg GPR:$src, i16), (SEXT16 GPR:$src)>;
def : Pat<(sext_inreg GPR:$src, i8),  (SEXT8 GPR:$src)>;

// Call
def : Pat<(TraxJmpLink (i32 tglobaladdr:$dst)),
          (BRLID (i32 R15), tglobaladdr:$dst)>;

def : Pat<(TraxJmpLink (i32 texternalsym:$dst)),
          (BRLID (i32 R15), texternalsym:$dst)>;

def : Pat<(TraxJmpLink GPR:$dst),
          (BRALD (i32 R15), GPR:$dst)>;

// Shift Instructions
def : Pat<(shl GPR:$L, GPR:$R), (ShiftL GPR:$L, GPR:$R)>;
def : Pat<(sra GPR:$L, GPR:$R), (ShiftRA GPR:$L, GPR:$R)>;
def : Pat<(srl GPR:$L, GPR:$R), (ShiftRL GPR:$L, GPR:$R)>;

// SET_CC operations
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETEQ),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMP GPR:$R, GPR:$L), 1)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETNE),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMP GPR:$R, GPR:$L), 2)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETGT),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMP GPR:$R, GPR:$L), 3)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETLT),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMP GPR:$R, GPR:$L), 4)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETGE),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMP GPR:$R, GPR:$L), 5)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETLE),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMP GPR:$R, GPR:$L), 6)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETUGT),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMPU GPR:$R, GPR:$L), 3)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETULT),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMPU GPR:$R, GPR:$L), 4)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETUGE),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMPU GPR:$R, GPR:$L), 5)>;
def : Pat<(setcc (i32 GPR:$L), (i32 GPR:$R), SETULE),
          (Select_CC (ADDIK (i32 R0), 1), (ADDIK (i32 R0), 0),
                     (CMPU GPR:$R, GPR:$L), 6)>;

// SELECT operations
def : Pat<(select (i32 GPR:$C), (i32 GPR:$T), (i32 GPR:$F)),
          (Select_CC GPR:$T, GPR:$F, GPR:$C, 2)>;

// SELECT_CC
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETEQ),
          (Select_CC GPR:$T, GPR:$F, (CMP GPR:$R, GPR:$L), 1)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETNE),
          (Select_CC GPR:$T, GPR:$F, (CMP GPR:$R, GPR:$L), 2)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETGT),
          (Select_CC GPR:$T, GPR:$F, (CMP GPR:$R, GPR:$L), 3)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETLT),
          (Select_CC GPR:$T, GPR:$F, (CMP GPR:$R, GPR:$L), 4)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETGE),
          (Select_CC GPR:$T, GPR:$F, (CMP GPR:$R, GPR:$L), 5)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETLE),
          (Select_CC GPR:$T, GPR:$F, (CMP GPR:$R, GPR:$L), 6)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETUGT),
          (Select_CC GPR:$T, GPR:$F, (CMPU GPR:$R, GPR:$L), 3)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETULT),
          (Select_CC GPR:$T, GPR:$F, (CMPU GPR:$R, GPR:$L), 4)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETUGE),
          (Select_CC GPR:$T, GPR:$F, (CMPU GPR:$R, GPR:$L), 5)>;
def : Pat<(selectcc (i32 GPR:$L), (i32 GPR:$R),
                    (i32 GPR:$T), (i32 GPR:$F), SETULE),
          (Select_CC GPR:$T, GPR:$F, (CMPU GPR:$R, GPR:$L), 6)>;

// Ret instructions
def : Pat<(TraxRet GPR:$target), (RTSD GPR:$target, 0x8)>;
def : Pat<(TraxIRet GPR:$target), (RTID GPR:$target, 0x0)>;

// BR instructions
def : Pat<(br bb:$T), (BRID bb:$T)>;
def : Pat<(brind GPR:$T), (BRAD GPR:$T)>;

// BRCOND instructions
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETEQ), bb:$T),
          (BEQID (CMP GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETNE), bb:$T),
          (BNEID (CMP GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETGT), bb:$T),
          (BGTID (CMP GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETLT), bb:$T),
          (BLTID (CMP GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETGE), bb:$T),
          (BGEID (CMP GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETLE), bb:$T),
          (BLEID (CMP GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETUGT), bb:$T),
          (BGTID (CMPU GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETULT), bb:$T),
          (BLTID (CMPU GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETUGE), bb:$T),
          (BGEID (CMPU GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (setcc (i32 GPR:$L), (i32 GPR:$R), SETULE), bb:$T),
          (BLEID (CMPU GPR:$R, GPR:$L), bb:$T)>;
def : Pat<(brcond (i32 GPR:$C), bb:$T),
          (BNEID GPR:$C, bb:$T)>;

// Jump tables, global addresses, and constant pools
def : Pat<(MBWrapper tglobaladdr:$in), (ORI (i32 R0), tglobaladdr:$in)>;
def : Pat<(MBWrapper tjumptable:$in),  (ORI (i32 R0), tjumptable:$in)>;
def : Pat<(MBWrapper tconstpool:$in),  (ORI (i32 R0), tconstpool:$in)>;

// Intrinsic instructions *****jbs*****
def : Pat<(int_trax_printi (i32 GPR:$in)),(PRINT GPR:$in)>;
def : Pat<(int_trax_printf (f32 GPR:$in)),(PRINT GPR:$in)>;
// Profile
def : Pat<(int_trax_profile (i32 imm:$imm)),(PROFILE imm:$imm)>;
// global loads
def : Pat<(int_trax_loadi (i32 GPR:$addr), imm:$imm),
	  (LOAD GPR:$addr, imm:$imm)>;
def : Pat<(int_trax_loadf (i32 GPR:$addr), imm:$imm),
	  (LOAD GPR:$addr, imm:$imm)>;
// global stores
def : Pat<(int_trax_storei (i32 GPR:$dst), GPR:$addr, imm:$imm),
	  (STORE GPR:$dst, GPR:$addr, imm:$imm)>;
def : Pat<(int_trax_storef (f32 GPR:$dst), GPR:$addr, imm:$imm),
	  (STORE GPR:$dst, GPR:$addr, imm:$imm)>;
// atominc
def : Pat<(int_trax_atominc imm:$imm),
	  (ATOMINC imm:$imm)>;
// global register read
def : Pat<(int_trax_globalrrd imm:$imm),
	  (GLOBALRRD imm:$imm)>;
// min/max
def : Pat<(int_trax_min (f32 GPR:$src1), (f32 GPR:$src2)),
	  (MIN GPR:$src1, GPR:$src2)>;
def : Pat<(int_trax_max (f32 GPR:$src1), (f32 GPR:$src2)),
	  (MAX GPR:$src1, GPR:$src2)>;
// inv_sqrt
def : Pat<(int_trax_invsqrt (f32 GPR:$src)),
	  (INVSQRT GPR:$src)>;
// rand
def : Pat<(int_trax_rand),
	  (RAND)>;
// barrier
def : Pat<(int_trax_increset imm:$imm),
	  (INCRESET imm:$imm)>;
def : Pat<(int_trax_barrier imm:$imm),
	  (BARRIER imm:$imm)>;
// noise stuff
def : Pat<(int_trax_writesrf (f32 GPR:$src1), (f32 GPR:$src2), (f32 GPR:$src3)),
	  (WRITESR GPR:$src1, GPR:$src2, GPR:$src3)>;
def : Pat<(int_trax_writesri (i32 GPR:$src1), (i32 GPR:$src2), (i32 GPR:$src3)),
	  (WRITESR GPR:$src1, GPR:$src2, GPR:$src3)>;
def : Pat<(int_trax_callnoise),
	  (CALLNOISE)>;
// Stream queue stuff
def : Pat<(int_trax_startsw (i32 GPR:$src1)),
	  (STARTSW GPR:$src1)>;
def : Pat<(int_trax_streamwi (i32 GPR:$src1)),
	  (STREAMW GPR:$src1)>;
def : Pat<(int_trax_streamwf (f32 GPR:$src1)),
	  (STREAMW GPR:$src1)>;
def : Pat<(int_trax_endsw),
	  (ENDSW)>;
def : Pat<(int_trax_startsr),
	  (STARTSR)>;
def : Pat<(int_trax_streamri),
	  (STREAMR)>;
def : Pat<(int_trax_streamrf),
	  (STREAMR)>;
def : Pat<(int_trax_endsr),
	  (ENDSR)>;
def : Pat<(int_trax_streamsize (i32 GPR:$src1)),
	  (STRSIZE GPR:$src1)>;
def : Pat<(int_trax_streamschedule (i32 GPR:$src1)),
	  (STRSCHED GPR:$src1)>;
def : Pat<(int_trax_setstreamid (i32 GPR:$src1)),
	  (SETSTRID GPR:$src1)>;
// thread ID etc.
def : Pat<(int_trax_getid imm:$imm),
	  (GETID imm:$imm)>;


// Misc instructions
def : Pat<(and (i32 GPR:$lh), (not (i32 GPR:$rh))),(ANDN GPR:$lh, GPR:$rh)>;

// Arithmetic with immediates
def : Pat<(add (i32 GPR:$in), imm:$imm),(ADDIK GPR:$in, imm:$imm)>;
def : Pat<(or (i32 GPR:$in), imm:$imm),(ORI GPR:$in, imm:$imm)>;
def : Pat<(xor (i32 GPR:$in), imm:$imm),(XORI GPR:$in, imm:$imm)>;

// Convert any extend loads into zero extend loads
def : Pat<(extloadi8  iaddr:$src), (i32 (LBUI iaddr:$src))>;
def : Pat<(extloadi16 iaddr:$src), (i32 (LHUI iaddr:$src))>;
def : Pat<(extloadi8  xaddr:$src), (i32 (LBU xaddr:$src))>;
def : Pat<(extloadi16 xaddr:$src), (i32 (LHU xaddr:$src))>;

// 32-bit load and store
def : Pat<(store (i32 GPR:$dst), xaddr:$addr), (SW GPR:$dst, xaddr:$addr)>;
def : Pat<(load xaddr:$addr), (i32 (LW xaddr:$addr))>;

// 16-bit load and store
def : Pat<(truncstorei16 (i32 GPR:$dst), xaddr:$addr), (SH GPR:$dst, xaddr:$addr)>;
def : Pat<(zextloadi16 xaddr:$addr), (i32 (LHU xaddr:$addr))>;

// 8-bit load and store
def : Pat<(truncstorei8 (i32 GPR:$dst), xaddr:$addr), (SB GPR:$dst, xaddr:$addr)>;
def : Pat<(zextloadi8 xaddr:$addr), (i32 (LBU xaddr:$addr))>;

// Peepholes
def : Pat<(store (i32 0), iaddr:$dst), (SWI (i32 R0), iaddr:$dst)>;

//===----------------------------------------------------------------------===//
// Floating Point Support
//===----------------------------------------------------------------------===//
include "TraxInstrFSL.td"
include "TraxInstrFPU.td"
