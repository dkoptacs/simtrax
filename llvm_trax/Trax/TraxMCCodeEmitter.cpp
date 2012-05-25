//===-- TraxMCCodeEmitter.cpp - Convert Trax code to machine code -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the TraxMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "mccodeemitter"
#include "Trax.h"
#include "TraxInstrInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");

namespace {
class TraxMCCodeEmitter : public MCCodeEmitter {
  TraxMCCodeEmitter(const TraxMCCodeEmitter &); // DO NOT IMPLEMENT
  void operator=(const TraxMCCodeEmitter &); // DO NOT IMPLEMENT
  const TargetMachine &TM;
  const TargetInstrInfo &TII;
  MCContext &Ctx;

public:
  TraxMCCodeEmitter(TargetMachine &tm, MCContext &ctx)
    : TM(tm), TII(*TM.getInstrInfo()), Ctx(ctx) {
  }

  ~TraxMCCodeEmitter() {}

  // getBinaryCodeForInstr - TableGen'erated function for getting the
  // binary encoding for an instruction.
  unsigned getBinaryCodeForInstr(const MCInst &MI) const;

  /// getMachineOpValue - Return binary encoding of operand. If the machine
  /// operand requires relocation, record the relocation and return zero.
  unsigned getMachineOpValue(const MCInst &MI,const MCOperand &MO) const;
  unsigned getMachineOpValue(const MCInst &MI, unsigned OpIdx) const {
    return getMachineOpValue(MI, MI.getOperand(OpIdx));
  }

  static unsigned GetTraxRegNum(const MCOperand &MO) {
    // FIXME: getTraxRegisterNumbering() is sufficient?
    assert(0 && "TraxMCCodeEmitter::GetTraxRegNum() not yet implemented.");
    return 0;
  }

  void EmitByte(unsigned char C, unsigned &CurByte, raw_ostream &OS) const {
    // The MicroBlaze uses a bit reversed format so we need to reverse the
    // order of the bits. Taken from:
    // http://graphics.stanford.edu/~seander/bithacks.html
    C = ((C * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;

    OS << (char)C;
    ++CurByte;
  }

  void EmitRawByte(unsigned char C, unsigned &CurByte, raw_ostream &OS) const {
    OS << (char)C;
    ++CurByte;
  }

  void EmitConstant(uint64_t Val, unsigned Size, unsigned &CurByte,
                    raw_ostream &OS) const {
    assert(Size <= 8 && "size too big in emit constant");

    for (unsigned i = 0; i != Size; ++i) {
      EmitByte(Val & 255, CurByte, OS);
      Val >>= 8;
    }
  }

  void EmitIMM(const MCOperand &imm, unsigned &CurByte, raw_ostream &OS) const;
  void EmitIMM(const MCInst &MI, unsigned &CurByte, raw_ostream &OS) const;

  void EmitImmediate(const MCInst &MI, unsigned opNo, bool pcrel,
                     unsigned &CurByte, raw_ostream &OS,
                     SmallVectorImpl<MCFixup> &Fixups) const;

  void EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups) const;
};

} // end anonymous namespace


MCCodeEmitter *llvm::createTraxMCCodeEmitter(const Target &,
                                               TargetMachine &TM,
                                               MCContext &Ctx) {
  return new TraxMCCodeEmitter(TM, Ctx);
}

/// getMachineOpValue - Return binary encoding of operand. If the machine
/// operand requires relocation, record the relocation and return zero.
unsigned TraxMCCodeEmitter::getMachineOpValue(const MCInst &MI,
                                             const MCOperand &MO) const {
  if (MO.isReg())
    return TraxRegisterInfo::getRegisterNumbering(MO.getReg());
  else if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());
  else if (MO.isExpr())
      return 0; // The relocation has already been recorded at this point.
  else {
#ifndef NDEBUG
    errs() << MO;
#endif
    llvm_unreachable(0);
  }
  return 0;
}

