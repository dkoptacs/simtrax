#!/bin/bash

# This script installs the llvm-trax compiler.

echo Setting up llvm-trax workspace

NUM_CORES=4

PWD=`pwd`

tar zxvf llvm-3.1.tar.gz

LLVMDIR=$PWD/llvm-3.1

# Configure and build
cd $LLVMDIR
echo Configuring...
./configure
echo Building...
make -j$NUM_CORES
cd ../

# Create symbolic link if it doesn't exist yet
if [ -L bin ]; then
    echo bin symbolic link already exists.
else
    echo Creating symbolic link to bin directory...
    ln -s $LLVMDIR/Release+Asserts/bin bin
fi

echo To rebuild, enter $LLVMDIR and run \"make clean\", then delete $PWD/bin then run this script again.
