# file: <path>/simtrax/samples/src/mandelbot/CMakeLists.txt


# set up our sources and headers
set(MANDELBROT_SRC
	mandelbrot.cc
)

set(MANDELBROT_HDR
	Color.h
	Image.h
)

# get the project set up
applyToTarget(mandelbrot MANDELBROT_SRC MANDELBROT_HDR "" "")