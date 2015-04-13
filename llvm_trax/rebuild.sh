#!/bin/bash

# This script rebuilds the llvm-trax compiler.

NUM_CORES=3

echo Copying TRaX-specific files to llvm-3.5.0.src...
cp Trax/IntrinsicsMips.td llvm-3.5.0.src/include/llvm/IR/IntrinsicsMips.td
cp Trax/MipsInstrFormats.td llvm-3.5.0.src/lib/Target/Mips/MipsInstrFormats.td
cp Trax/MipsSchedule.td llvm-3.5.0.src/lib/Target/Mips/MipsSchedule.td
cp Trax/MipsInstrInfo.td llvm-3.5.0.src/lib/Target/Mips/MipsInstrInfo.td
cp Trax/MipsSubtarget.cpp llvm-3.5.0.src/lib/Target/Mips/MipsSubtarget.cpp

cd build

echo Building...
make -j$NUM_CORES
