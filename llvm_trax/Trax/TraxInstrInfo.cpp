//===- TraxInstrInfo.cpp - Trax Instruction Information -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Trax implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "TraxInstrInfo.h"
#include "TraxTargetMachine.h"
#include "TraxMachineFunction.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "TraxGenInstrInfo.inc"

using namespace llvm;

TraxInstrInfo::TraxInstrInfo(TraxTargetMachine &tm)
  : TargetInstrInfoImpl(TraxInsts, array_lengthof(TraxInsts)),
    TM(tm), RI(*TM.getSubtargetImpl(), *this) {}

static bool isZeroImm(const MachineOperand &op) {
  return op.isImm() && op.getImm() == 0;
}

/// isLoadFromStackSlot - If the specified machine instruction is a direct
/// load from a stack slot, return the virtual or physical register number of
/// the destination along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than loading from the stack slot.
unsigned TraxInstrInfo::
isLoadFromStackSlot(const MachineInstr *MI, int &FrameIndex) const {
  if (MI->getOpcode() == Trax::LWI) {
    if ((MI->getOperand(1).isFI()) && // is a stack slot
        (MI->getOperand(2).isImm()) &&  // the imm is zero
        (isZeroImm(MI->getOperand(2)))) {
      FrameIndex = MI->getOperand(1).getIndex();
      return MI->getOperand(0).getReg();
    }
  }

  return 0;
}

/// isStoreToStackSlot - If the specified machine instruction is a direct
/// store to a stack slot, return the virtual or physical register number of
/// the source reg along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than storing to the stack slot.
unsigned TraxInstrInfo::
isStoreToStackSlot(const MachineInstr *MI, int &FrameIndex) const {
  if (MI->getOpcode() == Trax::SWI) {
    if ((MI->getOperand(1).isFI()) && // is a stack slot
        (MI->getOperand(2).isImm()) &&  // the imm is zero
        (isZeroImm(MI->getOperand(2)))) {
      FrameIndex = MI->getOperand(1).getIndex();
      return MI->getOperand(0).getReg();
    }
  }
  return 0;
}

/// insertNoop - If data hazard condition is found insert the target nop
/// instruction.
void TraxInstrInfo::
insertNoop(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI) const {
  DebugLoc DL;
  BuildMI(MBB, MI, DL, get(Trax::NOP));
}

void TraxInstrInfo::
copyPhysReg(MachineBasicBlock &MBB,
            MachineBasicBlock::iterator I, DebugLoc DL,
            unsigned DestReg, unsigned SrcReg,
            bool KillSrc) const {
  llvm::BuildMI(MBB, I, DL, get(Trax::ADDK), DestReg)
    .addReg(SrcReg, getKillRegState(KillSrc)).addReg(Trax::R0);
}

void TraxInstrInfo::
storeRegToStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                    unsigned SrcReg, bool isKill, int FI,
                    const TargetRegisterClass *RC,
                    const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  BuildMI(MBB, I, DL, get(Trax::SWI)).addReg(SrcReg,getKillRegState(isKill))
    .addFrameIndex(FI).addImm(0); //.addFrameIndex(FI);
}

void TraxInstrInfo::
loadRegFromStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                     unsigned DestReg, int FI,
                     const TargetRegisterClass *RC,
                     const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  BuildMI(MBB, I, DL, get(Trax::LWI), DestReg)
      .addFrameIndex(FI).addImm(0); //.addFrameIndex(FI);
}

//===----------------------------------------------------------------------===//
// Branch Analysis
//===----------------------------------------------------------------------===//
bool TraxInstrInfo::AnalyzeBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *&TBB,
                                    MachineBasicBlock *&FBB,
                                    SmallVectorImpl<MachineOperand> &Cond,
                                    bool AllowModify) const {
  // If the block has no terminators, it just falls into the block after it.
  MachineBasicBlock::iterator I = MBB.end();
  if (I == MBB.begin())
    return false;
  --I;
  while (I->isDebugValue()) {
    if (I == MBB.begin())
      return false;
    --I;
  }
  if (!isUnpredicatedTerminator(I))
    return false;

  // Get the last instruction in the block.
  MachineInstr *LastInst = I;

  // If there is only one terminator instruction, process it.
  unsigned LastOpc = LastInst->getOpcode();
  if (I == MBB.begin() || !isUnpredicatedTerminator(--I)) {
    if (Trax::isUncondBranchOpcode(LastOpc)) {
      TBB = LastInst->getOperand(0).getMBB();
      return false;
    }
    if (Trax::isCondBranchOpcode(LastOpc)) {
      // Block ends with fall-through condbranch.
      TBB = LastInst->getOperand(1).getMBB();
      Cond.push_back(MachineOperand::CreateImm(LastInst->getOpcode()));
      Cond.push_back(LastInst->getOperand(0));
      return false;
    }
    // Otherwise, don't know what this is.
    return true;
  }

  // Get the instruction before it if it's a terminator.
  MachineInstr *SecondLastInst = I;

  // If there are three terminators, we don't know what sort of block this is.
  if (SecondLastInst && I != MBB.begin() && isUnpredicatedTerminator(--I))
    return true;

  // If the block ends with something like BEQID then BRID, handle it.
  if (Trax::isCondBranchOpcode(SecondLastInst->getOpcode()) &&
      Trax::isUncondBranchOpcode(LastInst->getOpcode())) {
    TBB = SecondLastInst->getOperand(1).getMBB();
    Cond.push_back(MachineOperand::CreateImm(SecondLastInst->getOpcode()));
    Cond.push_back(SecondLastInst->getOperand(0));
    FBB = LastInst->getOperand(0).getMBB();
    return false;
  }

  // If the block ends with two unconditional branches, handle it.
  // The second one is not executed, so remove it.
  if (Trax::isUncondBranchOpcode(SecondLastInst->getOpcode()) &&
      Trax::isUncondBranchOpcode(LastInst->getOpcode())) {
    TBB = SecondLastInst->getOperand(0).getMBB();
    I = LastInst;
    if (AllowModify)
      I->eraseFromParent();
    return false;
  }

  // Otherwise, can't handle this.
  return true;
}

