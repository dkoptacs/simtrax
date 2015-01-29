# basic CMAKE file to compile the programs inside simtrax/

project(samples)
cmake_minimum_required(VERSION 2.8)

# set up environment depending on whether we're on windows or Linux
string(COMPARE EQUAL ${CMAKE_SYSTEM_NAME} "Windows" SYS_IS_WIN)
set(ADD_TO_EXE_NAME "")
set(TRAX_DIR "${CMAKE_SOURCE_DIR}/../llvm_trax/" CACHE PATH "Location of TRAX Directory")
set(LLVM_BINS "${TRAX_DIR}/bin/")
if(${SYS_IS_WIN})
	set(ADD_TO_EXE_NAME ".exe")
#	set(LLVM_BINS "${LLVM_BINS}/bin/")
endif(${SYS_IS_WIN})

set(LLVM_CLANG_LOC "clang++${ADD_TO_EXE_NAME}" CACHE FILEPATH "path to clang++${ADD_TO_EXE_NAME} used for compilation")
set(LLVM_BIN_DIR "${LLVM_BINS}/" CACHE PATH "LLVM binaries location")
set(LLVM_LINK_LOC "llvm-link${ADD_TO_EXE_NAME}" CACHE STRING "LLVM name of llvm-link${ADD_TO_EXE_NAME}")
set(LLVM_OPT_LOC "opt${ADD_TO_EXE_NAME}" CACHE STRING "LLVM name of opt${ADD_TO_EXE_NAME}")
set(LLVM_LLC_LOC "llc${ADD_TO_EXE_NAME}" CACHE STRING "LLVM name of llc${ADD_TO_EXE_NAME}")
set(TRAX_LN_PY_LOC "${TRAX_DIR}/linker/ln.py" CACHE FILEPATH "Trax linker python script 'ln.py'")
set(TRAX_INCLUDE_DIR "${TRAX_DIR}/include" CACHE PATH "Trax include directory that holds trax.hpp")

