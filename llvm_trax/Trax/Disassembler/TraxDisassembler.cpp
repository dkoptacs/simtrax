//===- TraxDisassembler.cpp - Disassembler for MicroBlaze  ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the Trax Disassembler. It contains code to translate
// the data produced by the decoder into MCInsts.
//
//===----------------------------------------------------------------------===//

#include "Trax.h"
#include "TraxInstrInfo.h"
#include "TraxDisassembler.h"

#include "llvm/MC/EDInstInfo.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/raw_ostream.h"

// #include "TraxGenDecoderTables.inc"
// #include "TraxGenRegisterNames.inc"
#include "TraxGenInstrInfo.inc"
#include "TraxGenEDInfo.inc"

using namespace llvm;

const unsigned UNSUPPORTED = -1;

static unsigned traxBinary2Opcode[] = {
  Trax::ADD,   Trax::RSUB,   Trax::ADDC,   Trax::RSUBC,   //00,01,02,03
  Trax::ADDK,  Trax::RSUBK,  Trax::ADDKC,  Trax::RSUBKC,  //04,05,06,07
  Trax::ADDI,  Trax::RSUBI,  Trax::ADDIC,  Trax::RSUBIC,  //08,09,0A,0B
  Trax::ADDIK, Trax::RSUBIK, Trax::ADDIKC, Trax::RSUBIKC, //0C,0D,0E,0F

  Trax::MUL,   Trax::BSRL,   Trax::IDIV,   Trax::GETD,    //10,11,12,13
  UNSUPPORTED,   UNSUPPORTED,    Trax::FADD,   UNSUPPORTED,     //14,15,16,17
  Trax::MULI,  Trax::BSRLI,  UNSUPPORTED,    Trax::GET,     //18,19,1A,1B
  UNSUPPORTED,   UNSUPPORTED,    UNSUPPORTED,    UNSUPPORTED,     //1C,1D,1E,1F

  Trax::OR,    Trax::AND,    Trax::XOR,    Trax::ANDN,    //20,21,22,23
  Trax::SEXT8, Trax::MFS,    Trax::BR,     Trax::BEQ,     //24,25,26,27
  Trax::ORI,   Trax::ANDI,   Trax::XORI,   Trax::ANDNI,   //28,29,2A,2B
  Trax::IMM,   Trax::RTSD,   Trax::BRI,    Trax::BEQI,    //2C,2D,2E,2F

  Trax::LBU,   Trax::LHU,    Trax::LW,     UNSUPPORTED,     //30,31,32,33
  Trax::SB,    Trax::SH,     Trax::SW,     UNSUPPORTED,     //34,35,36,37
  Trax::LBUI,  Trax::LHUI,   Trax::LWI,    UNSUPPORTED,     //38,39,3A,3B
  Trax::SBI,   Trax::SHI,    Trax::SWI,    UNSUPPORTED,     //3C,3D,3E,3F
};

static unsigned getRD(uint32_t insn) {
  return TraxRegisterInfo::getRegisterFromNumbering((insn>>21)&0x1F);
}

static unsigned getRA(uint32_t insn) {
  return TraxRegisterInfo::getRegisterFromNumbering((insn>>16)&0x1F);
}

static unsigned getRB(uint32_t insn) {
  return TraxRegisterInfo::getRegisterFromNumbering((insn>>11)&0x1F);
}

static int64_t getRS(uint32_t insn) {
  return TraxRegisterInfo::getSpecialRegisterFromNumbering(insn&0x3FFF);
}

static int64_t getIMM(uint32_t insn) {
    int16_t val = (insn & 0xFFFF);
    return val;
}

static int64_t getSHT(uint32_t insn) {
    int16_t val = (insn & 0x1F);
    return val;
}

static unsigned getFLAGS(int32_t insn) {
    return (insn & 0x7FF);
}

static int64_t getFSL(uint32_t insn) {
    int16_t val = (insn & 0xF);
    return val;
}

static unsigned decodeMUL(uint32_t insn) {
    switch (getFLAGS(insn)) {
    default: return UNSUPPORTED;
    case 0:  return Trax::MUL;
    case 1:  return Trax::MULH;
    case 2:  return Trax::MULHSU;
    case 3:  return Trax::MULHU;
    }
}

static unsigned decodeSEXT(uint32_t insn) {
    switch (insn&0x7FF) {
    default:   return UNSUPPORTED;
    case 0x60: return Trax::SEXT8;
    case 0x68: return Trax::WIC;
    case 0x64: return Trax::WDC;
    case 0x66: return Trax::WDCC;
    case 0x74: return Trax::WDCF;
    case 0x61: return Trax::SEXT16;
    case 0x41: return Trax::SRL;
    case 0x21: return Trax::SRC;
    case 0x01: return Trax::SRA;
    }
}

