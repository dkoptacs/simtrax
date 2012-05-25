//===- TraxDisassembler.h - Disassembler for MicroBlaze  ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the Trax Disassembler. It it the header for
// TraxDisassembler, a subclass of MCDisassembler.
//
//===----------------------------------------------------------------------===//

#ifndef TRAXDISASSEMBLER_H
#define TRAXDISASSEMBLER_H

#include "llvm/MC/MCDisassembler.h"

struct InternalInstruction;

namespace llvm {
  
class MCInst;
class MemoryObject;
class raw_ostream;

struct EDInstInfo;
  
/// TraxDisassembler - Disassembler for all Trax platforms.
class TraxDisassembler : public MCDisassembler {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  TraxDisassembler() :
    MCDisassembler() {
  }

  ~TraxDisassembler() {
  }

  /// getInstruction - See MCDisassembler.
  bool getInstruction(MCInst &instr,
                      uint64_t &size,
                      const MemoryObject &region,
                      uint64_t address,
                      raw_ostream &vStream) const;

  /// getEDInfo - See MCDisassembler.
  EDInstInfo *getEDInfo() const;
};

} // namespace llvm
  
#endif