if(CMAKE_COMPILER_IS_GNUCC)
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
endif( CMAKE_COMPILER_IS_GNUCC)
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(UNIX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLINUX_64")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX_64")
endif(UNIX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")

#
#  Default Build To Release, If Not Set By User
#
set(CMAKE_DEBUG_POSTFIX d)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE string
	"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	if(UNIX OR APPLE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	endif(UNIX OR APPLE)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# Update install location if it's the default
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "samples install location prefix" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Get relative path from bin to scenes folder
file(RELATIVE_PATH BIN_TO_SCENES_PATH "${CMAKE_INSTALL_PREFIX}/bin/example/" "${CMAKE_SOURCE_DIR}/scenes/")

# Default scene and such... can be easily changed within visual studio
set(SIMDIR "${CMAKE_SOURCE_DIR}/../sim/" CACHE PATH "Location of the Simulator Sources")
set(LIBDIR "${TRAX_DIR}/lib")
set(SCENE_VIEWFILE \"${BIN_TO_SCENES_PATH}/cornell/cornell.view\" CACHE string "Default view file for scenes (must have double quotes)")
set(SCENE_MODELFILE \"${BIN_TO_SCENES_PATH}/cornell/CornellBox.obj\"  CACHE string "Default model file for scenes (must have double quotes)")
set(SCENE_LIGHTFILE \"${BIN_TO_SCENES_PATH}/cornell/cornell.light\"  CACHE string "Default light file for scenes (must have double quotes)")
set(SCENE_RAYDEPTH 1 CACHE string "Default maximum ray depth for path tracer, reflection and refraction")
set(SCENE_NUMSAMPLES 1 CACHE string "Default number of samples per pixel for super-sampling")
set(SCENE_CPPHEIGHT 512 CACHE string "Default height for CPU compilation")
set(SCENE_CPPWIDTH  512 CACHE string "Default width for CPU compilation")
set(SCENE_TRXHEIGHT 128 CACHE string "Default height for Trax Assembly compilation")
set(SCENE_TRXWIDTH  128 CACHE string "Default width for Trax Assembly compilation")
add_definitions(-DVIEWFILE=${SCENE_VIEWFILE} -DMODELFILE=${SCENE_MODELFILE} -DLIGHTFILE=${SCENE_LIGHTFILE})
include_directories(${SIMDIR} ${LIBDIR} ${TRAX_INCLUDE_DIR})


# Find pthreads
include(FindPthreads.cmake)
if(NOT PTHREADS_FOUND)
	message("- Build Error: pthread libraries not found!")
endif(NOT PTHREADS_FOUND)
include_directories(${PTHREADS_INCLUDE_DIR})

# Find the .dll we need to copy in case of windows
set(PTHREADS_LIBRARY_DLL "")
if(WIN32 AND PTHREADS_FOUND)
	# check 2 spots: same folder, with same name but with dll, rather than lib
	# or replacing the last 2 lib with dll
	string(FIND ${PTHREADS_LIBRARY} ".lib" LIBPATHINCLUDESLIB)
	if(${LIBPATHINCLUDESLIB} EQUAL -1)
		message(FATAL_ERROR "The pthreads library path (set to: ${PTHREADS_LIBRARY}) doesn't include .lib, please make sure that PTHREADS_LIBRARY path points to the pthreads libray file!")
	endif(${LIBPATHINCLUDESLIB} EQUAL -1)
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


#################################################################
set(TRAX_HEADERS
	${SIMDIR}/LoadMemory.h
	${SIMDIR}/OBJLoader.h
	${SIMDIR}/OBJListLoader.h
	${SIMDIR}/IWLoader.h
	${SIMDIR}/lodepng.h
	${SIMDIR}/BVH.h
	${SIMDIR}/Grid.h
	${SIMDIR}/Camera.h
	${SIMDIR}/ReadViewfile.h
	${SIMDIR}/Material.h
	${SIMDIR}/Triangle.h
	${SIMDIR}/MTLLoader.h
	${SIMDIR}/PPM.h
	${TRAX_INCLUDE_DIR}/trax.hpp
	${SIMDIR}/WinCommonNixFcns.h
	${SIMDIR}/TGALoader.h
	${SIMDIR}/ReadLightfile.h
	${SIMDIR}/CustomLoadMemory.h
)
set(TRAX_SOURCES
	${LIBDIR}/trax_cpp.cpp
	${SIMDIR}/lodepng.cc
	${SIMDIR}/LoadMemory.cc
	${SIMDIR}/OBJLoader.cc
	${SIMDIR}/OBJListLoader.cc
	${SIMDIR}/IWLoader.cc
	${SIMDIR}/BVH.cc
	${SIMDIR}/Grid.cc
	${SIMDIR}/Camera.cc
	${SIMDIR}/ReadViewfile.cc
	${SIMDIR}/Material.cc
	${SIMDIR}/Triangle.cc 
	${SIMDIR}/MTLLoader.cc
	${SIMDIR}/PPM.cc
	${SIMDIR}/TGALoader.cc
	${SIMDIR}/ReadLightfile.cc
	${SIMDIR}/CustomLoadMemory.cc
)

set(TRAX_TRAX_SOURCES ${LIBDIR}/trax_trax.cpp)


##################################################################
# Helper function that will create the sample projects for us, every time we want
# 
# targetName       - name of the target we want to give
# targetSources    - source files (.cc) that will be compiled with LLVM
# targetHeaders    - header files (.h) that will be used to create basic executable
# directoryPrePend - directory to prepend before all exe files... aka prepend='../' for examples/project# files
# addnlDef		   - additional definitions to append
function(applyToTarget targetName targetSources targetHeaders directoryPrePend addnlDef)

	# update paths for TRAX sources
	set(PRE_TRAX_SRC)
	foreach(src ${TRAX_SOURCES})
		list(APPEND PRE_TRAX_SRC ${directoryPrePend}${src})
	endforeach()

	# update paths for TRAX headers	
	set(PRE_TRAX_HDR)
	foreach(hdr ${TRAX_HEADERS})
		list(APPEND PRE_TRAX_HDR ${directoryPrePend}${hdr})
	endforeach()

	# get relative path between sample source location and actual example location
	# to be used as install location
	file(RELATIVE_PATH TARGET_INSTALL_SUBDIR ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})


	############
	# generate main executable
	add_executable(${targetName} ${${targetSources}} ${${targetHeaders}} ${PRE_TRAX_SRC} ${PRE_TRAX_HDR})
	target_link_libraries(${targetName} ${PTHREADS_LIBRARIES})
	set_target_properties(${targetName} PROPERTIES COMPILE_FLAGS "-DTRAX=0 -DWIDTH=${SCENE_CPPWIDTH} -DHEIGHT=${SCENE_CPPHEIGHT} ${PTHREADS_DEFINITIONS}")
	source_group(TRAX_Sources FILES ${PRE_TRAX_SRC})
	source_group(TRAX_Headers FILES ${PRE_TRAX_HDR})

	install(TARGETS ${targetName} DESTINATION "bin/${TARGET_INSTALL_SUBDIR}/")
	if(PTHREADS_LIBRARY_DLL)
		install(FILES ${PTHREADS_LIBRARY_DLL} DESTINATION "bin/${TARGET_INSTALL_SUBDIR}/")
	endif(PTHREADS_LIBRARY_DLL)


	############
	# generate TRAX target
	set(COMBINED_SOURCES)
	foreach(_file ${${targetSources}})
		list(APPEND COMBINED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${_file})
	endforeach(_file ${${targetSources}})
	foreach(_file ${TRAX_TRAX_SOURCES})
		list(APPEND COMBINED_SOURCES ${directoryPrePend}${_file})
	endforeach(_file ${TRAX_TRAX_SOURCES})

	if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/BC/")
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/BC/")
	endif(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/BC/")

	set(TARGET_BC_FILES)
	foreach(_file_cc ${COMBINED_SOURCES})
		get_filename_component(file_cc ${_file_cc} NAME_WE)
		set(file_bc "${CMAKE_CURRENT_BINARY_DIR}/BC/${file_cc}.bc")
		add_custom_command( OUTPUT ${file_bc}
							COMMAND ${directoryPrePend}${LLVM_BIN_DIR}/${LLVM_CLANG_LOC} -emit-llvm -DTRAX=1 -target mipsel -S -O3 ${addnlDef} -DWIDTH=${SCENE_TRXWIDTH} -DHEIGHT=${SCENE_TRXHEIGHT} -I${directoryPrePend}${TRAX_INCLUDE_DIR} -o ${file_bc} -c ${_file_cc}
							DEPENDS ${_file_cc})
		list(APPEND TARGET_BC_FILES ${file_bc})
	endforeach()

	# do we need the annoying .exe part?
	set(WIN_EXE_POSTPEND "")
	if(${SYS_IS_WIN})
		set(WIN_EXE_POSTPEND ".exe")
	endif(${SYS_IS_WIN})

	# do we need memory functions?
	set(MEMSET_FCNS_ADD "${directoryPrePend}${LIBDIR}/memset.s")
	set(MEMCPY_FCNS_ADD "${directoryPrePend}${LIBDIR}/memcpy.s")
	set(EXTENDS_FDF2_ADD "${directoryPrePend}${LIBDIR}/__extendsfdf2.s")

	# now, actually do the linking, etc
	set(targetPREll_file "${CMAKE_CURRENT_BINARY_DIR}/BC/${targetName}_pre")
	set(targetRTOPTbc_file "${CMAKE_CURRENT_BINARY_DIR}/BC/${targetName}_rtopt")
	set(targetRTs_file "${CMAKE_CURRENT_BINARY_DIR}/BC/${targetName}_rt")

	add_custom_command(OUTPUT "${targetPREll_file}.ll"
					   COMMAND ${directoryPrePend}${LLVM_BIN_DIR}/${LLVM_LINK_LOC} ${TARGET_BC_FILES} -o ${targetPREll_file}.ll
					   DEPENDS ${TARGET_BC_FILES})
	add_custom_command(OUTPUT "${targetRTOPTbc_file}.bc"
					   COMMAND ${directoryPrePend}${LLVM_BIN_DIR}/${LLVM_OPT_LOC} ${targetPREll_file}.ll -O3 -inline-threshold 200000000 -o ${targetRTOPTbc_file}.bc
					   DEPENDS "${targetPREll_file}.ll")
	add_custom_command(OUTPUT "${targetRTs_file}.s"
						COMMAND ${directoryPrePend}${LLVM_BIN_DIR}/${LLVM_LLC_LOC} ${targetRTOPTbc_file}.bc -o ${targetRTs_file}.s -march=mipsel -mattr=+mips32,+single-float -relocation-model=static
						DEPENDS "${targetRTOPTbc_file}.bc")
	list(APPEND TARGET_BC_FILES "${targetPREll_file}.ll" "${targetRTOPTbc_file}.bc" "${targetRTs_file}.s")

	# add the final rt-llvm.s target, which depends on things like memset.s
	set(targetRTLLVMs_file "${CMAKE_CURRENT_BINARY_DIR}/${targetName}_rt-llvm.s")
	add_custom_command(OUTPUT ${targetRTLLVMs_file}
					   COMMAND python "${directoryPrePend}${TRAX_LN_PY_LOC}" ${targetRTs_file}.s ${MEMSET_FCNS_ADD} ${MEMCPY_FCNS_ADD} ${EXTENDS_FDF2_ADD} > ${targetRTLLVMs_file}
					   DEPENDS "${targetRTs_file}.s" "${MEMSET_FCNS_ADD}" "${MEMCPY_FCNS_ADD}" "${EXTENDS_FDF2_ADD}" )	
	list(APPEND TARGET_BC_FILES "${MEMSET_FCNS_ADD}" "${MEMCPY_FCNS_ADD}" "${EXTENDS_FDF2_ADD}" "${targetRTLLVMs_file}")

	# final project target
	add_custom_target(TRAX_${targetName} ALL
					  DEPENDS ${TARGET_BC_FILES})

	install(FILES ${targetRTLLVMs_file} DESTINATION "bin/${TARGET_INSTALL_SUBDIR}/")

endfunction( applyToTarget )


###################################################################
# Actual projects
add_subdirectory(src)