static unsigned decodeBEQ(uint32_t insn) {
    switch ((insn>>21)&0x1F) {
    default:    return UNSUPPORTED;
    case 0x00:  return Trax::BEQ;
    case 0x10:  return Trax::BEQD;
    case 0x05:  return Trax::BGE;
    case 0x15:  return Trax::BGED;
    case 0x04:  return Trax::BGT;
    case 0x14:  return Trax::BGTD;
    case 0x03:  return Trax::BLE;
    case 0x13:  return Trax::BLED;
    case 0x02:  return Trax::BLT;
    case 0x12:  return Trax::BLTD;
    case 0x01:  return Trax::BNE;
    case 0x11:  return Trax::BNED;
    }
}

static unsigned decodeBEQI(uint32_t insn) {
    switch ((insn>>21)&0x1F) {
    default:    return UNSUPPORTED;
    case 0x00:  return Trax::BEQI;
    case 0x10:  return Trax::BEQID;
    case 0x05:  return Trax::BGEI;
    case 0x15:  return Trax::BGEID;
    case 0x04:  return Trax::BGTI;
    case 0x14:  return Trax::BGTID;
    case 0x03:  return Trax::BLEI;
    case 0x13:  return Trax::BLEID;
    case 0x02:  return Trax::BLTI;
    case 0x12:  return Trax::BLTID;
    case 0x01:  return Trax::BNEI;
    case 0x11:  return Trax::BNEID;
    }
}

static unsigned decodeBR(uint32_t insn) {
    switch ((insn>>16)&0x1F) {
    default:   return UNSUPPORTED;
    case 0x00: return Trax::BR;
    case 0x08: return Trax::BRA;
    case 0x0C: return Trax::BRK;
    case 0x10: return Trax::BRD;
    case 0x14: return Trax::BRLD;
    case 0x18: return Trax::BRAD;
    case 0x1C: return Trax::BRALD;
    }
}

static unsigned decodeBRI(uint32_t insn) {
    switch ((insn>>16)&0x1F) {
    default:   return UNSUPPORTED;
    case 0x00: return Trax::BRI;
    case 0x08: return Trax::BRAI;
    case 0x0C: return Trax::BRKI;
    case 0x10: return Trax::BRID;
    case 0x14: return Trax::BRLID;
    case 0x18: return Trax::BRAID;
    case 0x1C: return Trax::BRALID;
    }
}

static unsigned decodeBSRL(uint32_t insn) {
    switch ((insn>>9)&0x3) {
    default:  return UNSUPPORTED;
    case 0x2: return Trax::BSLL;
    case 0x1: return Trax::BSRA;
    case 0x0: return Trax::BSRL;
    }
}

static unsigned decodeBSRLI(uint32_t insn) {
    switch ((insn>>9)&0x3) {
    default:  return UNSUPPORTED;
    case 0x2: return Trax::BSLLI;
    case 0x1: return Trax::BSRAI;
    case 0x0: return Trax::BSRLI;
    }
}

static unsigned decodeRSUBK(uint32_t insn) {
    switch (getFLAGS(insn)) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::RSUBK;
    case 0x1: return Trax::CMP;
    case 0x3: return Trax::CMPU;
    }
}

static unsigned decodeFADD(uint32_t insn) {
    switch (getFLAGS(insn)) {
    default:    return UNSUPPORTED;
    case 0x000: return Trax::FADD;
    case 0x080: return Trax::FRSUB;
    case 0x100: return Trax::FMUL;
    case 0x180: return Trax::FDIV;
    case 0x200: return Trax::FCMP_UN;
    case 0x210: return Trax::FCMP_LT;
    case 0x220: return Trax::FCMP_EQ;
    case 0x230: return Trax::FCMP_LE;
    case 0x240: return Trax::FCMP_GT;
    case 0x250: return Trax::FCMP_NE;
    case 0x260: return Trax::FCMP_GE;
    case 0x280: return Trax::FLT;
    case 0x300: return Trax::FINT;
    case 0x380: return Trax::FSQRT;
    }
}

