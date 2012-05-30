#!/bin/bash

SIMTRAXROOT=../../
#SIMTRAXROOT=/absolute/path/to/simtrax

ASSEMBLY=$SIMTRAXROOT/samples/src/pathtracer/rt-llvm.s
# simulation report file
OUTPUT_FILE=temp_results.txt
# image output name
OUTPUT_PREFIX=out
# hardware configuration file
CONFIG=$SIMTRAXROOT/samples/configs/default.config

# WIDTH=32
# HEIGHT=32
WIDTH=64
HEIGHT=64
# WIDTH=128
# HEIGHT=128
# WIDTH=512
# HEIGHT=512
SIMULATION_THREADS=2
NUM_PROCS=32
THREADS_PER_PROC=1
SIMD_WIDTH=1
NUM_TMS=4
NUM_ICACHES=4
NUM_ICACHE_BANKS=16
NUM_L2S=1
NUM_REGS=36
# 32 compiler used + 4 reserved
#OTHER is used just to add extra arguments
OTHER=

SIMTRAX=$SIMTRAXROOT/sim/simtrax

# SCENE=--no-scene
#cornell
SCENEDIR=$SIMTRAXROOT/samples/scenes/cornell
SCENE="--view-file $SCENEDIR/cornell.view --model $SCENEDIR/CornellBox.obj --light-file $SCENEDIR/cornell.light"

ICACHE_CONF="--num-icaches $NUM_ICACHES --num-icache-banks $NUM_ICACHE_BANKS --num-l2s $NUM_L2S"
CORE_CONF="--num-regs $NUM_REGS --num-thread-procs $NUM_PROCS --threads-per-proc $THREADS_PER_PROC --num-cores $NUM_TMS --simd-width $SIMD_WIDTH"
IMAGE_CONF="--width $WIDTH --height $HEIGHT --output-prefix $OUTPUT_PREFIX"
MISC_CONF="--simulation-threads $SIMULATION_THREADS --config-file $CONFIG"

RUN="time nice $SIMTRAX $OTHER $CORE_CONF $IMAGE_CONF $MISC_CONF $ICACHE_CONF $SCENE --load-assembly $ASSEMBLY"
echo $RUN
$RUN > $OUTPUT_FILE


