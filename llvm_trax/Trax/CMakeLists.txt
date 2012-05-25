set(LLVM_TARGET_DEFINITIONS Trax.td)

tablegen(TraxGenRegisterInfo.h.inc -gen-register-desc-header)
tablegen(TraxGenRegisterNames.inc -gen-register-enums)
tablegen(TraxGenRegisterInfo.inc -gen-register-desc)
tablegen(TraxGenInstrNames.inc -gen-instr-enums)
tablegen(TraxGenInstrInfo.inc -gen-instr-desc)
tablegen(TraxGenCodeEmitter.inc -gen-emitter)
tablegen(TraxGenAsmWriter.inc -gen-asm-writer)
tablegen(TraxGenAsmMatcher.inc -gen-asm-matcher)
tablegen(TraxGenDAGISel.inc -gen-dag-isel)
tablegen(TraxGenCallingConv.inc -gen-callingconv)
tablegen(TraxGenSubtarget.inc -gen-subtarget)
tablegen(TraxGenIntrinsics.inc -gen-tgt-intrinsic)
tablegen(TraxGenEDInfo.inc -gen-enhanced-disassembly-info)

add_llvm_target(TraxCodeGen
  TraxDelaySlotFiller.cpp
  TraxInstrInfo.cpp
  TraxISelDAGToDAG.cpp
  TraxISelLowering.cpp
  TraxFrameLowering.cpp
  TraxMCAsmInfo.cpp
  TraxRegisterInfo.cpp
  TraxSubtarget.cpp
  TraxTargetMachine.cpp
  TraxTargetObjectFile.cpp
  TraxIntrinsicInfo.cpp
  TraxSelectionDAGInfo.cpp
  TraxAsmPrinter.cpp
#  TraxAsmBackend.cpp
  TraxMCInstLower.cpp
  TraxELFWriterInfo.cpp
  TraxMCCodeEmitter.cpp
  )

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
