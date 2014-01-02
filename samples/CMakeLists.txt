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
	set(LLVM_BINS "${LLVM_BINS}/bin/")
endif(${SYS_IS_WIN})

set(LLVM_BIN_DIR "${LLVM_BINS}/" CACHE PATH "LLVM binaries location")
set(LLVM_GCC_LOC "llvm-gcc${ADD_TO_EXE_NAME}" CACHE STRING "LLVM name of llvm-gcc${ADD_TO_EXE_NAME}")	# --- modify to use clang instead!!!
set(LLVM_LD_LOC "llvm-ld${ADD_TO_EXE_NAME}" CACHE STRING "LLVM name of llvm-ld${ADD_TO_EXE_NAME}")
set(LLVM_DIS_LOC "llvm-dis${ADD_TO_EXE_NAME}" CACHE STRING "LLVM name of llvm-dis${ADD_TO_EXE_NAME}")
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


# Default scene and such... can be easily changed within visual studio
set(SIMDIR "${CMAKE_SOURCE_DIR}/../sim/" CACHE PATH "Location of the Simulator")
set(LIBDIR "${TRAX_DIR}/lib")
set(SCENE_VIEWFILE \"${CMAKE_SOURCE_DIR}/scenes/cornell/cornell.view\" CACHE string "Default view file for scenes (must have double quotes)")
set(SCENE_MODELFILE \"${CMAKE_SOURCE_DIR}/scenes/cornell/CornellBox.obj\"  CACHE string "Default model file for scenes (must have double quotes)")
set(SCENE_LIGHTFILE \"${CMAKE_SOURCE_DIR}/scenes/cornell/cornell.light\"  CACHE string "Default light file for scenes (must have double quotes)")
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


#################################################################
set(TRAX_HEADERS ${SIMDIR}/LoadMemory.h 		${SIMDIR}/OBJLoader.h	${SIMDIR}/OBJListLoader.h
				 ${SIMDIR}/IWLoader.h 
				 ${SIMDIR}/BVH.h 				${SIMDIR}/Grid.h		${SIMDIR}/Camera.h
				 ${SIMDIR}/ReadViewfile.h		${SIMDIR}/Material.h	${SIMDIR}/Triangle.h 
				 ${SIMDIR}/MTLLoader.h 	   		${SIMDIR}/PPM.h			${TRAX_INCLUDE_DIR}/trax.hpp
				 ${SIMDIR}/WinCommonNixFcns.h 	${SIMDIR}/TGALoader.h	${SIMDIR}/ReadLightfile.h
				 ${SIMDIR}/CustomLoadMemory.h
)
set(TRAX_SOURCES ${LIBDIR}/trax_cpp.cpp 
				 ${SIMDIR}/LoadMemory.cc 	${SIMDIR}/OBJLoader.cc 		${SIMDIR}/OBJListLoader.cc
				 ${SIMDIR}/IWLoader.cc 
				 ${SIMDIR}/BVH.cc			${SIMDIR}/Grid.cc	   		${SIMDIR}/Camera.cc
				 ${SIMDIR}/ReadViewfile.cc	${SIMDIR}/Material.cc  		${SIMDIR}/Triangle.cc 
				 ${SIMDIR}/MTLLoader.cc		${SIMDIR}/PPM.cc	   		${SIMDIR}/TGALoader.cc
				 ${SIMDIR}/ReadLightfile.cc ${SIMDIR}/CustomLoadMemory.cc
)

set(TRAX_TRAX_SOURCES ${LIBDIR}/trax_trax.cpp)


##################################################################
# Helper function that will create the sample projects for us, every time we want
# 
# targetName       - name of the target we want to give
# targetSources    - source files (.cc) that will be compiled with LLVM
# targetHeaders    - header files (.h) that will be used to create basic executable
# directoryPrePend - directory to prepend before all exe files... aka prepend='../' for examples/project# files
# needMemset       - flag whether to include ${LIBDIR}/memset.s into our final assembly files
# addnlDef		   - additional definitions to append
function(applyToTarget targetName targetSources targetHeaders directoryPrePend needMemset addnlDef)

	# update paths for TRAX sources
	set(PRE_TRAX_SRC)
	foreach(src ${TRAX_SOURCES})
		list(APPEND PRE_TRAX_SRC ${directoryPrePend}${src})
	endforeach()

	# update pahts for TRAX headers	
	set(PRE_TRAX_HDR)
	foreach(hdr ${TRAX_HEADERS})
		list(APPEND PRE_TRAX_HDR ${directoryPrePend}${hdr})
	endforeach()


	############
	# generate main executable
	add_executable(${targetName} ${${targetSources}} ${${targetHeaders}} ${PRE_TRAX_SRC} ${PRE_TRAX_HDR})
	target_link_libraries(${targetName} ${PTHREADS_LIBRARIES})
	set_target_properties(${targetName} PROPERTIES COMPILE_FLAGS "-DTRAX=0 -DWIDTH=${SCENE_CPPWIDTH} -DHEIGHT=${SCENE_CPPHEIGHT} ${PTHREADS_DEFINITIONS}")
	source_group(TRAX_Sources FILES ${PRE_TRAX_SRC})
	source_group(TRAX_Headers FILES ${PRE_TRAX_HDR})


	############
	# generate TRAX target
	foreach(_file ${TRAX_TRAX_SOURCES})
		list(APPEND ${targetSources} ${directoryPrePend}${_file})
	endforeach(_file ${TRAX_TRAX_SOURCES})

	set(TARGET_BC_FILES)
	foreach(_file_cc ${${targetSources}})
		get_filename_component(file_cc ${_file_cc} NAME_WE)
		set(file_bc "BC/${file_cc}.bc")
#		message( ${directoryPrePend}${LLVM_BIN_DIR}${LLVM_GCC_LOC} -emit-llvm -DTRAX=1 ${addnlDef} -DWIDTH=${SCENE_TRXWIDTH} -DHEIGHT=${SCENE_TRXHEIGHT} -I${directoryPrePend}${TRAX_INCLUDE_DIR} -o ${file_bc} -c ${_file_cc} )
		add_custom_command( OUTPUT ${file_bc}
							COMMAND ${directoryPrePend}${LLVM_BIN_DIR}${LLVM_GCC_LOC} -emit-llvm -DTRAX=1 ${addnlDef} -DWIDTH=${SCENE_TRXWIDTH} -DHEIGHT=${SCENE_TRXHEIGHT} -I${directoryPrePend}${TRAX_INCLUDE_DIR} -o ${file_bc} -c ${_file_cc}
							DEPENDS ${file_cc})
		list(APPEND TARGET_BC_FILES ${file_bc})
	endforeach()

	# do we need the annoying .exe part?
	set(WIN_EXE_POSTPEND "")
	if(${SYS_IS_WIN})
		set(WIN_EXE_POSTPEND ".exe")
	endif(${SYS_IS_WIN})

	# do we need memory functions?
	set(MEMSET_FCNS_ADD "")
	if(${needMemset})
		set(MEMSET_FCNS_ADD "${directoryPrePend}${LIBDIR}/memset.s")
	endif(${needMemset})

	# now, actually do the linking, etc
	add_custom_command(OUTPUT "BC/${targetName}_rt.bc"    COMMAND ${directoryPrePend}${LLVM_BIN_DIR}${LLVM_LD_LOC} ${TARGET_BC_FILES} -o BC/${targetName}_rt DEPENDS ${TARGET_BC_FILES})
	add_custom_command(OUTPUT "BC/${targetName}_pre.ll"   COMMAND ${directoryPrePend}${LLVM_BIN_DIR}${LLVM_DIS_LOC} BC/${targetName}_rt${WIN_EXE_POSTPEND}.bc -o BC/${targetName}_pre.ll DEPENDS "BC/${targetName}_rt.bc")
	add_custom_command(OUTPUT "BC/${targetName}_rtopt.bc" COMMAND ${directoryPrePend}${LLVM_BIN_DIR}${LLVM_OPT_LOC} BC/${targetName}_rt${WIN_EXE_POSTPEND}.bc -O3 -inline-threshold 200000000 -o BC/${targetName}_rt${WIN_EXE_POSTPEND}.bc DEPENDS "BC/${targetName}_rt.bc" "BC/${targetName}_pre.ll")
	add_custom_command(OUTPUT "BC/${targetName}_post.ll"  COMMAND ${directoryPrePend}${LLVM_BIN_DIR}${LLVM_DIS_LOC} BC/${targetName}_rt${WIN_EXE_POSTPEND}.bc -o BC/${targetName}_post.ll DEPENDS "BC/${targetName}_rtopt.bc")
	add_custom_command(OUTPUT "BC/${targetName}_rt.s"     COMMAND ${directoryPrePend}${LLVM_BIN_DIR}${LLVM_LLC_LOC} BC/${targetName}_rt${WIN_EXE_POSTPEND}.bc -o BC/${targetName}_rt.s -march=trax DEPENDS "BC/${targetName}_rt.bc" "BC/${targetName}_rtopt.bc" "BC/${targetName}_post.ll")
	list(APPEND TARGET_BC_FILES ${file_bc} "BC/${targetName}_rt.bc" "BC/${targetName}_pre.ll" "BC/${targetName}_rtopt.bc" "BC/${targetName}_post.ll" "BC/${targetName}_rt.s")

	# add the final rt-llvm.s target, which depends on things like memset.s
	if(${needMemset})
		add_custom_command(OUTPUT "${targetName}_rt-llvm.s"   COMMAND python "${directoryPrePend}${TRAX_LN_PY_LOC} " BC/${targetName}_rt.s ${MEMSET_FCNS_ADD} > ${targetName}_rt-llvm.s DEPENDS "BC/${targetName}_rt.s" "${MEMSET_FCNS_ADD}" )	
		list(APPEND TARGET_BC_FILES "${MEMSET_FCNS_ADD}")
	else(${needMemset})
		add_custom_command(OUTPUT "${targetName}_rt-llvm.s"   COMMAND python "${directoryPrePend}${TRAX_LN_PY_LOC} " BC/${targetName}_rt.s > ${targetName}_rt-llvm.s DEPENDS "BC/${targetName}_rt.s")
	endif(${needMemset})
	list(APPEND TARGET_BC_FILES "${targetName}_rt-llvm.s")

	# final project target
	add_custom_target(TRAX_${targetName} DEPENDS ${TARGET_BC_FILES})
	
	# TODO: add install targets to copy all executables to some bin directory

endfunction( applyToTarget )


###################################################################
# Actual projects
add_subdirectory(src)