static unsigned decodeGET(uint32_t insn) {
    switch ((insn>>10)&0x3F) {
    default:   return UNSUPPORTED;
    case 0x00: return Trax::GET;
    case 0x01: return Trax::EGET;
    case 0x02: return Trax::AGET;
    case 0x03: return Trax::EAGET;
    case 0x04: return Trax::TGET;
    case 0x05: return Trax::TEGET;
    case 0x06: return Trax::TAGET;
    case 0x07: return Trax::TEAGET;
    case 0x08: return Trax::CGET;
    case 0x09: return Trax::ECGET;
    case 0x0A: return Trax::CAGET;
    case 0x0B: return Trax::ECAGET;
    case 0x0C: return Trax::TCGET;
    case 0x0D: return Trax::TECGET;
    case 0x0E: return Trax::TCAGET;
    case 0x0F: return Trax::TECAGET;
    case 0x10: return Trax::NGET;
    case 0x11: return Trax::NEGET;
    case 0x12: return Trax::NAGET;
    case 0x13: return Trax::NEAGET;
    case 0x14: return Trax::TNGET;
    case 0x15: return Trax::TNEGET;
    case 0x16: return Trax::TNAGET;
    case 0x17: return Trax::TNEAGET;
    case 0x18: return Trax::NCGET;
    case 0x19: return Trax::NECGET;
    case 0x1A: return Trax::NCAGET;
    case 0x1B: return Trax::NECAGET;
    case 0x1C: return Trax::TNCGET;
    case 0x1D: return Trax::TNECGET;
    case 0x1E: return Trax::TNCAGET;
    case 0x1F: return Trax::TNECAGET;
    case 0x20: return Trax::PUT;
    case 0x22: return Trax::APUT;
    case 0x24: return Trax::TPUT;
    case 0x26: return Trax::TAPUT;
    case 0x28: return Trax::CPUT;
    case 0x2A: return Trax::CAPUT;
    case 0x2C: return Trax::TCPUT;
    case 0x2E: return Trax::TCAPUT;
    case 0x30: return Trax::NPUT;
    case 0x32: return Trax::NAPUT;
    case 0x34: return Trax::TNPUT;
    case 0x36: return Trax::TNAPUT;
    case 0x38: return Trax::NCPUT;
    case 0x3A: return Trax::NCAPUT;
    case 0x3C: return Trax::TNCPUT;
    case 0x3E: return Trax::TNCAPUT;
    }
}

static unsigned decodeGETD(uint32_t insn) {
    switch ((insn>>5)&0x3F) {
    default:   return UNSUPPORTED;
    case 0x00: return Trax::GETD;
    case 0x01: return Trax::EGETD;
    case 0x02: return Trax::AGETD;
    case 0x03: return Trax::EAGETD;
    case 0x04: return Trax::TGETD;
    case 0x05: return Trax::TEGETD;
    case 0x06: return Trax::TAGETD;
    case 0x07: return Trax::TEAGETD;
    case 0x08: return Trax::CGETD;
    case 0x09: return Trax::ECGETD;
    case 0x0A: return Trax::CAGETD;
    case 0x0B: return Trax::ECAGETD;
    case 0x0C: return Trax::TCGETD;
    case 0x0D: return Trax::TECGETD;
    case 0x0E: return Trax::TCAGETD;
    case 0x0F: return Trax::TECAGETD;
    case 0x10: return Trax::NGETD;
    case 0x11: return Trax::NEGETD;
    case 0x12: return Trax::NAGETD;
    case 0x13: return Trax::NEAGETD;
    case 0x14: return Trax::TNGETD;
    case 0x15: return Trax::TNEGETD;
    case 0x16: return Trax::TNAGETD;
    case 0x17: return Trax::TNEAGETD;
    case 0x18: return Trax::NCGETD;
    case 0x19: return Trax::NECGETD;
    case 0x1A: return Trax::NCAGETD;
    case 0x1B: return Trax::NECAGETD;
    case 0x1C: return Trax::TNCGETD;
    case 0x1D: return Trax::TNECGETD;
    case 0x1E: return Trax::TNCAGETD;
    case 0x1F: return Trax::TNECAGETD;
    case 0x20: return Trax::PUTD;
    case 0x22: return Trax::APUTD;
    case 0x24: return Trax::TPUTD;
    case 0x26: return Trax::TAPUTD;
    case 0x28: return Trax::CPUTD;
    case 0x2A: return Trax::CAPUTD;
    case 0x2C: return Trax::TCPUTD;
    case 0x2E: return Trax::TCAPUTD;
    case 0x30: return Trax::NPUTD;
    case 0x32: return Trax::NAPUTD;
    case 0x34: return Trax::TNPUTD;
    case 0x36: return Trax::TNAPUTD;
    case 0x38: return Trax::NCPUTD;
    case 0x3A: return Trax::NCAPUTD;
    case 0x3C: return Trax::TNCPUTD;
    case 0x3E: return Trax::TNCAPUTD;
    }
}

