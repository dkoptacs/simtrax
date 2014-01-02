project(simtrax)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})

# Find pthreads
include(FindPthreads.cmake)
if(NOT PTHREADS_FOUND)
	message( "- Build Error: pthread libraries not found!")
endif(NOT PTHREADS_FOUND)
include_directories(${PTHREADS_INCLUDE_DIR})

set(simHdr
	Animation.h
	Assembler.h
	Bitwise.h
	BranchUnit.h
	BVH.h
	Camera.h
	ConversionUnit.h
	CustomLoadMemory.h
	DebugUnit.h
	DiskBuffer.h
	FourByte.h
	FPAddSub.h
	FPCompare.h
	FPDiv.h
	FPInvSqrt.h
	FPMinMax.h
	FPMul.h
	FunctionalUnit.h
	GlobalRegisterFile.h
	Grid.h
	Hammersley.h
	HardwareModule.h
	Instruction.h
	IntAddSub.h
	IntMul.h
	IssueUnit.h
	IWLoader.h
	L1Cache.h
	L2Cache.h
	LoadMemory.h
	LoadStore.h
	LocalStore.h
	MainMemory.h
	Material.h
	memory_controller.h
	MemoryBase.h
	MTLLoader.h
	OBJListLoader.h
	OBJLoader.h
	params.h
	PPM.h
	Primitive.h
	processor.h
	ReadConfig.h
	ReadLightfile.h
	ReadViewfile.h
	scheduler.h
	SimpleRegisterFile.h
	Synchronize.h
	TGALoader.h
	ThreadProcessor.h
	ThreadState.h
	TraxCore.h
	Triangle.h
	usimm.h
	utils.h
	utlist.h
	Vector3.h
	WinCommonNixFcns.h
	WriteRequest.h
)

set(simSrc
	Animation.cc
	Assembler.cc
	Bitwise.cc
	BranchUnit.cc
	BVH.cc
	Camera.cc
	ConversionUnit.cc
	CustomLoadMemory.cc
	DebugUnit.cc
	FPAddSub.cc
	FPCompare.cc
	FPDiv.cc
	FPInvSqrt.cc
	FPMinMax.cc
	FPMul.cc
	GlobalRegisterFile.cc
	Grid.cc
	Instruction.cc
	IntAddSub.cc
	IntMul.cc
	IssueUnit.cc
	IWLoader.cc
	L1Cache.cc
	L2Cache.cc
	LoadMemory.cc
	LoadStore.cc
	LocalStore.cc
	MainMemory.cc
	Material.cc
	memory_controller.cc
	MemoryBase.cc
	MTLLoader.cc
	OBJListLoader.cc
	OBJLoader.cc
	PPM.cc
	ReadConfig.cc
	ReadLightfile.cc
	ReadViewfile.cc
	scheduler.cc
	SimpleRegisterFile.cc
	Synchronize.cc
	TGALoader.cc
	ThreadProcessor.cc
	ThreadState.cc
	TraxCore.cc
	Triangle.cc
	usimm.cc
	WriteRequest.cc
	main.cc
)

add_executable(simtrax ${simSrc} ${simHdr})
target_link_libraries(simtrax ${PTHREADS_LIBRARIES})

install(TARGETS simtrax DESTINATION bin)
