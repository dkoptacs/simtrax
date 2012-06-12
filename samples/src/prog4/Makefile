# Example makefile for compiling trax code
# Note, rt.s and rt.bc are temporary files used by the makefile
# pre.ll and post.ll are human readable LLVM bitcode for debugging purposes

# Height and width for the executable
WIDTH=512
HEIGHT=512

# The source file with the main() function
SOURCES := $(wildcard *.cc)
SOURCE=prog4.cc
#SOURCE=example.cpp

# The names for your binary executable and trax assembly
BINARYOUTPUT=run_rt
TRAXOUTPUT=rt-llvm.s

# The directory for SimHWRT
SIMDIR=../../../sim

# The directory for llvm+trac
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

objs/%.o: ${SIMDIR}/%.cc mkdirs
	@echo "Building $<"
	@${CXX} ${CXXFLAGS} -o $@ -c $<

objs/trax_trax.bc: ${LIBDIR}/trax_trax.cpp mkdirs
	@echo "Building $<"
	@${TRAXC} -emit-llvm ${TRAXCFLAGS} -o $@ -c $<

objs/%.bc: %${CSUFFIX} mkdirs
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
	rm -rf objs/ rt.s rt.bc pre.ll post.ll ${TRAXOUTPUT} ${BINARYOUTPUT} out.png *~ *.dSYM a.out