static unsigned decodeIDIV(uint32_t insn) {
    switch (insn&0x3) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::IDIV;
    case 0x2: return Trax::IDIVU;
    }
}

static unsigned decodeLBU(uint32_t insn) {
    switch ((insn>>9)&0x1) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::LBU;
    case 0x1: return Trax::LBUR;
    }
}

static unsigned decodeLHU(uint32_t insn) {
    switch ((insn>>9)&0x1) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::LHU;
    case 0x1: return Trax::LHUR;
    }
}

static unsigned decodeLW(uint32_t insn) {
    switch ((insn>>9)&0x3) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::LW;
    case 0x1: return Trax::LWR;
    case 0x2: return Trax::LWX;
    }
}

static unsigned decodeSB(uint32_t insn) {
    switch ((insn>>9)&0x1) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::SB;
    case 0x1: return Trax::SBR;
    }
}

static unsigned decodeSH(uint32_t insn) {
    switch ((insn>>9)&0x1) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::SH;
    case 0x1: return Trax::SHR;
    }
}

static unsigned decodeSW(uint32_t insn) {
    switch ((insn>>9)&0x3) {
    default:  return UNSUPPORTED;
    case 0x0: return Trax::SW;
    case 0x1: return Trax::SWR;
    case 0x2: return Trax::SWX;
    }
}

static unsigned decodeMFS(uint32_t insn) {
    switch ((insn>>15)&0x1) {
    default:   return UNSUPPORTED;
    case 0x0:
      switch ((insn>>16)&0x1) {
      default:   return UNSUPPORTED;
      case 0x0: return Trax::MSRSET;
      case 0x1: return Trax::MSRCLR;
      }
    case 0x1:
      switch ((insn>>14)&0x1) {
      default:   return UNSUPPORTED;
      case 0x0: return Trax::MFS;
      case 0x1: return Trax::MTS;
      }
    }
}

static unsigned decodeOR(uint32_t insn) {
    switch (getFLAGS(insn)) {
    default:    return UNSUPPORTED;
    case 0x000: return Trax::OR;
    case 0x400: return Trax::PCMPBF;
    }
}

static unsigned decodeXOR(uint32_t insn) {
    switch (getFLAGS(insn)) {
    default:    return UNSUPPORTED;
    case 0x000: return Trax::XOR;
    case 0x400: return Trax::PCMPEQ;
    }
}

static unsigned decodeANDN(uint32_t insn) {
    switch (getFLAGS(insn)) {
    default:    return UNSUPPORTED;
    case 0x000: return Trax::ANDN;
    case 0x400: return Trax::PCMPNE;
    }
}

static unsigned decodeRTSD(uint32_t insn) {
    switch ((insn>>21)&0x1F) {
    default:   return UNSUPPORTED;
    case 0x10: return Trax::RTSD;
    case 0x11: return Trax::RTID;
    case 0x12: return Trax::RTBD;
    case 0x14: return Trax::RTED;
    }
}

static unsigned getOPCODE(uint32_t insn) {
  unsigned opcode = traxBinary2Opcode[ (insn>>26)&0x3F ];
  switch (opcode) {
  case Trax::MUL:     return decodeMUL(insn);
  case Trax::SEXT8:   return decodeSEXT(insn);
  case Trax::BEQ:     return decodeBEQ(insn);
  case Trax::BEQI:    return decodeBEQI(insn);
  case Trax::BR:      return decodeBR(insn);
  case Trax::BRI:     return decodeBRI(insn);
  case Trax::BSRL:    return decodeBSRL(insn);
  case Trax::BSRLI:   return decodeBSRLI(insn);
  case Trax::RSUBK:   return decodeRSUBK(insn);
  case Trax::FADD:    return decodeFADD(insn);
  case Trax::GET:     return decodeGET(insn);
  case Trax::GETD:    return decodeGETD(insn);
  case Trax::IDIV:    return decodeIDIV(insn);
  case Trax::LBU:     return decodeLBU(insn);
  case Trax::LHU:     return decodeLHU(insn);
  case Trax::LW:      return decodeLW(insn);
  case Trax::SB:      return decodeSB(insn);
  case Trax::SH:      return decodeSH(insn);
  case Trax::SW:      return decodeSW(insn);
  case Trax::MFS:     return decodeMFS(insn);
  case Trax::OR:      return decodeOR(insn);
  case Trax::XOR:     return decodeXOR(insn);
  case Trax::ANDN:    return decodeANDN(insn);
  case Trax::RTSD:    return decodeRTSD(insn);
  default:              return opcode;
  }
}

