//===-- TraxELFWriterInfo.cpp - ELF Writer Info for the Trax backend --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements ELF writer information for the Trax backend.
//
//===----------------------------------------------------------------------===//

#include "TraxELFWriterInfo.h"
#include "TraxRelocations.h"
#include "llvm/Function.h"
#include "llvm/Support/ELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
//  Implementation of the TraxELFWriterInfo class
//===----------------------------------------------------------------------===//

TraxELFWriterInfo::TraxELFWriterInfo(TargetMachine &TM)
  : TargetELFWriterInfo(TM.getTargetData()->getPointerSizeInBits() == 64,
                        TM.getTargetData()->isLittleEndian()) {
}

TraxELFWriterInfo::~TraxELFWriterInfo() {}

unsigned TraxELFWriterInfo::getRelocationType(unsigned MachineRelTy) const {
  switch (MachineRelTy) {
  case Trax::reloc_pcrel_word:
    return ELF::R_MICROBLAZE_64_PCREL;
  case Trax::reloc_absolute_word:
    return ELF::R_MICROBLAZE_NONE;
  default:
    llvm_unreachable("unknown trax machine relocation type");
  }
  return 0;
}

long int TraxELFWriterInfo::getDefaultAddendForRelTy(unsigned RelTy,
                                                    long int Modifier) const {
  switch (RelTy) {
  case ELF::R_MICROBLAZE_32_PCREL:
    return Modifier - 4;
  case ELF::R_MICROBLAZE_32:
    return Modifier;
  default:
    llvm_unreachable("unknown trax relocation type");
  }
  return 0;
}

unsigned TraxELFWriterInfo::getRelocationTySize(unsigned RelTy) const {
  // FIXME: Most of these sizes are guesses based on the name
  switch (RelTy) {
  case ELF::R_MICROBLAZE_32:
  case ELF::R_MICROBLAZE_32_PCREL:
  case ELF::R_MICROBLAZE_32_PCREL_LO:
  case ELF::R_MICROBLAZE_32_LO:
  case ELF::R_MICROBLAZE_SRO32:
  case ELF::R_MICROBLAZE_SRW32:
  case ELF::R_MICROBLAZE_32_SYM_OP_SYM:
  case ELF::R_MICROBLAZE_GOTOFF_32:
    return 32;

  case ELF::R_MICROBLAZE_64_PCREL:
  case ELF::R_MICROBLAZE_64:
  case ELF::R_MICROBLAZE_GOTPC_64:
  case ELF::R_MICROBLAZE_GOT_64:
  case ELF::R_MICROBLAZE_PLT_64:
  case ELF::R_MICROBLAZE_GOTOFF_64:
    return 64;
  }

  return 0;
}

bool TraxELFWriterInfo::isPCRelativeRel(unsigned RelTy) const {
  // FIXME: Most of these are guesses based on the name
  switch (RelTy) {
  case ELF::R_MICROBLAZE_32_PCREL:
  case ELF::R_MICROBLAZE_64_PCREL:
  case ELF::R_MICROBLAZE_32_PCREL_LO:
  case ELF::R_MICROBLAZE_GOTPC_64:
    return true;
  }

  return false;
}

unsigned TraxELFWriterInfo::getAbsoluteLabelMachineRelTy() const {
  return Trax::reloc_absolute_word;
}

long int TraxELFWriterInfo::computeRelocation(unsigned SymOffset,
                                                unsigned RelOffset,
                                                unsigned RelTy) const {
  if (RelTy == ELF::R_MICROBLAZE_32_PCREL || ELF::R_MICROBLAZE_64_PCREL)
    return SymOffset - (RelOffset + 4);
  else
    assert("computeRelocation unknown for this relocation type");

  return 0;
}
