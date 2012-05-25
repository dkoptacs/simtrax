//===- TraxRegisterInfo.cpp - Trax Register Information -== -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Trax implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "trax-frame-info"

#include "Trax.h"
#include "TraxSubtarget.h"
#include "TraxRegisterInfo.h"
#include "TraxMachineFunction.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineLocation.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"

using namespace llvm;

TraxRegisterInfo::
TraxRegisterInfo(const TraxSubtarget &ST, const TargetInstrInfo &tii)
  : TraxGenRegisterInfo(Trax::ADJCALLSTACKDOWN, Trax::ADJCALLSTACKUP),
    Subtarget(ST), TII(tii) {}

/// getRegisterNumbering - Given the enum value for some register, e.g.
/// Trax::R0, return the number that it corresponds to (e.g. 0).
unsigned TraxRegisterInfo::getRegisterNumbering(unsigned RegEnum) {
  switch (RegEnum) {
    case Trax::R0     : return 0;
    case Trax::R1     : return 1;
    case Trax::R2     : return 2;
    case Trax::R3     : return 3;
    case Trax::R4     : return 4;
    case Trax::R5     : return 5;
    case Trax::R6     : return 6;
    case Trax::R7     : return 7;
    case Trax::R8     : return 8;
    case Trax::R9     : return 9;
    case Trax::R10    : return 10;
    case Trax::R11    : return 11;
    case Trax::R12    : return 12;
    case Trax::R13    : return 13;
    case Trax::R14    : return 14;
    case Trax::R15    : return 15;
    case Trax::R16    : return 16;
    case Trax::R17    : return 17;
    case Trax::R18    : return 18;
    case Trax::R19    : return 19;
    case Trax::R20    : return 20;
    case Trax::R21    : return 21;
    case Trax::R22    : return 22;
    case Trax::R23    : return 23;
    case Trax::R24    : return 24;
    case Trax::R25    : return 25;
    case Trax::R26    : return 26;
    case Trax::R27    : return 27;
    case Trax::R28    : return 28;
    case Trax::R29    : return 29;
    case Trax::R30    : return 30;
    case Trax::R31    : return 31;
    case Trax::RPC    : return 0x0000;
    case Trax::RMSR   : return 0x0001;
    case Trax::REAR   : return 0x0003;
    case Trax::RESR   : return 0x0005;
    case Trax::RFSR   : return 0x0007;
    case Trax::RBTR   : return 0x000B;
    case Trax::REDR   : return 0x000D;
    case Trax::RPID   : return 0x1000;
    case Trax::RZPR   : return 0x1001;
    case Trax::RTLBX  : return 0x1002;
    case Trax::RTLBLO : return 0x1003;
    case Trax::RTLBHI : return 0x1004;
    case Trax::RPVR0  : return 0x2000;
    case Trax::RPVR1  : return 0x2001;
    case Trax::RPVR2  : return 0x2002;
    case Trax::RPVR3  : return 0x2003;
    case Trax::RPVR4  : return 0x2004;
    case Trax::RPVR5  : return 0x2005;
    case Trax::RPVR6  : return 0x2006;
    case Trax::RPVR7  : return 0x2007;
    case Trax::RPVR8  : return 0x2008;
    case Trax::RPVR9  : return 0x2009;
    case Trax::RPVR10 : return 0x200A;
    case Trax::RPVR11 : return 0x200B;
    default: llvm_unreachable("Unknown register number!");
  }
  return 0; // Not reached
}

