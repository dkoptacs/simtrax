//===-- TraxTargetMachine.h - Define TargetMachine for Trax --- C++ ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Trax specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef TRAX_TARGETMACHINE_H
#define TRAX_TARGETMACHINE_H

#include "TraxSubtarget.h"
#include "TraxInstrInfo.h"
#include "TraxISelLowering.h"
#include "TraxSelectionDAGInfo.h"
#include "TraxIntrinsicInfo.h"
#include "TraxFrameLowering.h"
#include "TraxELFWriterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {
  class formatted_raw_ostream;

  class TraxTargetMachine : public LLVMTargetMachine {
    TraxSubtarget        Subtarget;
    const TargetData       DataLayout; // Calculates type size & alignment
    TraxInstrInfo        InstrInfo;
    TraxFrameLowering    FrameLowering;
    TraxTargetLowering   TLInfo;
    TraxSelectionDAGInfo TSInfo;
    TraxIntrinsicInfo    IntrinsicInfo;
    TraxELFWriterInfo    ELFWriterInfo;
  public:
    TraxTargetMachine(const Target &T, const std::string &TT,
                      const std::string &FS);

    virtual const TraxInstrInfo *getInstrInfo() const
    { return &InstrInfo; }

    virtual const TargetFrameLowering *getFrameLowering() const
    { return &FrameLowering; }

    virtual const TraxSubtarget *getSubtargetImpl() const
    { return &Subtarget; }

    virtual const TargetData *getTargetData() const
    { return &DataLayout;}

    virtual const TraxRegisterInfo *getRegisterInfo() const
    { return &InstrInfo.getRegisterInfo(); }

    virtual const TraxTargetLowering *getTargetLowering() const
    { return &TLInfo; }

    virtual const TraxSelectionDAGInfo* getSelectionDAGInfo() const
    { return &TSInfo; }

    const TargetIntrinsicInfo *getIntrinsicInfo() const
    { return &IntrinsicInfo; }

    virtual const TraxELFWriterInfo *getELFWriterInfo() const {
      return &ELFWriterInfo;
    }

    // Pass Pipeline Configuration
    virtual bool addInstSelector(PassManagerBase &PM, CodeGenOpt::Level Opt);
    virtual bool addPreEmitPass(PassManagerBase &PM,CodeGenOpt::Level Opt);
  };
} // End llvm namespace

#endif
