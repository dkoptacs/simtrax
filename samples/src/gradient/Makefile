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
BINDIR=${LLVM_TRAXDIR}/build/Release+Asserts/bin
LIBDIR=${LLVM_TRAXDIR}/lib
INCLUDEDIR=${LLVM_TRAXDIR}/include
LINKERDIR=${LLVM_TRAXDIR}/linker

# Scene setup
SCENEDIR=../../scenes
# Cornell Scene
VIEWFILE=\"${SCENEDIR}/cornell/cornell.view\"
MODELFILE=\"${SCENEDIR}/cornell/CornellBox.obj\"
LIGHTFILE=\"${SCENEDIR}/cornell/cornell.light\"

# If you want to load memory dumps instead of rebuilding the BVH
# use trax_setup(true)
MEMORYFILE=\"memory.mem\"

# The compiler to use for the executable
CXX=g++
CXXFILENAMES=-DVIEWFILE=${VIEWFILE} -DMODELFILE=${MODELFILE} -DLIGHTFILE=${LIGHTFILE} -DMEMORYFILE=${MEMORYFILE}
CXXFLAGS=-DTRAX=0 -DWIDTH=${WIDTH} -DHEIGHT=${HEIGHT} ${CXXFILENAMES} -I${SIMDIR} -I${INCLUDEDIR} -pthread -O3 -g
CXXSIMFILES=${SIMDIR}/LoadMemory.cc ${SIMDIR}/OBJLoader.cc ${SIMDIR}/OBJListLoader.cc ${SIMDIR}/IWLoader.cc ${SIMDIR}/BVH.cc ${SIMDIR}/Grid.cc ${SIMDIR}/Camera.cc ${SIMDIR}/ReadLightfile.cc ${SIMDIR}/ReadViewfile.cc ${SIMDIR}/Material.cc ${SIMDIR}/Triangle.cc ${SIMDIR}/MTLLoader.cc ${SIMDIR}/PPM.cc ${SIMDIR}/TGALoader.cc ${SIMDIR}/CustomLoadMemory.cc ${SIMDIR}/lodepng.cc
CXXLIBS=${LIBDIR}/trax_cpp.cpp

# for OSX
LDFLAGS = -I/opt/local/include/ -L/opt/local/lib/ -lboost_regex-mt -lboost_system-mt -lboost_chrono-mt

# for CADE
#LDFLAGS = -I/usr/local/stow/boost/boost_1_55_0/include/ -L/usr/local/stow/boost/boost_1_55_0/lib/ -lboost_regex -lboost_system -lboost_chrono

# for ubuntu/other linux
#LDFLAGS = -lboost_regex -lboost_system -lboost_chrono


# Front end compiler to use for trax target
# For old systems
#TRAXC=llvm-gcc

# clang is preferred.
TRAXC=${BINDIR}/clang++
#TRAXC=clang++
TRAXCFLAGS=-DTRAX=1 -I${INCLUDEDIR} -target mipsel -S -O3

all: mkdirs ${BINARYOUTPUT} ${TRAXOUTPUT}
default: mkdirs ${BINARYOUTPUT} ${TRAXOUTPUT}
#all: mkdirs ${TRAXOUTPUT}
#default: mkdirs ${TRAXOUTPUT}

COBJS := $(addprefix objs/, $(notdir $(CXXSIMFILES:.cc=.o)))

${BINARYOUTPUT}: ${SOURCES} ${COBJS}
	@echo "Building ${BINARYOUTPUT}"
	@${CXX} ${SOURCES} ${COBJS} ${CXXLIBS} ${CXXFLAGS} ${LDFLAGS} -o ${BINARYOUTPUT}

${TRAXOUTPUT}: rt.s
	@echo "Writing ${TRAXOUTPUT}"
	@${LINKERDIR}/ln.py rt.s ${LIBDIR}/memset.s ${LIBDIR}/memcpy.s ${LIBDIR}/__extendsfdf2.s > ${TRAXOUTPUT}

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
#OBJS := $(addprefix objs/, $(notdir $(SOURCES:${CSUFFIX}=.bc)))

rt.s: ${OBJS} ${INCLUDEDIR}/trax.hpp
	@${BINDIR}/llvm-link ${OBJS} -o rt.bc
	@${BINDIR}/opt rt.bc -O3 -inline-threshold 200000000 -o rt.bc
	@${BINDIR}/llc rt.bc -o rt.s -march=mipsel -mattr=+mips32,+single-float -relocation-model=static


clean:
	rm -rf objs/ rt.s rt.bc pre.ll post.ll ${TRAXOUTPUT} ${BINARYOUTPUT} out.png *~ *.dSYM a.out