/// getRegisterFromNumbering - Given the enum value for some register, e.g.
/// Trax::R0, return the number that it corresponds to (e.g. 0).
unsigned TraxRegisterInfo::getRegisterFromNumbering(unsigned Reg) {
  switch (Reg) {
    case 0  : return Trax::R0;
    case 1  : return Trax::R1;
    case 2  : return Trax::R2;
    case 3  : return Trax::R3;
    case 4  : return Trax::R4;
    case 5  : return Trax::R5;
    case 6  : return Trax::R6;
    case 7  : return Trax::R7;
    case 8  : return Trax::R8;
    case 9  : return Trax::R9;
    case 10 : return Trax::R10;
    case 11 : return Trax::R11;
    case 12 : return Trax::R12;
    case 13 : return Trax::R13;
    case 14 : return Trax::R14;
    case 15 : return Trax::R15;
    case 16 : return Trax::R16;
    case 17 : return Trax::R17;
    case 18 : return Trax::R18;
    case 19 : return Trax::R19;
    case 20 : return Trax::R20;
    case 21 : return Trax::R21;
    case 22 : return Trax::R22;
    case 23 : return Trax::R23;
    case 24 : return Trax::R24;
    case 25 : return Trax::R25;
    case 26 : return Trax::R26;
    case 27 : return Trax::R27;
    case 28 : return Trax::R28;
    case 29 : return Trax::R29;
    case 30 : return Trax::R30;
    case 31 : return Trax::R31;
    default: llvm_unreachable("Unknown register number!");
  }
  return 0; // Not reached
}

unsigned TraxRegisterInfo::getSpecialRegisterFromNumbering(unsigned Reg) {
  switch (Reg) {
    case 0x0000 : return Trax::RPC;
    case 0x0001 : return Trax::RMSR;
    case 0x0003 : return Trax::REAR;
    case 0x0005 : return Trax::RESR;
    case 0x0007 : return Trax::RFSR;
    case 0x000B : return Trax::RBTR;
    case 0x000D : return Trax::REDR;
    case 0x1000 : return Trax::RPID;
    case 0x1001 : return Trax::RZPR;
    case 0x1002 : return Trax::RTLBX;
    case 0x1003 : return Trax::RTLBLO;
    case 0x1004 : return Trax::RTLBHI;
    case 0x2000 : return Trax::RPVR0;
    case 0x2001 : return Trax::RPVR1;
    case 0x2002 : return Trax::RPVR2;
    case 0x2003 : return Trax::RPVR3;
    case 0x2004 : return Trax::RPVR4;
    case 0x2005 : return Trax::RPVR5;
    case 0x2006 : return Trax::RPVR6;
    case 0x2007 : return Trax::RPVR7;
    case 0x2008 : return Trax::RPVR8;
    case 0x2009 : return Trax::RPVR9;
    case 0x200A : return Trax::RPVR10;
    case 0x200B : return Trax::RPVR11;
    default: llvm_unreachable("Unknown register number!");
  }
  return 0; // Not reached
}

unsigned TraxRegisterInfo::getPICCallReg() {
  return Trax::R20;
}

//===----------------------------------------------------------------------===//
// Callee Saved Registers methods
//===----------------------------------------------------------------------===//

/// Trax Callee Saved Registers
const unsigned* TraxRegisterInfo::
getCalleeSavedRegs(const MachineFunction *MF) const {
  // Trax callee-save register range is R20 - R31
  static const unsigned CalleeSavedRegs[] = {
    Trax::R20, Trax::R21, Trax::R22, Trax::R23,
    Trax::R24, Trax::R25, Trax::R26, Trax::R27,
    Trax::R28, Trax::R29, Trax::R30, Trax::R31,
    0
  };

  return CalleeSavedRegs;
}

BitVector TraxRegisterInfo::
getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(Trax::R0);
  Reserved.set(Trax::R1);
  Reserved.set(Trax::R2);
  Reserved.set(Trax::R13);
  Reserved.set(Trax::R14);
  Reserved.set(Trax::R15);
  Reserved.set(Trax::R16);
  Reserved.set(Trax::R17);
  Reserved.set(Trax::R18);
  Reserved.set(Trax::R19);
  return Reserved;
}

