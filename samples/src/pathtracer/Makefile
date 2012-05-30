# Example makefile for compiling trax code
# Note, rt.s and rt.bc are temporary files used by the makefile
# pre.ll and post.ll are human readable LLVM bitcode for debugging purposes

# Height and width for the executable
WIDTH=512
HEIGHT=512

# The suffix for your source files
CSUFFIX=.cc
SOURCES := $(wildcard *${CSUFFIX})

# The names for your binary executable and trax assembly
BINARYOUTPUT=run_rt
TRAXOUTPUT=rt-llvm.s

# The directory for SimHWRT
SIMDIR=../../../sim

# The directory for LLVM_Trax
LLVM_TRAXDIR=../../../llvm_trax

# Directories that should be in LLVM_Trax
BINDIR=${LLVM_TRAXDIR}/bin
LIBDIR=${LLVM_TRAXDIR}/lib
INCLUDEDIR=${LLVM_TRAXDIR}/include
LINKERDIR=${LLVM_TRAXDIR}/linker

# Scene setup
SCENEDIR=../../scenes
# Cornell Scene
VIEWFILE=\"${SCENEDIR}/cornell/cornell.view\"
MODELFILE=\"${SCENEDIR}/cornell/CornellBox.obj\"
LIGHTFILE=\"${SCENEDIR}/cornell/cornell.light\"
# Sponza Scene
# VIEWFILE=\"${SIMDIR}/views/sponza_obj.view\"
# MODELFILE=\"${SIMDIR}/test_models/sponza.obj\"
# LIGHTFILE=\"${SIMDIR}/lights/sponza.light\"
# Crytek Sponza Scene
#VIEWFILE=\"${SIMDIR}/views/sponza_obj.view\"
#MODELFILE=\"${SIMDIR}/test_models/crytek-sponza/sponza.obj\"
#LIGHTFILE=\"${SIMDIR}/lights/sponza.light\"
# Cube Scene
#VIEWFILE=\"${SIMDIR}/views/cube.view\"
#MODELFILE=\"${SIMDIR}/test_models/cube/cube.obj\"
#LIGHTFILE=\"${SIMDIR}/lights/cube.light\"
# Conference Scene
# VIEWFILE=\"${SIMDIR}/views/conference.view\"
# MODELFILE=\"${SIMDIR}/test_models/conference.obj\"
# LIGHTFILE=\"${SIMDIR}/lights/conference.light\"
# Luxmark Room Scene
# VIEWFILE=\"${SIMDIR}/views/luxroom_wide.view\"
# MODELFILE=\"${SIMDIR}/test_models/luxroom/luxroom.objl\"
# LIGHTFILE=\"${SIMDIR}/lights/cube.light\"
# Luxmark Sala Scene
#VIEWFILE=\"${SIMDIR}/views/luxsala.view\"
#MODELFILE=\"${SIMDIR}/test_models/luxsala/luxsala.objl\"
#LIGHTFILE=\"${SIMDIR}/lights/cube.light\"

# If you want to load memory dumps instead of rebuilding the BVH
# use trax_setup(true)
MEMORYFILE=\"memory.mem\"

# Should not need to edit below this point unless you know what you're doing
# The compiler to use for the executable
CXX=g++
CXXFILENAMES=-DVIEWFILE=${VIEWFILE} -DMODELFILE=${MODELFILE} -DLIGHTFILE=${LIGHTFILE} -DMEMORYFILE=${MEMORYFILE}
CXXFLAGS=-DTRAX=0 -DWIDTH=${WIDTH} -DHEIGHT=${HEIGHT} ${CXXFILENAMES} -I${SIMDIR} -I${INCLUDEDIR} -pthread -O3 -g
CXXSIMFILES=${SIMDIR}/LoadMemory.cc ${SIMDIR}/OBJLoader.cc ${SIMDIR}/OBJListLoader.cc ${SIMDIR}/IWLoader.cc ${SIMDIR}/BVH.cc ${SIMDIR}/Grid.cc ${SIMDIR}/Camera.cc ${SIMDIR}/ReadLightfile.cc ${SIMDIR}/ReadViewfile.cc ${SIMDIR}/Material.cc ${SIMDIR}/Triangle.cc ${SIMDIR}/MTLLoader.cc ${SIMDIR}/PPM.cc ${SIMDIR}/TGALoader.cc ${SIMDIR}/CustomLoadMemory.cc
CXXLIBS=${LIBDIR}/trax_cpp.cpp

# Front end compiler to use for trax target
# For the default mac build
TRAXC=llvm-gcc
# For compiling on the CADE machines
# TRAXC=/usr/local/stow/llvm-gcc/llvm-gcc4.2-2.6/bin/g++
# TRAXC=/scratch/Research/hwrt/llvm-gcc4.2-2.9-x86_64-linux/bin/llvm-gcc
# If you prefer clang it might work.
# TRAXC=clang
TRAXCFLAGS=-DTRAX=1 -I${INCLUDEDIR}

all: mkdirs ${BINARYOUTPUT} ${TRAXOUTPUT}
default: mkdirs ${BINARYOUTPUT} ${TRAXOUTPUT}

COBJS := $(addprefix objs/, $(notdir $(CXXSIMFILES:.cc=.o)))

${BINARYOUTPUT}: ${SOURCES} ${COBJS}
	@echo "Building ${BINARYOUTPUT}"
	@${CXX} ${SOURCES} ${COBJS} ${CXXLIBS} ${CXXFLAGS} -o ${BINARYOUTPUT}

${TRAXOUTPUT}: rt.s
	@echo "Writing ${TRAXOUTPUT}"
	@${LINKERDIR}/ln.py rt.s > ${TRAXOUTPUT}

mkdirs:
	@mkdir -p objs;

objs/%.o: ${SIMDIR}/%.cc
	@echo "Building $<"
	@${CXX} ${CXXFLAGS} -o $@ -c $<

objs/trax_trax.bc: ${LIBDIR}/trax_trax.cpp
	@echo "Building $<"
	@${TRAXC} -emit-llvm ${TRAXCFLAGS} -o $@ -c $<

objs/%.bc: %${CSUFFIX}
	@echo "Building $<"
	@${TRAXC} -emit-llvm ${TRAXCFLAGS} -o $@ -c $<

OBJS := $(addprefix objs/, $(notdir $(SOURCES:${CSUFFIX}=.bc))) objs/trax_trax.bc

rt.s: ${OBJS} ${INCLUDEDIR}/trax.hpp
	@${BINDIR}/llvm-ld ${OBJS} -b rt.bc
	@${BINDIR}/llvm-dis rt.bc -o pre.ll
	@${BINDIR}/opt rt.bc -O3 -inline-threshold 200000000 -o rt.bc
	@${BINDIR}/llvm-dis rt.bc -o post.ll
	@${BINDIR}/llc rt.bc -o rt.s -march=trax

clean:
	rm -rf objs/ rt.s rt.bc pre.ll post.ll ${TRAXOUTPUT} ${BINARYOUTPUT} out.png *~ 
