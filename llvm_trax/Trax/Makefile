##===- lib/Target/Trax/Makefile --------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##
LEVEL = ../../..
LIBRARYNAME = LLVMTraxCodeGen
TARGET = Trax

# Make sure that tblgen is run, first thing.
BUILT_SOURCES = TraxGenRegisterInfo.h.inc TraxGenRegisterNames.inc \
								TraxGenRegisterInfo.inc TraxGenInstrNames.inc \
								TraxGenInstrInfo.inc TraxGenAsmWriter.inc \
								TraxGenDAGISel.inc TraxGenAsmMatcher.inc \
								TraxGenCodeEmitter.inc TraxGenCallingConv.inc \
								TraxGenSubtarget.inc TraxGenIntrinsics.inc \
								TraxGenEDInfo.inc

DIRS = InstPrinter AsmParser Disassembler TargetInfo

include $(LEVEL)/Makefile.common

