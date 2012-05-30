Each of the functional units is listed as follows:
<unit name> <latency> <issue width> <per unit area um^2> <power mW>

The caches are handled a little differently since they have more
parameters. L1 and L2 are as follows:
<L1 | L2> <hit latency> <cache size> <num banks> <line size> <cache area um^2> <power mW>

Main memory is as follows:
MEMORY <latency> <number of memory blocks>

Stream memory is as follows:
STREAM <latency> <size> <memory area um^2> <power mW>

When the config is loaded there is one L1 created per core(TM) and one
L2 per L2 at the command line with everything duplicated per L2. Only
a single instance of main memory and stream memory is instantiated.


