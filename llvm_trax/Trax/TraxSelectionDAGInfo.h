//===-- TraxSelectionDAGInfo.h - Trax SelectionDAG Info -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the Trax subclass for TargetSelectionDAGInfo.
//
//===----------------------------------------------------------------------===//

#ifndef TRAXSELECTIONDAGINFO_H
#define TRAXSELECTIONDAGINFO_H

#include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm {

class TraxTargetMachine;

class TraxSelectionDAGInfo : public TargetSelectionDAGInfo {
public:
  explicit TraxSelectionDAGInfo(const TraxTargetMachine &TM);
  ~TraxSelectionDAGInfo();
};

}

#endif
