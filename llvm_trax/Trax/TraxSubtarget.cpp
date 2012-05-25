//===- TraxSubtarget.cpp - Trax Subtarget Information -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Trax specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#include "TraxSubtarget.h"
#include "Trax.h"
#include "TraxGenSubtarget.inc"
#include "llvm/Support/CommandLine.h"
using namespace llvm;

TraxSubtarget::TraxSubtarget(const std::string &TT, const std::string &FS):
  HasPipe3(false), HasBarrel(false), HasDiv(true), HasMul(true),
  HasFSL(false), HasEFSL(false), HasMSRSet(false), HasException(false),
  HasPatCmp(false), HasFPU(true), HasESR(false), HasPVR(false),
  HasMul64(false), HasSqrt(true), HasMMU(false)
{
  std::string CPU = "v400";
  TraxArchVersion = V400;

  // Parse features string.
  ParseSubtargetFeatures(FS, CPU);
}
