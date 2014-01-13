# file: <path>/simtrax/samples/src/helloworld/CMakeLists.txt


# Write out all of the sources here
# Note that the paths are relative to directory where this CMakeLists.txt is located
set(HELLOWORLD_SRC
	helloworld.cc
)

# Write out all of the headers here
set(HELLOWORLD_HDR
)

# get the project set up
# parameters: helloworld - basename of the project. Results in helloworld.exe and helloworld_rt-llvm.s
#             HELLOWORLD_SRC - source files list from above
#             HELLOWORLD_HDR - header files list from above
#             the other 2 should just be "" each
applyToTarget(helloworld HELLOWORLD_SRC HELLOWORLD_HDR "" "")