// This function eliminate ADJCALLSTACKDOWN/ADJCALLSTACKUP pseudo instructions
void TraxRegisterInfo::
eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I) const {
  const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();

  if (!TFI->hasReservedCallFrame(MF)) {
    // If we have a frame pointer, turn the adjcallstackup instruction into a
    // 'addi r1, r1, -<amt>' and the adjcallstackdown instruction into
    // 'addi r1, r1, <amt>'
    MachineInstr *Old = I;
    int Amount = Old->getOperand(0).getImm() + 4;
    if (Amount != 0) {
      // We need to keep the stack aligned properly.  To do this, we round the
      // amount of space needed for the outgoing arguments up to the next
      // alignment boundary.
      unsigned Align = TFI->getStackAlignment();
      Amount = (Amount+Align-1)/Align*Align;

      MachineInstr *New;
      if (Old->getOpcode() == Trax::ADJCALLSTACKDOWN) {
        New = BuildMI(MF,Old->getDebugLoc(),TII.get(Trax::ADDIK),Trax::R1)
                .addReg(Trax::R1).addImm(-Amount);
      } else {
        assert(Old->getOpcode() == Trax::ADJCALLSTACKUP);
        New = BuildMI(MF,Old->getDebugLoc(),TII.get(Trax::ADDIK),Trax::R1)
                .addReg(Trax::R1).addImm(Amount);
      }

      // Replace the pseudo instruction with a new instruction...
      MBB.insert(I, New);
    }
  }

  // Simply discard ADJCALLSTACKDOWN, ADJCALLSTACKUP instructions.
  MBB.erase(I);
}

// FrameIndex represent objects inside a abstract stack.
// We must replace FrameIndex with an stack/frame pointer
// direct reference.
void TraxRegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                    RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineFrameInfo *MFI = MF.getFrameInfo();

  unsigned i = 0;
  while (!MI.getOperand(i).isFI()) {
    ++i;
    assert(i < MI.getNumOperands() &&
           "Instr doesn't have FrameIndex operand!");
  }

  unsigned oi = i == 2 ? 1 : 2;

  DEBUG(dbgs() << "\nFunction : " << MF.getFunction()->getName() << "\n";
        dbgs() << "<--------->\n" << MI);

  int FrameIndex = MI.getOperand(i).getIndex();
  int stackSize  = MFI->getStackSize();
  int spOffset   = MFI->getObjectOffset(FrameIndex);

  DEBUG(TraxFunctionInfo *TraxFI = MF.getInfo<TraxFunctionInfo>();
        dbgs() << "FrameIndex : " << FrameIndex << "\n"
               << "spOffset   : " << spOffset << "\n"
               << "stackSize  : " << stackSize << "\n"
               << "isFixed    : " << MFI->isFixedObjectIndex(FrameIndex) << "\n"
               << "isLiveIn   : " << TraxFI->isLiveIn(FrameIndex) << "\n"
               << "isSpill    : " << MFI->isSpillSlotObjectIndex(FrameIndex)
               << "\n" );

  // as explained on LowerFormalArguments, detect negative offsets
  // and adjust SPOffsets considering the final stack size.
  int Offset = (spOffset < 0) ? (stackSize - spOffset) : spOffset;
  Offset += MI.getOperand(oi).getImm();

  DEBUG(dbgs() << "Offset     : " << Offset << "\n" << "<--------->\n");

  MI.getOperand(oi).ChangeToImmediate(Offset);
  MI.getOperand(i).ChangeToRegister(getFrameRegister(MF), false);
}

void TraxRegisterInfo::
processFunctionBeforeFrameFinalized(MachineFunction &MF) const {
  // Set the stack offset where GP must be saved/loaded from.
  MachineFrameInfo *MFI = MF.getFrameInfo();
  TraxFunctionInfo *TraxFI = MF.getInfo<TraxFunctionInfo>();
  if (TraxFI->needGPSaveRestore())
    MFI->setObjectOffset(TraxFI->getGPFI(), TraxFI->getGPStackOffset());
}

unsigned TraxRegisterInfo::getRARegister() const {
  return Trax::R15;
}

unsigned TraxRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();

  return TFI->hasFP(MF) ? Trax::R19 : Trax::R1;
}

unsigned TraxRegisterInfo::getEHExceptionRegister() const {
  llvm_unreachable("What is the exception register");
  return 0;
}

unsigned TraxRegisterInfo::getEHHandlerRegister() const {
  llvm_unreachable("What is the exception handler register");
  return 0;
}

int TraxRegisterInfo::getDwarfRegNum(unsigned RegNo, bool isEH) const {
  return TraxGenRegisterInfo::getDwarfRegNumFull(RegNo,0);
}

#include "TraxGenRegisterInfo.inc"

