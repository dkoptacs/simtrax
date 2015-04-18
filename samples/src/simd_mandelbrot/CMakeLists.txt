# file: <path>/simtrax/samples/src/mandelbot/CMakeLists.txt


# Write out all of the sources here
# Note that the paths are relative to directory where this CMakeLists.txt is located
set(MANDELBROT_SRC
	mandelbrot.cc
	Image.cc
	Color.cc
)

# Write out all of the headers here
set(MANDELBROT_HDR
	Color.h
	Image.h
)

# get the project set up
# parameters: mandelbrot - basename of the project. Results in mandelbrot.exe and mandelbrot_rt-llvm.s
#             MANDELBROT_SRC - source files list from above
#             MANDELBROT_HDR - header files list from above
#             the other 2 should just be "" each
applyToTarget(mandelbrot MANDELBROT_SRC MANDELBROT_HDR "" "")