EDInstInfo *TraxDisassembler::getEDInfo() const {
  return instInfoTrax;
}

//
// Public interface for the disassembler
//

bool TraxDisassembler::getInstruction(MCInst &instr,
                                        uint64_t &size,
                                        const MemoryObject &region,
                                        uint64_t address,
                                        raw_ostream &vStream) const {
  // The machine instruction.
  uint32_t insn;
  uint8_t bytes[4];

  // We always consume 4 bytes of data
  size = 4;

  // We want to read exactly 4 bytes of data.
  if (region.readBytes(address, 4, (uint8_t*)bytes, NULL) == -1)
    return false;

  // Encoded as a big-endian 32-bit word in the stream.
  insn = (bytes[0]<<24) | (bytes[1]<<16) | (bytes[2]<< 8) | (bytes[3]<<0);

  // Get the MCInst opcode from the binary instruction and make sure
  // that it is a valid instruction.
  unsigned opcode = getOPCODE(insn);
  if (opcode == UNSUPPORTED)
    return false;

  instr.setOpcode(opcode);

  uint64_t tsFlags = TraxInsts[opcode].TSFlags;
  switch ((tsFlags & TraxII::FormMask)) {
  default: llvm_unreachable("unknown instruction encoding");

  case TraxII::FRRRR:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateReg(getRB(insn)));
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    break;

  case TraxII::FRRR:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    instr.addOperand(MCOperand::CreateReg(getRB(insn)));
    break;

  case TraxII::FRI:
    switch (opcode) {
    default: llvm_unreachable("unknown instruction encoding");
    case Trax::MFS:
      instr.addOperand(MCOperand::CreateReg(getRD(insn)));
      instr.addOperand(MCOperand::CreateImm(insn&0x3FFF));
      break;
    case Trax::MTS:
      instr.addOperand(MCOperand::CreateImm(insn&0x3FFF));
      instr.addOperand(MCOperand::CreateReg(getRA(insn)));
      break;
    case Trax::MSRSET:
    case Trax::MSRCLR:
      instr.addOperand(MCOperand::CreateReg(getRD(insn)));
      instr.addOperand(MCOperand::CreateImm(insn&0x7FFF));
      break;
    }
    break;

  case TraxII::FRRI:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    switch (opcode) {
    default:
      instr.addOperand(MCOperand::CreateImm(getIMM(insn)));
      break;
    case Trax::BSRLI:
    case Trax::BSRAI:
    case Trax::BSLLI:
      instr.addOperand(MCOperand::CreateImm(insn&0x1F));
      break;
    }
    break;

  case TraxII::FCRR:
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    instr.addOperand(MCOperand::CreateReg(getRB(insn)));
    break;

  case TraxII::FCRI:
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    instr.addOperand(MCOperand::CreateImm(getIMM(insn)));
    break;

  case TraxII::FRCR:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateReg(getRB(insn)));
    break;

  case TraxII::FRCI:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateImm(getIMM(insn)));
    break;

  case TraxII::FCCR:
    instr.addOperand(MCOperand::CreateReg(getRB(insn)));
    break;

  case TraxII::FCCI:
    instr.addOperand(MCOperand::CreateImm(getIMM(insn)));
    break;

  case TraxII::FRRCI:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    instr.addOperand(MCOperand::CreateImm(getSHT(insn)));
    break;

  case TraxII::FRRC:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    break;

  case TraxII::FRCX:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateImm(getFSL(insn)));
    break;

  case TraxII::FRCS:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateReg(getRS(insn)));
    break;

  case TraxII::FCRCS:
    instr.addOperand(MCOperand::CreateReg(getRS(insn)));
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    break;

  case TraxII::FCRCX:
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    instr.addOperand(MCOperand::CreateImm(getFSL(insn)));
    break;

  case TraxII::FCX:
    instr.addOperand(MCOperand::CreateImm(getFSL(insn)));
    break;

  case TraxII::FCR:
    instr.addOperand(MCOperand::CreateReg(getRB(insn)));
    break;

  case TraxII::FRIR:
    instr.addOperand(MCOperand::CreateReg(getRD(insn)));
    instr.addOperand(MCOperand::CreateImm(getIMM(insn)));
    instr.addOperand(MCOperand::CreateReg(getRA(insn)));
    break;
  }

  return true;
}

static MCDisassembler *createTraxDisassembler(const Target &T) {
  return new TraxDisassembler;
}

extern "C" void LLVMInitializeTraxDisassembler() {
  // Register the disassembler.
  TargetRegistry::RegisterMCDisassembler(TheTraxTarget,
                                         createTraxDisassembler);
}
