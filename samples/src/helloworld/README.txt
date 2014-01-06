The basic hello world example.
The Makefile compiles two versions: the CPU executable binary "run_rt" and the simtrax assembly file "rt-llvm.s"

Run the functional simulator with the command:
./run_rt --no-scene

Run the simtrax (cycle accurate) simulator from the sim directory with:
./simtrax --no-scene --load-assembly <path to helloworld/rt-llvm.s>


The --no-scene argument causes the simulator to not load a default model, light, and camera, since this example does not require them.

See the simulation wiki page for more details:
http://code.google.com/p/simtrax/wiki/Simulation
