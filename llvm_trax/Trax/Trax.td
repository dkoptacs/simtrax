//===- Trax.td - Describe the Trax Target Machine ------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This is the top level entry point for the Trax target.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Register File, Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "TraxRegisterInfo.td"
include "TraxSchedule.td"
include "TraxIntrinsics.td"
include "TraxInstrInfo.td"
include "TraxCallingConv.td"

def TraxInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Microblaze Subtarget features                                              //
//===----------------------------------------------------------------------===//

def FeaturePipe3       : SubtargetFeature<"pipe3", "HasPipe3", "true",
                                "Implements 3-stage pipeline">;
def FeatureBarrel      : SubtargetFeature<"barrel", "HasBarrel", "true",
                                "Implements barrel shifter">;
def FeatureDiv         : SubtargetFeature<"div", "HasDiv", "true",
                                "Implements hardware divider">;
def FeatureMul         : SubtargetFeature<"mul", "HasMul", "true",
                                "Implements hardware multiplier">;
def FeatureFSL         : SubtargetFeature<"fsl", "HasFSL", "true",
                                "Implements FSL instructions">;
def FeatureEFSL        : SubtargetFeature<"efsl", "HasEFSL", "true",
                                "Implements extended FSL instructions">;
def FeatureMSRSet      : SubtargetFeature<"msrset", "HasMSRSet", "true",
                                "Implements MSR register set and clear">;
def FeatureException   : SubtargetFeature<"exception", "HasException", "true",
                                "Implements hardware exception support">;
def FeaturePatCmp      : SubtargetFeature<"patcmp", "HasPatCmp", "true",
                                "Implements pattern compare instruction">;
def FeatureFPU         : SubtargetFeature<"fpu", "HasFPU", "true",
                                "Implements floating point unit">;
def FeatureESR         : SubtargetFeature<"esr", "HasESR", "true",
                                "Implements ESR and EAR registers">;
def FeaturePVR         : SubtargetFeature<"pvr", "HasPVR", "true",
                                "Implements processor version register">;
def FeatureMul64       : SubtargetFeature<"mul64", "HasMul64", "true",
                                "Implements multiplier with 64-bit result">;
def FeatureSqrt        : SubtargetFeature<"sqrt", "HasSqrt", "true",
                                "Implements sqrt and floating point convert">;
def FeatureMMU         : SubtargetFeature<"mmu", "HasMMU", "true",
                                "Implements memory management unit">;

//===----------------------------------------------------------------------===//
// Trax processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, TraxGenericItineraries, Features>;

def : Proc<"v400", []>;
def : Proc<"v500", []>;
def : Proc<"v600", []>;
def : Proc<"v700", []>;
def : Proc<"v710", []>;

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

def TraxAsmWriter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Target Declaration
//===----------------------------------------------------------------------===//

def Trax : Target {
  let InstructionSet = TraxInstrInfo;
  let AssemblyWriters = [TraxAsmWriter];
}
