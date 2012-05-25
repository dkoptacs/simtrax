//===-- TraxSelectionDAGInfo.cpp - Trax SelectionDAG Info -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the TraxSelectionDAGInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "trax-selectiondag-info"
#include "TraxTargetMachine.h"
using namespace llvm;

TraxSelectionDAGInfo::TraxSelectionDAGInfo(const TraxTargetMachine &TM)
  : TargetSelectionDAGInfo(TM) {
}

TraxSelectionDAGInfo::~TraxSelectionDAGInfo() {
}
