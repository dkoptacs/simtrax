//===-- TraxMCAsmInfo.cpp - Trax asm properties -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the TraxMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "TraxMCAsmInfo.h"
using namespace llvm;

TraxMCAsmInfo::TraxMCAsmInfo() {
  SupportsDebugInformation    = true;
  AlignmentIsInBytes          = false;
  PrivateGlobalPrefix         = "$";
  GPRel32Directive            = "\t.gpword\t";
}
