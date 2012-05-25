//=====-- TraxMCAsmInfo.h - Trax asm properties -----------*- C++ -*--====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the TraxMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef TRAXTARGETASMINFO_H
#define TRAXTARGETASMINFO_H

#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmInfo.h"

namespace llvm {
  class Target;

  class TraxMCAsmInfo : public MCAsmInfo {
  public:
    explicit TraxMCAsmInfo();
  };

} // namespace llvm

#endif
