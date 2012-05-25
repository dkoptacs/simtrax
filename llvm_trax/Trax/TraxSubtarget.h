//=====-- TraxSubtarget.h - Define Subtarget for the Trax -*- C++ -*--====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Trax specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#ifndef TRAXSUBTARGET_H
#define TRAXSUBTARGET_H

#include "llvm/Target/TargetSubtarget.h"
#include "llvm/Target/TargetMachine.h"

#include <string>

namespace llvm {

class TraxSubtarget : public TargetSubtarget {

protected:

  enum TraxArchEnum {
    V400, V500, V600, V700, V710
  };

  // Trax architecture version
  TraxArchEnum TraxArchVersion;

  bool HasPipe3;
  bool HasBarrel;
  bool HasDiv;
  bool HasMul;
  bool HasFSL;
  bool HasEFSL;
  bool HasMSRSet;
  bool HasException;
  bool HasPatCmp;
  bool HasFPU;
  bool HasESR;
  bool HasPVR;
  bool HasMul64;
  bool HasSqrt;
  bool HasMMU;

  InstrItineraryData InstrItins;

public:

  /// This constructor initializes the data members to match that
  /// of the specified triple.
  TraxSubtarget(const std::string &TT, const std::string &FS);

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  std::string ParseSubtargetFeatures(const std::string &FS,
                                     const std::string &CPU);

  bool hasFPU()    const { return HasFPU; }
  bool hasSqrt()   const { return HasSqrt; }
  bool hasMul()    const { return HasMul; }
  bool hasMul64()  const { return HasMul64; }
  bool hasDiv()    const { return HasDiv; }
  bool hasBarrel() const { return HasBarrel; }

  bool isV400() const { return TraxArchVersion == V400; }
  bool isV500() const { return TraxArchVersion == V500; }
  bool isV600() const { return TraxArchVersion == V600; }
  bool isV700() const { return TraxArchVersion == V700; }
  bool isV710() const { return TraxArchVersion == V710; }
};
} // End llvm namespace

#endif
