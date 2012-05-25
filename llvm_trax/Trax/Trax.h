//===-- Trax.h - Top-level interface for Trax ---------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM Trax back-end.
//
//===----------------------------------------------------------------------===//

#ifndef TARGET_TRAX_H
#define TARGET_TRAX_H

#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class TraxTargetMachine;
  class FunctionPass;
  class MachineCodeEmitter;
  class MCCodeEmitter;
  class TargetAsmBackend;
  class formatted_raw_ostream;

  MCCodeEmitter *createTraxMCCodeEmitter(const Target &,
                                           TargetMachine &TM,
                                           MCContext &Ctx);

  TargetAsmBackend *createTraxAsmBackend(const Target &, const std::string &);

  FunctionPass *createTraxISelDag(TraxTargetMachine &TM);
  FunctionPass *createTraxDelaySlotFillerPass(TraxTargetMachine &TM);

  extern Target TheTraxTarget;
} // end namespace llvm;

// Defines symbolic names for Trax registers.  This defines a mapping from
// register name to register number.
#include "TraxGenRegisterNames.inc"

// Defines symbolic names for the Trax instructions.
#include "TraxGenInstrNames.inc"

#endif