unsigned TraxInstrInfo::
InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
             MachineBasicBlock *FBB,
             const SmallVectorImpl<MachineOperand> &Cond,
             DebugLoc DL) const {
  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 2 || Cond.size() == 0) &&
         "Trax branch conditions have two components!");

  unsigned Opc = Trax::BRID;
  if (!Cond.empty())
    Opc = (unsigned)Cond[0].getImm();

  if (FBB == 0) {
    if (Cond.empty()) // Unconditional branch
      BuildMI(&MBB, DL, get(Opc)).addMBB(TBB);
    else              // Conditional branch
      BuildMI(&MBB, DL, get(Opc)).addReg(Cond[1].getReg()).addMBB(TBB);
    return 1;
  }

  BuildMI(&MBB, DL, get(Opc)).addReg(Cond[1].getReg()).addMBB(TBB);
  BuildMI(&MBB, DL, get(Trax::BRID)).addMBB(FBB);
  return 2;
}

unsigned TraxInstrInfo::RemoveBranch(MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator I = MBB.end();
  if (I == MBB.begin()) return 0;
  --I;
  while (I->isDebugValue()) {
    if (I == MBB.begin())
      return 0;
    --I;
  }

  if (!Trax::isUncondBranchOpcode(I->getOpcode()) &&
      !Trax::isCondBranchOpcode(I->getOpcode()))
    return 0;

  // Remove the branch.
  I->eraseFromParent();

  I = MBB.end();

  if (I == MBB.begin()) return 1;
  --I;
  if (!Trax::isCondBranchOpcode(I->getOpcode()))
    return 1;

  // Remove the branch.
  I->eraseFromParent();
  return 2;
}

bool TraxInstrInfo::ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 2 && "Invalid Trax branch opcode!");
  switch (Cond[0].getImm()) {
  default:            return true;
  case Trax::BEQ:   Cond[0].setImm(Trax::BNE); return false;
  case Trax::BNE:   Cond[0].setImm(Trax::BEQ); return false;
  case Trax::BGT:   Cond[0].setImm(Trax::BLE); return false;
  case Trax::BGE:   Cond[0].setImm(Trax::BLT); return false;
  case Trax::BLT:   Cond[0].setImm(Trax::BGE); return false;
  case Trax::BLE:   Cond[0].setImm(Trax::BGT); return false;
  case Trax::BEQI:  Cond[0].setImm(Trax::BNEI); return false;
  case Trax::BNEI:  Cond[0].setImm(Trax::BEQI); return false;
  case Trax::BGTI:  Cond[0].setImm(Trax::BLEI); return false;
  case Trax::BGEI:  Cond[0].setImm(Trax::BLTI); return false;
  case Trax::BLTI:  Cond[0].setImm(Trax::BGEI); return false;
  case Trax::BLEI:  Cond[0].setImm(Trax::BGTI); return false;
  case Trax::BEQD:  Cond[0].setImm(Trax::BNED); return false;
  case Trax::BNED:  Cond[0].setImm(Trax::BEQD); return false;
  case Trax::BGTD:  Cond[0].setImm(Trax::BLED); return false;
  case Trax::BGED:  Cond[0].setImm(Trax::BLTD); return false;
  case Trax::BLTD:  Cond[0].setImm(Trax::BGED); return false;
  case Trax::BLED:  Cond[0].setImm(Trax::BGTD); return false;
  case Trax::BEQID: Cond[0].setImm(Trax::BNEID); return false;
  case Trax::BNEID: Cond[0].setImm(Trax::BEQID); return false;
  case Trax::BGTID: Cond[0].setImm(Trax::BLEID); return false;
  case Trax::BGEID: Cond[0].setImm(Trax::BLTID); return false;
  case Trax::BLTID: Cond[0].setImm(Trax::BGEID); return false;
  case Trax::BLEID: Cond[0].setImm(Trax::BGTID); return false;
  }
}

/// getGlobalBaseReg - Return a virtual register initialized with the
/// the global base register value. Output instructions required to
/// initialize the register in the function entry block, if necessary.
///
unsigned TraxInstrInfo::getGlobalBaseReg(MachineFunction *MF) const {
  TraxFunctionInfo *TraxFI = MF->getInfo<TraxFunctionInfo>();
  unsigned GlobalBaseReg = TraxFI->getGlobalBaseReg();
  if (GlobalBaseReg != 0)
    return GlobalBaseReg;

  // Insert the set of GlobalBaseReg into the first MBB of the function
  MachineBasicBlock &FirstMBB = MF->front();
  MachineBasicBlock::iterator MBBI = FirstMBB.begin();
  MachineRegisterInfo &RegInfo = MF->getRegInfo();
  const TargetInstrInfo *TII = MF->getTarget().getInstrInfo();

  GlobalBaseReg = RegInfo.createVirtualRegister(Trax::GPRRegisterClass);
  BuildMI(FirstMBB, MBBI, DebugLoc(), TII->get(TargetOpcode::COPY),
          GlobalBaseReg).addReg(Trax::R20);
  RegInfo.addLiveIn(Trax::R20);

  TraxFI->setGlobalBaseReg(GlobalBaseReg);
  return GlobalBaseReg;
}
