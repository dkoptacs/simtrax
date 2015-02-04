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

# Find pthreads
include(FindPthreads.cmake)
if(NOT PTHREADS_FOUND)
	message( "- Build Error: pthread libraries not found!")
endif(NOT PTHREADS_FOUND)
include_directories(${PTHREADS_INCLUDE_DIR})

# Find the .dll we need to copy in case of windows
set(PTHREADS_LIBRARY_DLL "")
if(WIN32 AND PTHREADS_FOUND)
	# check 2 spots: same folder, with same name but with dll, rather than lib
	# or replacing the last 2 lib with dll
	file(RELATIVE_PATH IncludeToLibPath ${PTHREADS_INCLUDE_DIR} ${PTHREADS_LIBRARY})
	string(REPLACE "lib" "dll" libFile ${IncludeToLibPath})
	file(TO_CMAKE_PATH  "${PTHREADS_INCLUDE_DIR}/${libFile}" libFile2)
	if(EXISTS ${libFile2})
		set(PTHREADS_LIBRARY_DLL ${libFile2})
		message(STATUS "Found pthreads dll: ${PTHREADS_LIBRARY_DLL}")
	ELSE(EXISTS ${libFile2})
		# try to find the file in the same folder...
		string(REPLACE ".lib" ".dll" libFile ${PTHREADS_LIBRARY})
		if(EXISTS ${libFile})
			set(PTHREADS_LIBRARY_DLL ${libFile})
			message(STATUS "Found pthreads dll: ${PTHREADS_LIBRARY_DLL}")
		else()
			message("Can't seem to find pthreads DLL, so it won't be copied into binary folders automatically on INSTALL")
			set(PTHREADS_LIBRARY_DLL PTHREADS_LIBRARY_DLL_NOT_FOUND)
		endif()
	ENDIF(EXISTS ${libFile2})
endif(WIN32 AND PTHREADS_FOUND)

# find Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
#set(Boost_ADDITIONAL_VERSIONS "1.49" "1.49.0" "1.55" "1.55.0")
#set(Boost_DEBUG TRUE)
find_package(Boost COMPONENTS regex chrono system REQUIRED)

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
link_directories(${Boost_LIBRARY_DIRS})
add_executable(simtrax ${simSrc} ${simHdr})
target_link_libraries(simtrax ${PTHREADS_LIBRARIES} ${Boost_LIBRARIES})

set(FINAL_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin/)
install(TARGETS simtrax DESTINATION ${FINAL_INSTALL_DIR})
if(CMAKE_HOST_WIN32)
	if(PTHREADS_LIBRARY_DLL)
		install(FILES ${PTHREADS_LIBRARY_DLL} DESTINATION ${FINAL_INSTALL_DIR})
	endif(PTHREADS_LIBRARY_DLL)

	if(Boost_INCLUDE_DIR)
		set(Boost_DLLS "")
		foreach(_file ${Boost_LIBRARIES})
			if(NOT ${_file} MATCHES "debug" AND NOT ${_file} MATCHES "optimized")
				get_filename_component(file_dir ${_file} PATH)
				get_filename_component(file_cc ${_file} NAME_WE)
				set(file_dll "${file_dir}/${file_cc}.dll")
				list(APPEND Boost_DLLS ${file_dll})
			endif()
		endforeach()
		install(FILES ${Boost_DLLS} DESTINATION ${FINAL_INSTALL_DIR})
	endif()
endif(CMAKE_HOST_WIN32)