This sample implements a simple Kajiya pathtracer. It is meant to be
an example starting point for writing a ray tracer in TRaX, and is not
meant to be a full-featured renderer. Many of the features are naive
implementations, for example the sampling techniques.

The Makefile compiles two versions: the CPU executable binary "run_rt" and the simtrax assembly file "rt-llvm.s"

Run the functional simulator with the command:
./run_rt
By default run_rt will load the Cornell Box scene.
Run with --help to see a list of available command-line options.

Run the simtrax (cycle accurate) simulator from the sim directory, for
example:
./simtrax --load-assembly ../samples/src/pathtracer/rt-llvm.s \
--model ../samples/scenes/cornell/CornellBox.obj \
--view-file ../samples/scenes/cornell/cornell.view \
--light-file ../samples/scenes/cornell/cornell.light

Useful options for this renderer:
--num-samples
--ray-depth

Run with --help to see a list of available command-line options.

See the simulation wiki page for more details:
http://code.google.com/p/simtrax/wiki/Simulation
