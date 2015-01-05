#!/bin/bash

# This script installs the llvm-trax compiler.

echo Setting up llvm-trax workspace

NUM_CORES=4

PWD=`pwd`
#http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-macosx-apple-darwin.tar.xz


which wget &> /dev/null
if [ $? == 0 ]; then
    DOWNLOADER="wget"
    DOWNLOADEROPTS = "-O"
fi
#echo $DOWNLOADER
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

uname | grep 'Darwin' &> /dev/null
if [ $? == 0 ]; then
   CLANGURL="http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-macosx-apple-darwin.tar.xz"
   CLANGDIR="clang+llvm-3.5.0-macosx-apple-darwin"
fi
uname | grep 'FreeBSD' &> /dev/null
if [ $? == 0 ]; then
   CLANGURL="http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-amd64-unknown-freebsd10.tar.xz"
   CLANGDIR="clang+llvm-3.5.0-amd64-unknown-freebsd10"
fi
uname | grep 'Linux' &> /dev/null
if [ $? == 0 ]; then
    cat /proc/version | grep 'Red'
    if [ $? == 0 ]; then
	CLANGURL="http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-i686-fedora20.tar.xz"
	CLANGDIR="clang+llvm-3.5.0-i686-fedora20"
    fi
    cat /proc/version | grep 'Ubuntu'
    if [ $? == 0 ]; then
	CLANGURL="http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
	CLANGDIR="clang+llvm-3.5.0-x86_64-linux-gnu-ubuntu-14.04"
    fi
    cat /proc/version | grep 'SUSE'
    if [ $? == 0 ]; then
	CLANGURL="http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-x86_64-opensuse13.1.tar.xz"
	CLANGDIR="clang+llvm-3.5.0-x86_64-opensuse13.1"
    fi
fi

echo Downloading clang...
RUN="$DOWNLOADER $DOWNLOADEROPTS clang.tar.xz $CLANGURL"
echo $RUN
$RUN

echo Extracting clang...
tar zxf clang.tar.xz
RUN="mv $CLANGDIR clang-3.5.0"
$RUN

echo Downloading llvm...
RUN="$DOWNLOADER $DOWNLOADEROPTS llvm-3.5.0.src.tar.xz http://llvm.org/releases/3.5.0/llvm-3.5.0.src.tar.xz"
echo $RUN
$RUN

echo Extracting llvm...
tar zxf llvm-3.5.0.src.tar.xz

echo Copying TRaX-specific files to llvm-3.5.0.src...
cp Trax/IntrinsicsMips.td llvm-3.5.0.src/include/llvm/IR/IntrinsicsMips.td
cp Trax/MipsInstrFormats.td llvm-3.5.0.src/lib/Target/Mips/MipsInstrFormats.td
cp Trax/MipsSchedule.td llvm-3.5.0.src/lib/Target/Mips/MipsSchedule.td
cp Trax/MipsInstrInfo.td llvm-3.5.0.src/lib/Target/Mips/MipsInstrInfo.td

CLANGBIN="$PWD/clang-3.5.0/bin/clang++"

cd llvm-3.5.0.src
echo Configuring...
CXX=$CLANGBIN ./configure

echo Building...
CXX=$CLANGBIN make -j$NUM_CORES




