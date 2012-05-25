//===- TraxIntrinsicInfo.h - Trax Intrinsic Information -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Trax implementation of TargetIntrinsicInfo.
//
//===----------------------------------------------------------------------===//
#ifndef TRAXINTRINSICS_H
#define TRAXINTRINSICS_H

#include "llvm/Target/TargetIntrinsicInfo.h"

namespace llvm {

  class TraxIntrinsicInfo : public TargetIntrinsicInfo {
  public:
    std::string getName(unsigned IntrID, const Type **Tys = 0,
                        unsigned numTys = 0) const;
    unsigned lookupName(const char *Name, unsigned Len) const;
    unsigned lookupGCCName(const char *Name) const;
    bool isOverloaded(unsigned IID) const;
    Function *getDeclaration(Module *M, unsigned ID, const Type **Tys = 0,
                             unsigned numTys = 0) const;
  };

}

#endif
