MSA vectorized version provided by Will Usher.

This sample generates a Mandelbrot fractal image. 
The Makefile does not compiler a CPU executable like other samples
do. Since this sample uses MIPS intrinsics, it only generates a simtrax assembly file "rt-llvm.s"

Run the functional simulator with the command:
./run_rt --no-scene

Run the simtrax (cycle accurate) simulator from the sim directory with:
./simtrax --no-scene --load-assembly <path to mandelbrot/rt-llvm.s>

The --no-scene argument causes the simulator to not load a default model, light, and camera, since this example does not require them.

See the simulation wiki page for more details:
http://code.google.com/p/simtrax/wiki/Simulation
