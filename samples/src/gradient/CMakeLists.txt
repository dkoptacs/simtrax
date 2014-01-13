# file: <path>/simtrax/samples/src/gradient/CMakeLists.txt


# Write out all of the sources here
# Note that the paths are relative to directory where this CMakeLists.txt is located
set(GRADIENT_SRC
	gradient.cc
)

# Write out all of the headers here
set(GRADIENT_HDR
)

# get the project set up
# parameters: gradient - basename of the project. Results in gradient.exe and gradient_rt-llvm.s
#             GRADIENT_SRC - source files list from above
#             GRADIENT_HDR - header files list from above
#             the other 2 should just be "" each
applyToTarget(gradient GRADIENT_SRC GRADIENT_HDR "" "")
