#!/bin/bash

# This script should be run to update the LLVM_Trax build.
# It copies all the needed files over and runs the compile.

NUM_CORES=2
BUILD_TYPE=Release
#BUILD_TYPE=Release+Asserts

echo Setting up llvm-trax workspace with build $BUILD_TYPE

PWD=`pwd`

# Check if LLVM is already downloaded and extracted
if [ -d $PWD/llvm-2.9 ]; then
    echo LLVM 2.9 found.
else
    echo Downloading LLVM...
    curl -OL http://llvm.org/releases/2.9/llvm-2.9.tgz
    echo Extracting LLVM...
    tar xzf llvm-2.9.tgz
    rm llvm-2.9.tgz
fi

echo Updating to latest version of source...
svn up
LLVMDIR=$PWD/llvm-2.9

# Copy files into llvm distribution
if [ -d $LLVMDIR/lib/Target/Trax ]; then
    echo Trax target already exists. Updating files...
    #####ADD FILE UPDATES#####
    for f in `ls Trax 2>/dev/null`
    do
	if [ -f Trax/$f ]; then
	    if diff Trax/$f $LLVMDIR/lib/Target/Trax/$f > /dev/null; then
		echo -n ''
	    else
		cp Trax/$f $LLVMDIR/lib/Target/Trax
	    fi
	fi
    done;
else
    echo Copying files into LLVM source...
    cp -R Trax $LLVMDIR/lib/Target/
fi

# Copy our versions of Triple.cpp and Triple.h
if [ -e bin/llc ]; then
    echo -n ''
else
    echo Updating configure script...
    cp misc/${BUILD_TYPE}_configure $LLVMDIR/configure

    cd $LLVMDIR
    echo Configuring...
    ./configure
    cd ../
fi
if diff misc/Triple.cpp $LLVMDIR/lib/Support/Triple.cpp > /dev/null; then
    echo -n ''
else
    cp misc/Triple.cpp $LLVMDIR/lib/Support/Triple.cpp
fi
if diff misc/Triple.h $LLVMDIR/include/llvm/ADT/Triple.h > /dev/null; then
    echo -n ''
else
    cp misc/Triple.h $LLVMDIR/include/llvm/ADT/Triple.h
fi
if diff misc/Intrinsics.td $LLVMDIR/include/llvm/Intrinsics.td > /dev/null; then
    echo -n ''
else
    cp misc/Intrinsics.td $LLVMDIR/include/llvm/Intrinsics.td
fi

if diff $LLVMDIR/lib/Target/Trax/IntrinsicsTrax.td $LLVMDIR/include/llvm/IntrinsicsTrax.td > /dev/null; then
    rm $LLVMDIR/lib/Target/Trax/IntrinsicsTrax.td
else
    mv $LLVMDIR/lib/Target/Trax/IntrinsicsTrax.td $LLVMDIR/include/llvm/IntrinsicsTrax.td
fi

# Configure and build
cd $LLVMDIR
echo Building...
make -j$NUM_CORES
cd ../

# Create symbolic link if it doesn't exist yet
if [ -L bin ]; then
    echo bin symbolic link already exists.
else
    echo Creating symbolic link to bin directory...
    ln -s $LLVMDIR/$BUILD_TYPE/bin bin
fi

echo To rebuild, delete $LLVMDIR and $PWD/bin then run this script again.
