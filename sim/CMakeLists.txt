project(simtrax)
cmake_minimum_required(VERSION 2.8)

#
#  Default Build To Release, If Not Set By User
#
set(CMAKE_DEBUG_POSTFIX d)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE string
	"Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	if(UNIX OR APPLE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	endif(UNIX OR APPLE)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# enable C11
#list(APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")


# Update install location if it's the default
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "simtrax install location prefix" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


# find Boost
# this is a bit whacky, on Linux we'll dynamically link
# but for Windows - statically
if(CMAKE_HOST_WIN32)
	set(Boost_USE_STATIC_LIBS ON)
	set(Boost_USE_MULTITHREADED ON)
	set(Boost_USE_STATIC_RUNTIME OFF)
else()
	set(Boost_USE_STATIC_LIBS OFF)
	set(Boost_USE_MULTITHREADED ON)
	set(Boost_USE_STATIC_RUNTIME OFF)
endif()
#set(Boost_ADDITIONAL_VERSIONS "1.49" "1.49.0" "1.55" "1.55.0")
#set(Boost_DEBUG TRUE)
find_package(Boost COMPONENTS regex chrono thread system REQUIRED)

# Figure out the relative path between bin directory and samples directory
file(RELATIVE_PATH REL_PATH_BIN_TO_SAMPLES "${CMAKE_INSTALL_PREFIX}" "${CMAKE_SOURCE_DIR}/../")
add_definitions(-DREL_PATH_BIN_TO_SAMPLES="${REL_PATH_BIN_TO_SAMPLES}/")


set(simHdr
#	Animation.h
	Assembler.h
	Bitwise.h
	BranchUnit.h
	BVH.h
	Camera.h
	ConversionUnit.h
	configfile.h
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
	lodepng.h
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
#	Animation.cc
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
	lodepng.cc
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

include_directories(${Boost_INCLUDE_DIRS})
add_executable(simtrax ${simSrc} ${simHdr})
target_link_libraries(simtrax ${Boost_LIBRARIES})

install(TARGETS simtrax DESTINATION ${CMAKE_INSTALL_PREFIX})
#if(PTHREADS_LIBRARY_DLL)
#	install(FILES ${PTHREADS_LIBRARY_DLL} DESTINATION ${CMAKE_INSTALL_PREFIX})
#endif(PTHREADS_LIBRARY_DLL)