void TraxMCCodeEmitter::
EmitIMM(const MCOperand &imm, unsigned &CurByte, raw_ostream &OS) const {
  int32_t val = (int32_t)imm.getImm();
  if (val > 32767 || val < -32768) {
    EmitByte(0x0D, CurByte, OS);
    EmitByte(0x00, CurByte, OS);
    EmitRawByte((val >> 24) & 0xFF, CurByte, OS);
    EmitRawByte((val >> 16) & 0xFF, CurByte, OS);
  }
}

void TraxMCCodeEmitter::
EmitIMM(const MCInst &MI, unsigned &CurByte,raw_ostream &OS) const {
  switch (MI.getOpcode()) {
  default: break;

  case Trax::ADDIK32:
  case Trax::ORI32:
  case Trax::BRLID32:
    EmitByte(0x0D, CurByte, OS);
    EmitByte(0x00, CurByte, OS);
    EmitRawByte(0, CurByte, OS);
    EmitRawByte(0, CurByte, OS);
  }
}

void TraxMCCodeEmitter::
EmitImmediate(const MCInst &MI, unsigned opNo, bool pcrel, unsigned &CurByte,
              raw_ostream &OS, SmallVectorImpl<MCFixup> &Fixups) const {
  assert(MI.getNumOperands()>opNo && "Not enought operands for instruction");

  MCOperand oper = MI.getOperand(opNo);

  if (oper.isImm()) {
    EmitIMM(oper, CurByte, OS);
  } else if (oper.isExpr()) {
    MCFixupKind FixupKind;
    switch (MI.getOpcode()) {
    default:
      FixupKind = pcrel ? FK_PCRel_2 : FK_Data_2;
      Fixups.push_back(MCFixup::Create(0,oper.getExpr(),FixupKind));
      break;
    case Trax::ORI32:
    case Trax::ADDIK32:
    case Trax::BRLID32:
      FixupKind = pcrel ? FK_PCRel_4 : FK_Data_4;
      Fixups.push_back(MCFixup::Create(0,oper.getExpr(),FixupKind));
      break;
    }
  }
}



void TraxMCCodeEmitter::
EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                  SmallVectorImpl<MCFixup> &Fixups) const {
  unsigned Opcode = MI.getOpcode();
  const TargetInstrDesc &Desc = TII.get(Opcode);
  uint64_t TSFlags = Desc.TSFlags;
  // Keep track of the current byte being emitted.
  unsigned CurByte = 0;

  // Emit an IMM instruction if the instruction we are encoding requires it
  EmitIMM(MI,CurByte,OS);

  switch ((TSFlags & TraxII::FormMask)) {
  default: break;
  case TraxII::FPseudo:
    // Pseudo instructions don't get encoded.
    return;
  case TraxII::FRRI:
    EmitImmediate(MI, 2, false, CurByte, OS, Fixups);
    break;
  case TraxII::FRIR:
    EmitImmediate(MI, 1, false, CurByte, OS, Fixups);
    break;
  case TraxII::FCRI:
    EmitImmediate(MI, 1, true, CurByte, OS, Fixups);
    break;
  case TraxII::FRCI:
    EmitImmediate(MI, 1, true, CurByte, OS, Fixups);
  case TraxII::FCCI:
    EmitImmediate(MI, 0, true, CurByte, OS, Fixups);
    break;
  }

  ++MCNumEmitted;  // Keep track of the # of mi's emitted
  unsigned Value = getBinaryCodeForInstr(MI);
  EmitConstant(Value, 4, CurByte, OS);
}

// FIXME: These #defines shouldn't be necessary. Instead, tblgen should
// be able to generate code emitter helpers for either variant, like it
// does for the AsmWriter.
#define TraxCodeEmitter TraxMCCodeEmitter
#define MachineInstr MCInst
#include "TraxGenCodeEmitter.inc"
#undef TraxCodeEmitter
#undef MachineInstr
