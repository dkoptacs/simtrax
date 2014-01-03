# file: <path>/simtrax/samples/src/gradient/CMakeLists.txt


# set up our sources and headers
set(GRADIENT_SRC gradient.cc
)

set(GRADIENT_HDR
)

# get the project set up
applyToTarget(gradient GRADIENT_SRC GRADIENT_HDR "" "")
