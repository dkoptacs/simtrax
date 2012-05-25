//===-- TraxTargetInfo.cpp - Trax Target Implementation ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Trax.h"
#include "llvm/Module.h"
#include "llvm/Target/TargetRegistry.h"
using namespace llvm;

Target llvm::TheTraxTarget;

extern "C" void LLVMInitializeTraxTargetInfo() {
  RegisterTarget<Triple::trax> X(TheTraxTarget, "trax", "Trax");
}
