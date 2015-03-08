echo Copying TRaX-specific files to llvm-3.5.0.src...
cp Trax/IntrinsicsMips.td   llvm-3.5.0.src/include/llvm/IR/IntrinsicsMips.td
cp Trax/MipsInstrFormats.td llvm-3.5.0.src/lib/Target/Mips/MipsInstrFormats.td
cp Trax/MipsSchedule.td     llvm-3.5.0.src/lib/Target/Mips/MipsSchedule.td
cp Trax/MipsInstrInfo.td    llvm-3.5.0.src/lib/Target/Mips/MipsInstrInfo.td
