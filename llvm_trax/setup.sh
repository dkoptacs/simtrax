#!/bin/bash

# This script installs the llvm-trax compiler.

echo Setting up llvm-trax workspace

NUM_CORES=4

PWD=`pwd`
#http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-macosx-apple-darwin.tar.xz


which wget &> /dev/null
if [ $? == 0 ]; then
    DOWNLOADER="wget"
    DOWNLOADEROPTS="-O"
fi

if [ ! -n "$DOWNLOADER" ]; then
    which curl &> /dev/null
    if [ $? == 0 ]; then
	DOWNLOADER="curl"
	DOWNLOADEROPTS="-o"
    else
	echo setup needs curl or wget
	exit 1
    fi
fi

echo Downloading llvm...
RUN="$DOWNLOADER $DOWNLOADEROPTS llvm-3.5.0.src.tar.xz http://llvm.org/releases/3.5.0/llvm-3.5.0.src.tar.xz"
echo $RUN
$RUN

echo Extracting llvm...
tar xf llvm-3.5.0.src.tar.xz

echo Downloading clang...
RUN="$DOWNLOADER $DOWNLOADEROPTS clang.tar.xz http://llvm.org/releases/3.5.0/cfe-3.5.0.src.tar.xz"
echo $RUN
$RUN

echo Extracting clang...
tar xf clang.tar.xz
mv cfe-3.5.0.src llvm-3.5.0.src/tools/clang

echo Copying TRaX-specific files to llvm-3.5.0.src...
cp Trax/IntrinsicsMips.td llvm-3.5.0.src/include/llvm/IR/IntrinsicsMips.td
cp Trax/MipsInstrFormats.td llvm-3.5.0.src/lib/Target/Mips/MipsInstrFormats.td
cp Trax/MipsSchedule.td llvm-3.5.0.src/lib/Target/Mips/MipsSchedule.td
cp Trax/MipsInstrInfo.td llvm-3.5.0.src/lib/Target/Mips/MipsInstrInfo.td
cp Trax/MipsSubtarget.cpp llvm-3.5.0.src/lib/Target/Mips/MipsSubtarget.cpp

mkdir build
cd build
echo Configuring...
CXX=g++ ../llvm-3.5.0.src/configure --enable-optimized

echo Building...
make -j$NUM_CORES




