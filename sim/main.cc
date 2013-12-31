#include "Bitwise.h"
#include "BranchUnit.h"
#include "BVH.h"
#include "Animation.h"
#include "Camera.h"
#include "ConversionUnit.h"
#include "CustomLoadMemory.h"
#include "DebugUnit.h"
#include "FourByte.h"
#include "FPAddSub.h"
#include "FPCompare.h"
#include "FPInvSqrt.h"
#include "FPMinMax.h"
#include "FPMul.h"
#include "FunctionalUnit.h"
#include "GlobalRegisterFile.h"
#include "Instruction.h"
#include "IntAddSub.h"
#include "IntMul.h"
#include "IssueUnit.h"
#include "IWLoader.h"
#include "L1Cache.h"
#include "L2Cache.h"
#include "LoadMemory.h"
#include "LocalStore.h"
#include "MainMemory.h"
#include "OBJLoader.h"
#include "ReadConfig.h"
#include "ReadViewfile.h"
#include "ReadLightfile.h"
#include "SimpleRegisterFile.h"
#include "Synchronize.h"
#include "ThreadState.h"
#include "ThreadProcessor.h"
#include "TraxCore.h"
#include "Triangle.h"
#include "Vector3.h"
#include "Assembler.h"
#include "usimm.h"
#include "memory_controller.h"
#include "params.h"

#include "WinCommonNixFcns.h"
// Windows pipe stuff (needs stdio.h)
#ifdef WIN32
# ifndef popen
#	define popen _popen
#	define pclose _pclose
# endif
#endif

#include <float.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <math.h>


pthread_mutex_t atominc_mutex;
pthread_mutex_t memory_mutex;
pthread_mutex_t sync_mutex;
pthread_mutex_t global_mutex;
pthread_mutex_t usimm_mutex[MAX_NUM_CHANNELS];
pthread_cond_t sync_cond;
// for synchronization
int global_total_simulation_threads;
int current_simulation_threads;
bool disable_usimm;

// this branch delay is reset by code that finds the delay from the
// config file
int BRANCH_DELAY = 0;
L2Cache** L2s;
unsigned int num_L2s;

// Utility for tracking simulation time
double clockdiff(clock_t clock1, clock_t clock2) {
  double clockticks = clock2-clock1;
  return (clockticks * 1) / CLOCKS_PER_SEC; // time in seconds
}

void SystemClockRise(std::vector<HardwareModule*>& modules) {
  for (size_t i = 0; i < modules.size(); i++) {
    modules[i]->ClockRise();
  }
}

void SystemClockFall(std::vector<HardwareModule*>& modules) {
  for (size_t i = 0; i < modules.size(); i++) {
    modules[i]->ClockFall();
  }
}

void PrintSystemInfo(long long int& cycle_num,
                     std::vector<HardwareModule*>& modules,
                     std::vector<std::string>& names)
{
  printf("Cycle %lld:\n", cycle_num++);
  for (size_t i = 0; i < modules.size(); i++) {
    printf("\t%s: ", names[i].c_str());
    modules[i]->print();
    printf("\n");
  }
  printf("\n");
}

void TrackUtilization(std::vector<HardwareModule*>& modules,
                      std::vector<double>& sums) {
  for (size_t i = 0; i < modules.size(); i++) {
    sums[i] += modules[i]->Utilization();
  }
}

void NormalizeUtilization(long long int cycle_num, std::vector<double>& sums) {
  for (size_t i = 0; i < sums.size(); i++) {
    sums[i] /= cycle_num;
  }
}

void PrintUtilization(std::vector<std::string>& module_names,
                      std::vector<double>& utilization) {
  printf("Module Utilization\n\n");
//   for (size_t i = 0; i < utilization.size()-6; i++) {
  for (size_t i = 0; i < module_names.size(); i++) {
    //printf("i = %d\n", (int)i);
    //fflush(stdout);
    if (utilization[i] > 0.)
      printf("\t%20s:  %6.2f\n", module_names[i].c_str(), 100.f * utilization[i]);
  }
  printf("\n");
}

// runs cores "start_core" through "end_core - 1"
struct CoreThreadArgs {
  int start_core;
  int end_core;
  int thread_num;
  long long int stop_cycle;
  std::vector<TraxCore*>* cores;
};


void SyncThread( CoreThreadArgs* core_args ) {
  // synchronizes a thread
  pthread_mutex_lock(&sync_mutex);
  current_simulation_threads--;
  if (current_simulation_threads > 0) {
    pthread_cond_wait(&sync_cond, &sync_mutex);
  }
  else {
    // last thread signal the others and sync caches
    for(size_t i = 0; i < num_L2s; i++)
      {
	L2s[i]->ClockRise();
	L2s[i]->ClockFall();
      }

    //DK: One thread updates the DRAM
    // 5 DRAM cycles per trax cycle
    // (assuming 1GHz trax, 5GHz GDDR5)
    if(!disable_usimm)
      {
	for(int i=0; i < DRAM_CLOCK_MULTIPLIER; i++)
	  usimmClock();
      }

    current_simulation_threads = global_total_simulation_threads;
    pthread_cond_broadcast(&sync_cond);
  }
  pthread_mutex_unlock(&sync_mutex);
}

void UsimmUpdate()
{
  pthread_mutex_lock(&sync_mutex);
  current_simulation_threads--;
  if (current_simulation_threads > 0) {
    pthread_cond_wait(&sync_cond, &sync_mutex);
  }
  else {
    //DK: One thread updates the DRAM
    // 5 DRAM cycles per trax cycle
    // (assuming 1GHz trax, 5GHz GDDR5)
    for(int i=0; i < DRAM_CLOCK_MULTIPLIER; i++)
      usimmClock();

    current_simulation_threads = global_total_simulation_threads;
    pthread_cond_broadcast(&sync_cond);
  }
  pthread_mutex_unlock(&sync_mutex);
}


void *CoreThread( void* args ) {
  CoreThreadArgs* core_args = static_cast<CoreThreadArgs*>(args);
  long long int stop_cycle = core_args->stop_cycle;
  printf("Thread %d running cores\t%d to\t%d ...\n", (int) core_args->thread_num, (int) core_args->start_core, (int) core_args->end_core-1);
  // main loop for this core
  while (true) {
    // Choose the first core to issue from
    int start_core = 0;
    long long int max_stall_cycles = -1;
    for (int i = core_args->start_core; i < core_args->end_core; ++i) {
      long long int stall_cycles = (*core_args->cores)[i]->CountStalls();
      if (stall_cycles > max_stall_cycles) {
	start_core = i - core_args->start_core;
	max_stall_cycles = stall_cycles;
      }
    }

    int num_cores = core_args->end_core - core_args->start_core;
    for (int i = 0; i < num_cores; ++i) {
      int core_id = ((i + start_core) % num_cores) + core_args->start_core;
      SystemClockRise((*core_args->cores)[core_id]->modules);
      SystemClockFall((*core_args->cores)[core_id]->modules);
    }
    SyncThread(core_args);

    //UsimmUpdate();
    bool all_done = true;
    for (int i = core_args->start_core; i < core_args->end_core; ++i) {
      TrackUtilization((*core_args->cores)[i]->modules,
		       (*core_args->cores)[i]->utilizations);
      (*core_args->cores)[i]->cycle_num++;
      if (!(*core_args->cores)[i]->issuer->halted) {
	all_done = false;
      }
    }
    if ((*core_args->cores)[0]->cycle_num == stop_cycle || all_done) {
      break;
    }
  }
  pthread_mutex_lock(&sync_mutex);
  global_total_simulation_threads--;
  current_simulation_threads--;
  if (current_simulation_threads == 0) {
    // if this was the last thread wake the others.
    current_simulation_threads = global_total_simulation_threads;
    pthread_cond_broadcast(&sync_cond);
  }
  pthread_mutex_unlock(&sync_mutex);
  return 0;
}


void SerialExecution(CoreThreadArgs* core_args, int num_cores)
{
  printf("Cores 0 through %d running (serial execution mode)\n", num_cores - 1);

  while (true) {
    bool all_halted = true;
	//TODO: put the core pointers into a vector (or some kind of collection),
    // and remove them when they halt, to cut down this loop... (or just swap halted ones to the end of the array?
    std::vector<TraxCore*>* cores = core_args[0].cores;
    long long int stop_cycle = core_args[0].stop_cycle;
    for(int i=0; i < num_cores; i++)
      {
	TraxCore* core = (*cores)[i];
	if(core->issuer->halted)
	  continue;
	all_halted = false;
	SystemClockRise(core->modules);
	SystemClockFall(core->modules);
	TrackUtilization(core->modules, core->utilizations);
	core->cycle_num++;
	if (core->cycle_num == stop_cycle)
	  core->issuer->halted = true;
      }
    if(all_halted)
      break;
  }
}


void printUsage(char* program_name) {
  printf("%s\n", program_name);
  printf("  + Simulator Parameters:\n");
  printf("\t--atominc-report       <number of cycles between reporting number of atomicincs -- default 1000000, 0 means off>\n");
  printf("\t--custom-mem-loader    [which custom mem loader to use -- default 0 (off)]\n");
  printf("\t--incremental-output   <number of stores between outputs -- default 64>\n");
  printf("\t--issue-verbosity      <level of verbosity for issue unit -- default 0>\n");
  printf("\t--load-mem-file        [read memory dump from file]\n");
  printf("\t--mem-file             <memory dump file name -- default memory.mem>\n");
  printf("\t--memory-trace         [writes memory access data (cache only) to memory_accesses.txt]\n");
  printf("\t--print-instructions   [print contents of instruction memory]\n");
  printf("\t--print-symbols        [print symbol table generated by assembler]\n");
  printf("\t--proc-register-trace  <proc id to trace> [trace saved in thread_registers.txt]\n");
  printf("\t--serial-execution     [use a single pthread to run simulation]\n");
  printf("\t--simulation-threads   <number of simulator threads. -- default 1>\n");
  printf("\t--stop-cycle           <final cycle>\n");
  printf("\t--write-dot            <depth> generates dot files for the tree after each frame. Depth should not exceed 8\n");
  printf("\t--write-mem-file       [write memory dump to file]\n");

  printf("\n");
  printf("  + TRAX Specification:\n");
  printf("\t--with-per-cycle     [enable per-cycle output -- currently broken]\n");
  printf("\t--no-cpi             [disable print of CPI at end]\n");
  printf("\t--cache-snoop        [enable nearby L1 snooping]\n");
  printf("\t--num-regs           <number of registers -- default 128>\n");
  printf("\t--num-globals        <number of global registers -- default 8>\n");
  printf("\t--num-thread-procs   <number of threads per TM -- default 4>\n");
  printf("\t--threads-per-proc   <number of hyperthreads per TP>\n");
  printf("\t--simd-width         <number of threads issuing in SIMD within a TM>\n");
  printf("\t--num-cores          <number of cores (Thread Multiprocessors) -- default 1>\n");
  printf("\t--num-l2s            <number of L2s. All resources are multiplied by this number. -- default 1>\n");
  printf("\t--l1-off             [turn off the L1 data cache and set latency to 0]\n");
  printf("\t--l2-off             [turn off the L2 data cache and set latency to 0]\n");
  printf("\t--l1-read-copy       [turn on read replication of same word reads on the same cycle]\n");
  printf("\t--num-icache-banks   <number of banks per icache -- default 32>\n");
  printf("\t--num-icaches        <number of icaches in a core. Should be a power of 2 -- default 1>\n");
  printf("\t--disable-usimm      [use naive DRAM simulation instead of usimm]\n");

  printf("\n");
  printf("   + Files:\n");
  printf("\t--config-file     <config file name>\n");
  printf("\t--light-file      <light file name>\n");
  printf("\t--load-assembly   <filename -- loads the default if not specified>\n");
  printf("\t--model           <model file name (.obj)>\n");
  printf("\t--output-prefix   <prefix for image output. Be sure any directories exist>\n");
  printf("\t--usimm-config    <usimm config file name>\n");
  printf("\t--view-file       <view file name>\n");

  printf("\n");
  printf("  + Scene Parameters:\n");
  printf("\t--epsilon         <small number used for various offsets, default 1e-4>\n");
  printf("\t--far-value       <far clipping plane (for rasterizer only) -- default 1000>\n");
  printf("\t--first-keyframe  <specify the first keyframe for an animation>\n");
  printf("\t--height          <height in pixels -- default 128>\n");
  printf("\t--image-type      <type for image output -- default png>\n");
  printf("\t--no-png          [disable png output]\n");
  printf("\t--no-scene\n");
  printf("\t--num-frames      <number of frames to animate over keyframes>\n");
  printf("\t--num-samples     <number of samples per pixel -- default 1>\n");
  printf("\t--ray-depth       <depth of rays -- default 1>\n");
  printf("\t--rebuild-every   <N (rebuild BVH every N frames) -- default 0>\n");
  printf("\t--tile-height     <height of tile in pixels -- default 16>\n");
  printf("\t--tile-width      <width of tile in pixels -- default 16>\n");
  printf("\t--usegrid         <grid dimensions> [uses grid acceleration structure]\n");
  printf("\t--width           <width in pixels -- default 128>\n");


  printf("\n");
  printf("  + Other:\n");
  printf("\t--pack-split-axis         [BVH nodes will pack split axis in the 2nd byte and num_children in the 1st byte (lsb) of 6th word]\n");
  printf("\t--pack-stream-boundaries  [BVH nodes will pack their parent and child subtree IDs in to one word, instead of saving their own subtree ID]\n");
  printf("\t--scheduling              <\"poststall\", \"prestall\", \"simple\">\n");
  printf("\t--subtree-size            <Minimum size in words of subtrees built in to BVH -- default 0 (will not build subtrees)>\n");
  printf("\t--triangles-store-edges   [set flag to store 2 edge vecs in a tri instead of 2 verts -- default: off]\n");
}


int main(int argc, char* argv[]) {
  clock_t start_time                    = clock();
  bool print_system_info                = false;
  bool print_cpi                        = true;
  bool print_png                        = true;
  bool cache_snoop                      = false;
  int image_width                       = 128;
  int image_height                      = 128;
  int grid_dimensions                   = -1;
  int num_regs                          = 128;
  int num_globals                       = 8;
  int num_thread_procs                  = 1;
  int threads_per_proc                  = 1;
  int simd_width                        = 1;
  int num_frames                        = 1;
  int rebuild_frequency                 = 0;
  bool duplicate_bvh                    = false;
  int frames_since_rebuild              = 0;
  unsigned int num_cores                = 1;
  num_L2s                               = 1;
  bool l1_off                           = false;
  bool l2_off                           = false;
  bool l1_read_copy                     = false;
  long long int stop_cycle              = -1;
  char* config_file                     = NULL;
  char* view_file                       = NULL;
  char* model_file                      = NULL;
  char* keyframe_file                   = NULL;
  char* light_file                      = NULL;
  char* output_prefix                   = (char*)"out";
  char* image_type                      = (char*)"png";
  float far                             = 1000;
  int dot_depth                         = 0;
  Camera* camera                        = NULL;
  float *light_pos                      = NULL;
  int tile_width                        = 16;
  int tile_height                       = 16;
  int ray_depth                         = 1;
  int num_samples                       = 1;
  float epsilon                         = 1e-4f;
  bool print_instructions               = false;
  bool no_scene                         = false;
  int issue_verbosity                   = 0;
  long long int atominc_report_period   = 1000000;
  int num_icaches                       = 1;
  int icache_banks                      = 32;
  char *assem_file                      = NULL;
  bool memory_trace                     = false;
  int proc_register_trace               = -1;
  bool print_symbols                    = false;
  char mem_file_orig[64]                = "memory.mem";
  char* mem_file                        = mem_file_orig;
  bool load_mem_file                    = false;
  bool write_mem_file                   = false;
  int custom_mem_loader                 = 0;
  bool incremental_output               = false;
  bool serial_execution                 = false;
  bool triangles_store_edges            = false;
  int stores_between_output             = 64;
  int subtree_size                      = 0;
  bool pack_split_axis                  = 0;
  bool pack_stream_boundaries           = 0;
  disable_usimm                         = 0; // globally defined for use above
  BVH* bvh;
  Animation *animation                  = NULL;
  ThreadProcessor::SchedulingScheme scheduling_scheme = ThreadProcessor::SIMPLE;
  int total_simulation_threads          = 1;
  char *usimm_config_file               = NULL;

  // Verify Instruction.h matches Instruction.cc in size.
  if (strncmp(Instruction::Opnames[Instruction::PROF].c_str(), "PROF", 4) != 0) {
    printf("Instruction.h does not match Instruction.cc. Please fix them.\n");
    exit(-1);
  }

  // If no arguments passed to simulator, print help and exit
  if (argc == 1) {
      printf("No parameters specified. Please see more information below:\n");
      printUsage(argv[0]);
      return -1;
  }

  for (int i = 1; i < argc; i++) {
    // print simulation details
    int argnum = i;
    printf("%s", argv[i]);
    if (strcmp(argv[i], "--with-per-cycle") == 0) {
      print_system_info = true;
    } else if (strcmp(argv[i], "--no-cpi") == 0) {
      print_cpi = false;
    } else if (strcmp(argv[i], "--no-png") == 0) {
      print_png = false;
    } else if (strcmp(argv[i], "--cache-snoop") == 0) {
      cache_snoop = true;
    } else if (strcmp(argv[i], "--width") == 0) {
      image_width = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--height") == 0) {
      image_height = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--usegrid") == 0) {
      grid_dimensions = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--num-regs") == 0) {
      num_regs = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--num-globals") == 0) {
      num_globals = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--num-thread-procs") == 0) {
      num_thread_procs = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--threads-per-proc") == 0) {
      threads_per_proc = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--simd-width") == 0) {
      simd_width = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--num-cores") == 0) {
      num_cores = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--num-l2s") == 0) {
      num_L2s = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--simulation-threads") == 0) {
      total_simulation_threads = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--l1-off") == 0) {
      l1_off = true;
    } else if (strcmp(argv[i], "--l2-off") == 0) {
      l2_off = true;
    } else if (strcmp(argv[i], "--l1-read-copy") == 0) {
      l1_read_copy = true;
    } else if (strcmp(argv[i], "--stop-cycle") == 0) {
      stop_cycle = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--config-file") == 0) {
      config_file = argv[++i];
    } else if (strcmp(argv[i], "--view-file") == 0) {
      view_file = argv[++i];
    } else if (strcmp(argv[i], "--model") == 0) {
      model_file = argv[++i];
    } else if (strcmp(argv[i], "--far-value") == 0) {
      far = static_cast<float>( atof(argv[++i]) );
    } else if (strcmp(argv[i], "--first-keyframe") == 0) {
      keyframe_file = argv[++i];
    } else if (strcmp(argv[i], "--num-frames") == 0) {
      num_frames = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--rebuild-every") == 0) {
      rebuild_frequency = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--light-file") == 0) {
      light_file = argv[++i];
    } else if (strcmp(argv[i], "--output-prefix") == 0) {
      output_prefix = argv[++i];
    } else if (strcmp(argv[i], "--image-type") == 0) {
      image_type = argv[++i];
    } else if (strcmp(argv[i], "--tile-width") == 0) {
      tile_width = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--tile-height") == 0) {
      tile_height = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--ray-depth") == 0) {
      ray_depth = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--epsilon") == 0) {
      epsilon = static_cast<float>( atof(argv[++i]) );
    } else if (strcmp(argv[i], "--num-samples") == 0) {
      num_samples = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--write-dot") == 0) {
      dot_depth = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--print-instructions") == 0) {
      print_instructions = true;
    } else if (strcmp(argv[i], "--no-scene") == 0) {
      no_scene = true;
    } else if (strcmp(argv[i], "--issue-verbosity") == 0) {
      issue_verbosity = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--atominc-report") == 0) {
      atominc_report_period = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--num-icache-banks") == 0) {
      icache_banks = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--num-icaches") == 0) {
      num_icaches = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--load-assembly") == 0) {
      assem_file = argv[++i];
    } else if (strcmp(argv[i], "--memory-trace") == 0) {
      memory_trace = true;
    } else if (strcmp(argv[i], "--proc-register-trace") == 0) {
      proc_register_trace = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--print-symbols") == 0) {
      print_symbols = true;
    } else if (strcmp(argv[i], "--mem-file") == 0) {
      mem_file = argv[++i];
    } else if (strcmp(argv[i], "--load-mem-file") == 0) {
      load_mem_file = true;
    } else if (strcmp(argv[i], "--write-mem-file") == 0) {
      write_mem_file = true;
    } else if (strcmp(argv[i], "--custom-mem-loader") == 0) {
      custom_mem_loader = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--incremental-output") == 0) {
      incremental_output = true;
      stores_between_output = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--serial-execution") == 0) {
      serial_execution = true;
    } else if (strcmp(argv[i], "--triangles-store-edges") == 0) {
      triangles_store_edges = true;
    } else if (strcmp(argv[i], "--subtree-size") == 0) {
      subtree_size = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--pack-split-axis") == 0) {
      pack_split_axis = true;
    } else if (strcmp(argv[i], "--pack-stream-boundaries") == 0) {
      pack_stream_boundaries = true;
    } else if (strcmp(argv[i], "--scheduling") == 0) {
      i++;
      if(strcmp(argv[i], "simple")==0)
        scheduling_scheme = ThreadProcessor::SIMPLE;
      if(strcmp(argv[i], "prestall")==0)
        scheduling_scheme = ThreadProcessor::PRESTALL;
      if(strcmp(argv[i], "poststall")==0)
        scheduling_scheme = ThreadProcessor::POSTSTALL;
    } else if (strcmp(argv[i], "--usimm-config") == 0) {
      usimm_config_file = argv[++i];
    } else if (strcmp(argv[i], "--disable-usimm") == 0) {
      disable_usimm = 1;
    } else {
      printf(" Unrecognized option %s\n", argv[i]);
      printUsage(argv[0]);
      return -1;
    }
    if(argnum!=i)
      printf(" %s", argv[i]);
    printf("\n");
  }

  if (print_system_info) {
    printf("Per cycle information currently not supported.\n");
  }

  if(rebuild_frequency > 0)
    duplicate_bvh = false;
  
  // size estimates
  double L2_size = 0;
  double core_size = 0;

  // Keep track of register names
  std::vector<symbol*> regs;

  // Assembler needs somewhere to store the jump table and string literals, to be loaded in to local stores after assembly
  std::vector<int> jump_table;
  std::vector<std::string> ascii_literals;

  // Instruction Memory
  std::vector<Instruction*> instructions;
  std::vector<TraxCore*> cores;
  L2s = new L2Cache*[num_L2s];
  MainMemory* memory;
  GlobalRegisterFile globals(num_globals, num_thread_procs * threads_per_proc * num_cores * num_L2s, atominc_report_period);

  if (config_file == NULL) {
    config_file = (char*)"../samples/configs/default.config";
    printf("No configuration specified, using default: %s\n", config_file);
  }
  //if (config_file != NULL) {
  // Set up memory from config (L2 and main memory)
  ReadConfig config_reader(config_file, L2s, num_L2s, memory, L2_size, disable_usimm, memory_trace, l1_off, l2_off, l1_read_copy);

  // loop through the L2s
  for (size_t l2_id = 0; l2_id < num_L2s; ++l2_id) {
    L2Cache* L2 = L2s[l2_id];
    // load the cores
    for (size_t i = 0; i < num_cores; ++i) {
      size_t core_id = i + num_cores * l2_id;
      printf("Loading core %d.\n", (int)core_id);
      // only one computation is needed... we'll end up with the last one after the loop
      core_size = 0;
      TraxCore *current_core = new TraxCore(num_thread_procs, threads_per_proc, num_regs, scheduling_scheme, &instructions, L2, core_id, l2_id);
      config_reader.current_core = current_core;
      config_reader.LoadConfig(L2, core_size);
      current_core->modules.push_back(&globals);
      current_core->functional_units.push_back(&globals);
      if (proc_register_trace > -1 && i == 0) {
	current_core->EnableRegisterDump(proc_register_trace);
	//current_core->register_files[thread_register_trace]->EnableDump(thread_trace_file);
      }
      // Add Sync unit
      Synchronize *sync_unit = new Synchronize(0,0,simd_width,num_thread_procs);
      current_core->modules.push_back(sync_unit);
      current_core->functional_units.push_back(sync_unit);
      // Add any other custom units before the push_back
      LocalStore *ls_unit = new LocalStore(1, num_thread_procs);
      current_core->modules.push_back(ls_unit);
      current_core->functional_units.push_back(ls_unit);
      current_core->SetSymbols(&regs);

      cores.push_back(current_core);
    }
    // adding register file size
    // 128, 32-bit registers is 22000um in 65nm lp (made up number - from scaling)
    // 128, 32-bit registers is 19470um in 65nm from Cacti
    float size_of_one_reg = 0.01947f / 128.0f;
    core_size += num_regs * num_thread_procs * threads_per_proc * size_of_one_reg;
  }


  //} else {
  //printf("ERROR: No configuration supplied.\n");
  //return -1;
  //}
  
  // set up L1 snooping if enabled
  if (cache_snoop) {
    for (size_t i = 0; i*4 < num_cores * num_L2s; ++i) {
      L1Cache* L1_0=NULL, *L1_1=NULL, *L1_2=NULL, *L1_3=NULL;
      L1_0 = cores[i*4]->L1;
      if (num_cores > i*4+1) {
	L1_1 = cores[i*4+1]->L1;
	L1_1->L1_1 = L1_0;
	L1_0->L1_1 = L1_1;
      }
      if (num_cores > i*4+2) {
	L1_2 = cores[i*4+2]->L1;
	L1_2->L1_1 = L1_0;
	L1_2->L1_2 = L1_1;
	L1_1->L1_2 = L1_2;
	L1_0->L1_2 = L1_2;
      }
      if (num_cores > i*4+3) {
	L1_3 = cores[i*4+3]->L1;
	L1_3->L1_1 = L1_0;
	L1_3->L1_2 = L1_1;
	L1_3->L1_3 = L1_2;
	L1_2->L1_3 = L1_3;
	L1_1->L1_3 = L1_3;
	L1_0->L1_3 = L1_3;
      }
    }
  }

  // find maximum branch delay
  //TODO: Why do we still need this? Compiler should use a base branch delay slot size
  std::vector<FunctionalUnit*> functional_units = cores[0]->functional_units;
  for (size_t i = 0; i < functional_units.size(); i++) {
    BranchUnit* brancher = dynamic_cast<BranchUnit*>(functional_units[i]);
    if (brancher) {
      int latency = brancher->GetLatency();
      if (latency > BRANCH_DELAY) BRANCH_DELAY = latency;
    }
  }

  int start_wq, start_framebuffer, start_scene, start_camera, start_bg_color, start_light, end_memory;
  int start_matls, start_permutation;

  if (load_mem_file && mem_file != NULL) {
    // load memory from file
    printf("Loading Memory from file '%s'.\n", mem_file);
    memory->LoadMemory( mem_file, start_wq, start_framebuffer, start_scene, 
			start_matls, start_camera, start_bg_color, start_light, end_memory,
			light_pos, start_permutation );
    printf("Memory Loaded.\n");

    start_framebuffer = memory->data[7].ivalue;

  } else { // (need to skip a lot of these if memory loading really works)

    if (custom_mem_loader) {
      // run custom loader
      CustomLoadMemory(memory->getData(), memory->getSize(), image_width, image_height,
		       epsilon, custom_mem_loader);
    } else {
      // default memory loader 
      if (view_file != NULL) {
	camera = ReadViewfile::LoadFile(view_file, far);
      } else if(!no_scene){
	printf("ERROR: No camera file supplied.\n");
	return -1;
      }
      
      // just set the model file equal to the keyframe file and load it the same way
      // first frame will be loaded exactly the same way, memory loader doesn't need to change.
      // subsequent frames will be updated by the Animation, directly in simulator's memory
      if(keyframe_file != NULL)
	model_file = keyframe_file;
      
      if (!no_scene && model_file == NULL) {
	printf("ERROR: No model data supplied.\n");
	return -1;
      }
      
      
      if (light_file != NULL) {
	light_pos = new float[3];
	ReadLightfile::LoadFile(light_file, light_pos);
      } else {
	light_pos = new float[3];
	//sponza light as default
	light_pos[0] = 7.97f;
	light_pos[1] = 1.4f;
	light_pos[2] = -1.74f;
      }
    
      LoadMemory(memory->getData(), bvh, memory->getSize(), image_width, image_height,
		 grid_dimensions,
		 camera, model_file,
		 start_wq, start_framebuffer, start_scene,
		 start_matls,
		 start_camera, start_bg_color, start_light, end_memory,
		 light_pos, start_permutation, tile_width, tile_height,
		 ray_depth, num_samples, num_thread_procs * num_cores, num_cores, subtree_size, 
		 epsilon, duplicate_bvh, triangles_store_edges, pack_split_axis, pack_stream_boundaries);
    }
  } // end else for memory dump file

  // Once the model has been loaded and the BVH has been built, set up the animation if there is one
  
  if(keyframe_file != NULL)
  {
      if(duplicate_bvh)
	{
	  printf("primary bvh starts at %d, secondary at %d\n", bvh->start_nodes, bvh->start_secondary_nodes);
	  printf("primary triangles start at %d, secondary at %d\n", bvh->start_tris, bvh->start_secondary_tris);
	  animation = new Animation(keyframe_file, num_frames, memory->getData(), bvh->num_nodes,
				    &bvh->tri_orders, bvh->inorder_tris.size(), 
				    bvh->start_tris, bvh->start_nodes,
				    bvh->start_secondary_tris, bvh->start_secondary_nodes);
	}
      else
	{
	  animation = new Animation(keyframe_file, num_frames, memory->getData(), bvh->num_nodes,
				    &bvh->tri_orders, bvh->inorder_tris.size(), 
				    bvh->start_tris, bvh->start_nodes);
	}
  }
  
  // Set up incremental output if option is specified
  if (incremental_output) {
    memory->image_width = image_width;
    memory->image_height = image_height;
    memory->incremental_output = incremental_output;
    memory->start_framebuffer = start_framebuffer;
    memory->stores_between_output = stores_between_output;
  }

  // to check the memory contents
  printf( "start_wq: %d\nstart_fb: %d\nstart_scene: %d\nstart_camera: %d\n",
	  start_wq, start_framebuffer, start_scene, start_camera);
  printf( "start_matls: %d\nstart_bg_color: %d\nstart_light: %d\nstart_permutation: %d\n",
	  start_matls, start_bg_color, start_light, start_permutation);

  int memory_size = memory->getSize();

  if (end_memory > memory_size) {
    printf("ERROR: Scene requires %d blocks while memory_size is only %d\n", end_memory, memory_size);
    exit(-1);
  }

  // TODO: tmp - set up the main image to look purple! ===============================================================================================
/*	for (int j = image_height - 1; j >= 0; j--) {
		for (int i = 0; i < image_width; i++) {
			int index = start_framebuffer + 3 * (j * image_width + i);
			memory->getData()[index+0].fvalue = 1.f;
			memory->getData()[index+1].fvalue = 0.f;
			memory->getData()[index+2].fvalue = 1.f;
		}
	}
*/


  // Write memory dump
  if (write_mem_file && mem_file != NULL) {
    // load memory from file
    printf("Dumping memory to file '%s'.\n", mem_file);
    memory->WriteMemory( mem_file, start_wq, start_framebuffer, start_scene, 
			 start_matls, start_camera, start_bg_color, start_light, end_memory,
			 light_pos, start_permutation );
    printf("Memory dump complete.\n");
    printf("Exiting without simulating.\n");
    return 0;
  }

  // Done preparing units, fill in instructions
  // declaration moved above
  //std::vector<Instruction*> instructions;

  if(assem_file!=NULL)
    {
      int numRegs = Assembler::LoadAssem(assem_file, instructions, regs, num_regs, jump_table, ascii_literals, start_wq, start_framebuffer, start_camera, start_scene, start_light, start_bg_color, start_matls, start_permutation, print_symbols);
      if(numRegs<=0)
        {
          printf("assembler returned an error, exiting\n");
          exit(-1);
        }
      printf("assembly uses %d registers\n", numRegs);
    }
  else
    {
      printf("Error: no assembly program specified\n");
      return -1;
    //loadStandardRayTracer(instructions, start_wq, start_framebuffer, start_camera, start_scene, start_light, start_bg_color);
    }

  int last_instruction = static_cast<int>(instructions.size());
  printf("Number of instructions: %d\n\n", last_instruction);

  //float initial_bvh_cost = bvh->computeSAHCost(start_scene, memory->getData());
  if(dot_depth > 0)
    bvh->writeDOT("bvh.dot", start_scene, memory->getData(), 1, dot_depth);
  // Once the machine finishes this it'll halt

  // icache area estimate
  // 128, 32-bit registers is 22000um in 65nm lp
  //float size_of_one_reg = .022/128.;
  //core_size += static_cast<float>(last_instruction) * num_icaches * size_of_one_reg;
  // removed in favor of using Cacti cache numbers

  if (print_instructions) {
    printf("Instruction listing:\n");
    for (int i = 0; i < last_instruction; ++i) {
      instructions[i]->print();
      printf("\n");
    }
  }

  // initialize the cores
  for (size_t i = 0; i < cores.size(); ++i) {
    cores[i]->initialize(issue_verbosity, num_icaches, icache_banks, simd_width, jump_table, ascii_literals);
  }

  // Check that there are units for each instruction in the program
  for (int i = 0; i < last_instruction; ++i) {
    // Exceptions for register file and misc
    if (instructions[i]->op == Instruction::HALT ||
	instructions[i]->op == Instruction::BARRIER ||
	instructions[i]->op == Instruction::MOV ||
	instructions[i]->op == Instruction::LOADIMM ||
	instructions[i]->op == Instruction::MOVINDRD ||
	instructions[i]->op == Instruction::MOVINDWR ||
	instructions[i]->op == Instruction::NOP ||
	instructions[i]->op == Instruction::PROF ||
	instructions[i]->op == Instruction::SETBOXPIPE ||
	instructions[i]->op == Instruction::SETTRIPIPE ||
	instructions[i]->op == Instruction::SLEEP) {
      continue;
    }
    bool op_found = false;
    for (size_t j = 0; j < cores[0]->issuer->units.size(); j++) {
      if (cores[0]->issuer->units[j]->SupportsOp(instructions[i]->op)) {
	op_found = true;
      }
    }
    if (!op_found) {
      printf("Instruction not supported in current config. Try a different config or add the following instruction to the exception list in main_new.cc\n");
      instructions[i]->print();
      exit(-1);
    }
  }

  if(!disable_usimm)
    {
      if (usimm_config_file == NULL) {
	//usimm_config_file = "configs/usimm_configs/1channel.cfg";
	//usimm_config_file = "configs/usimm_configs/4channel.cfg";
	usimm_config_file = (char*)"../samples/configs/usimm_configs/gddr5.cfg";
	printf("No USIMM configuration specified, using default: %s\n", usimm_config_file);
      }
      usimm_setup(usimm_config_file);
    }
  
  // Limit simulation threads to the number of cores
  if (total_simulation_threads > (int)(num_cores * num_L2s)) {
    total_simulation_threads = num_cores * num_L2s;
  }
  if (total_simulation_threads < 1) {
    total_simulation_threads = 1;
  }
  global_total_simulation_threads = total_simulation_threads;

  // set up thread arguments
  pthread_attr_t attr;
  pthread_t *threadids = new pthread_t[total_simulation_threads];
  CoreThreadArgs *args = new CoreThreadArgs[total_simulation_threads];
  pthread_mutex_init(&atominc_mutex, NULL);
  pthread_mutex_init(&memory_mutex, NULL);
  pthread_mutex_init(&sync_mutex, NULL);
  pthread_mutex_init(&global_mutex, NULL);
  for(int i=0; i < MAX_NUM_CHANNELS; i++)
    pthread_mutex_init(&(usimm_mutex[i]), NULL);
  pthread_cond_init(&sync_cond, NULL);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // initialize variables for synchronization
  current_simulation_threads = total_simulation_threads;
  
  int cores_per_thread = (num_cores * num_L2s) / total_simulation_threads + 1;
  int remainder_threads = (num_cores * num_L2s) % total_simulation_threads;
  int start_core = 0;
  for (int i = 0; i < total_simulation_threads; ++i) {
    if (remainder_threads == i) {
      cores_per_thread--;
    }
    args[i].start_core = start_core;
    args[i].end_core = start_core + cores_per_thread;
    start_core += cores_per_thread;
    args[i].thread_num = i;
    args[i].stop_cycle = stop_cycle;
    args[i].cores = &cores;
  }
  // Have the last thread do the remainder
  args[total_simulation_threads - 1].end_core = num_cores * num_L2s;
  
  clock_t setup_time = clock();
  printf("\t <== Setup time: %12.1f s ==>\n", clockdiff(start_time, setup_time));
  clock_t prev_frame_time;
  clock_t curr_frame_time;


  while(true)
  {  
    prev_frame_time = clock();
      if(serial_execution)
	SerialExecution(args, num_cores * num_L2s);      
      else
	{
	  for (int i = 0; i < total_simulation_threads; ++i) {
	    printf("Creating thread %d...\n", (int)i);
	    pthread_create( &threadids[i], &attr, CoreThread, (void *)&args[i] );
	  }
	  
	  // Wait for machine to halt
	  for (int i = 0; i < total_simulation_threads; ++i) {
	    pthread_join( threadids[i], NULL );
	  }
	}
      curr_frame_time = clock();
      printf("\t <== Frame time: %12.1f s ==>\n", clockdiff(prev_frame_time, curr_frame_time));
      
      
      // Machine has halted, take a look
      if(animation != NULL)
	{
	  printf("verifying BVH...\n");
	  if(duplicate_bvh)
	    bvh->verifyTree(animation->getRotateAddress(), memory->getData(), 0);
	  else
	    bvh->verifyTree(animation->start_nodes, memory->getData(), 0);
	  //printf("secondary BVH...\n");
	  //bvh->verifyTree(bvh->start_secondary_nodes, memory->getData(), 0);
	  printf("BVH verification complente\n");
	}

      
      //printf("Initial total BVH SAH cost: %f\n", initial_bvh_cost);
      if(animation != NULL && dot_depth > 0)
	{
	  char dotfile[80];
	  sprintf(dotfile, "rotatedBVH%d.dot", animation->current_frame);
	  printf("BVH 1 SAH cost: %f\n", bvh->computeSAHCost(animation->getRotateAddress(), memory->getData()));
	  bvh->writeDOT(dotfile, animation->getRotateAddress(), memory->getData(), 0, dot_depth);
	}
      
      //printf("BVH 2 SAH cost: %f\n", bvh->computeSAHCost(bvh->start_secondary_nodes, memory->getData()));
      
      if (print_cpi) {
	long long int L1_hits = 0;
	long long int L1_stores = 0;
	long long int L1_accesses = 0;
	long long int L1_misses = 0;
	long long int L1_bank_conflicts = 0;
	long long int L1_nearby_hits = 0;
	long long int L1_same_word_conflicts = 0;
	long long int L1_bus_transfers = 0;
	long long int L1_bus_hits = 0;
	for (size_t i = 0; i < num_cores * num_L2s; ++i) {
	  printf("<=== Core %d ===>\n", (int)i);
	  cores[i]->issuer->print();
	}
	for (size_t i = 0; i < num_cores * num_L2s; ++i) {
	  printf("\n ## Core %d ##\n", (int)i);
	  NormalizeUtilization(cores[i]->cycle_num, cores[i]->utilizations);
	  PrintUtilization(cores[i]->module_names, cores[i]->utilizations);
	  //cores[i]->L1->PrintStats();
	  L1_hits += cores[i]->L1->hits;
	  L1_stores += cores[i]->L1->stores;
	  L1_accesses += cores[i]->L1->accesses;
	  L1_misses += cores[i]->L1->misses;
	  L1_nearby_hits += cores[i]->L1->nearby_hits;
	  L1_bank_conflicts += cores[i]->L1->bank_conflicts;
	  L1_same_word_conflicts += cores[i]->L1->same_word_conflicts;
	  L1_bus_transfers += cores[i]->L1->bus_transfers;
	  L1_bus_hits += cores[i]->L1->bus_hits;
	}

	// get highest cycle count
	long long int cycle_count = 0;
	for (size_t i = 0; i < num_cores * num_L2s; ++i) {
	  if (cores[i]->cycle_num > cycle_count) 
	    cycle_count = cores[i]->cycle_num;
	}
	
	printf("\n");
	printf("L1 accesses: \t%lld\n", L1_accesses);
	printf("L1 hits: \t%lld\n", L1_hits);
	printf("L1 misses: \t%lld\n", L1_misses);
	printf("L1 bank conflicts: \t%lld\n", L1_bank_conflicts);
	printf("L1 same word conflicts: \t%lld\n", L1_same_word_conflicts);
	printf("L1 stores: \t%lld\n", L1_stores);
	printf("L1 near hit: \t%lld\n", L1_nearby_hits);  
	printf("L1 hit rate: \t%f\n", static_cast<float>(L1_hits)/L1_accesses);
	printf("L2 -> L1 bus transfers: %lld\n", L1_bus_transfers);
	printf("L2 -> L1 bus hits: %lld\n", L1_bus_hits);
	printf("\n");

	// Print L2 stats and gather agregate data
	long long int L2_accesses = 0;
	long long int L2_misses = 0;
	for (size_t i = 0; i < num_L2s; ++i) {
	  printf(" -= L2 #%d =-\n", (int)i);
	  L2s[i]->PrintStats();
	  L2_accesses += L2s[i]->accesses;
	  L2_misses += L2s[i]->misses;
	  printf("\n");
	}

	// for linesize just use the first L2
	L2Cache* L2 = L2s[0];
	
	// print bandwidth numbers
	int word_size = 4;
	int L1_line_size = (int)pow( 2.f, static_cast<float>(cores[0]->L1->line_size) );
	int L2_line_size = (int)pow( 2.f, static_cast<float>(L2->line_size) );
	float Hz = 1000000000;
	printf("Bandwidth numbers for %dMHz clock (GB/s):\n", static_cast<int>(Hz/1000000));
	printf("   register to L1 bandwidth: \t %f\n", static_cast<float>(L1_accesses) * word_size * Hz / cycle_count);
	printf("   L1 to L2 bandwidth: \t\t %f\n", static_cast<float>(L2_accesses) * word_size * L1_line_size * Hz / cycle_count);
	
	float DRAM_BW;
	if(disable_usimm)
	  {
	    DRAM_BW = static_cast<float>(L2_misses) * word_size * L2_line_size * Hz / cycle_count;
	  }
	else
	  {
	    long long int total_lines_transfered = 0;
	    for(int c=0; c < NUM_CHANNELS; c++)
	      total_lines_transfered += stats_reads_completed[c];
	    DRAM_BW = static_cast<float>(total_lines_transfered) * L2_line_size * word_size * Hz / cycle_count;
	  }
	printf("   L2 to memory bandwidth: \t %f\n", DRAM_BW);


	// TODO: debug help (print out the atominc counters =========================================================================================================
	printf("Final Global Register values: imgSize=%d\n", image_width*image_height);
	globals.print();

	// print sizes
	double size_estimate = L2_size * num_L2s + core_size * num_cores * num_L2s;
	printf("Core size: %.4lf\n", core_size);
	printf("L2 size: %.4lf\n", L2_size);
	printf("%d-L2 size: %.4lf\n", num_L2s, L2_size * num_L2s);
	printf("%d-core chip size: %.4lf\n", num_cores * num_L2s, size_estimate);
	
	// needs fixing
	//int num_tiles = image_width * image_height / 256;
	printf("FPS Statistics:\n");
	printf("Total clock cycles: %lld\n", cycle_count);
	printf("  FPS assuming %dMHz clock: %.4lf\n", (int)Hz / 1000000,
	       Hz/static_cast<double>(cycle_count));


	printf("\n\n");

	
	// DK: Not sure where to print these stats, just put it at the very end for testing for now.
	if(!disable_usimm)
	  printUsimmStats();


	/*
	double fps1024 = Hz/((cycle_count) /
				    static_cast<double>(image_width * image_height) * (1024*768));
	printf("  FPS assuming 1024x768: %.4lf\n",
	       fps1024);
	// for 20fps at 1024x768
	printf("  Number of cores for 20fps at 1024x768: %.4lf (%.4lf Threads)\n",
	       20/fps1024*num_cores, 20/fps1024 * num_cores * num_thread_procs);
	printf("  Number of cores for 30fps at 1024x768: %.4lf (%.4lf Threads)\n",
	       30/fps1024*num_cores, 30/fps1024 * num_thread_procs);
	printf("  Size for %d cores:\t%.4lf mm^2\n", static_cast<int>(ceil(20/fps1024*num_cores)),
	       ceil(20/fps1024*num_cores)*core_size + L2_size * num_L2s);
	//jbs: these numbers might need num_L2s in here
	printf("  FPS for %d cores:\t%.4lf\n", static_cast<int>(ceil(20/fps1024*num_cores)),
	       ceil(20/fps1024*num_cores)*fps1024/num_cores);
	printf("  Size for %d cores:\t%.4lf mm^2\n", static_cast<int>(ceil(30/fps1024*num_cores)),
	       ceil(30/fps1024*num_cores)*core_size + L2_size * num_L2s);
	printf("  FPS for %d cores:\t%.4lf\n", static_cast<int>(ceil(30/fps1024*num_cores)),
	       ceil(30/fps1024*num_cores)*fps1024/num_cores);
	*/
      }

      fflush(stdout);

      if (print_png) {
	char command_buf[512];
	if(animation != NULL)
	  sprintf(command_buf, "convert PPM:- %s.%d.%s", output_prefix, animation->current_frame, image_type);
	else
	  sprintf(command_buf, "convert PPM:- %s.%s", output_prefix, image_type);
#ifndef WIN32
	FILE* output = popen(command_buf, "w");
#else
	FILE* output = popen(command_buf, "wb");
#endif
	if (!output)
	  perror("Failed to open out.png");
	
#if OBJECTID_MAP
	srand( (unsigned)time( NULL ) );
	const int num_ids = 100000;
	float id_colors[num_ids][3];
	for (int i = 0; i < num_ids; i++) {
	  id_colors[i][0] = drand48();
	  id_colors[i][1] = drand48();
	  id_colors[i][2] = drand48();
	}
#endif

	fprintf(output, "P6\n%d %d\n%d\n", image_width, image_height, 255);
	for (int j = image_height - 1; j >= 0; j--) {
	  for (int i = 0; i < image_width; i++) {
	    int index = start_framebuffer + 3 * (j * image_width + i);
	    
	    float rgb[3];
#if OBJECTID_MAP
	    int object_id = memory->getData()[index].ivalue;
	    
	    switch (object_id) {
	    case -1:
	      rgb[0] = .2;
	      rgb[1] = .1;
	      rgb[2] = .5;
	      break;
	    case 0:
	      rgb[0] = 1.;
	      rgb[1] = .4;
	      rgb[2] = 1.;
	      break;
	    case 1:
	      rgb[0] = .2;
	      rgb[1] = .3;
	      rgb[2] = 1.;
	      break;
	    case 2:
	      rgb[0] = 1.;
	      rgb[1] = .3;
	      rgb[2] = .2;
	      break;
	    default:
	      rgb[0] = id_colors[object_id % num_ids][0];
	      rgb[1] = id_colors[object_id % num_ids][1];
	      rgb[2] = id_colors[object_id % num_ids][2];
	      break;
	    };
#else
	    // for gradient/colors we have the result
	    rgb[0] = memory->getData()[index + 0].fvalue;
	    rgb[1] = memory->getData()[index + 1].fvalue;
	    rgb[2] = memory->getData()[index + 2].fvalue;
#endif
	    fprintf(output, "%c%c%c",
		    (char)(int)(rgb[0] * 255),
		    (char)(int)(rgb[1] * 255),
		    (char)(int)(rgb[2] * 255));
	  }
	}
	pclose(output);
      }
      else
	exit(0);
      /*
      else {
	for (int j = 0; j < image_height; j++) {
	  printf("Row %03d: ------\n", j);
	  for (int i = 0; i < image_width; i++) {
	    int index = start_framebuffer + 3 * (j * image_width + i);
	    //printf("%.2f ", mem[index].fvalue);
	    printf("%d ", memory->getData()[index].ivalue);
	  }
	  printf("\n");
	}
      }
      */

      // reset the cores for a fresh frame (enforce "cache coherency" modified scene data)
      // only one section (one L2) of the chip does any scene modification, so this is fine
      // We keep the writes local and coherent within that L2 for the duration of a frame
      globals.Reset();
      for (size_t i = 0; i < num_cores * num_L2s; ++i) {
	cores[i]->Reset();
      }
      for(size_t i = 0; i < num_L2s; i++)
	L2s[i]->Reset();
      if(animation == NULL)
	break;
      if(!animation->loadNextFrame())
	break;
      frames_since_rebuild++;
      if(bvh!=NULL && frames_since_rebuild == rebuild_frequency)
	{
	  bvh->rebuild(memory->getData());
	  frames_since_rebuild = 0;
	}
  }
  
  // clean up thread stuff
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&atominc_mutex);
  pthread_mutex_destroy(&memory_mutex);
  pthread_mutex_destroy(&sync_mutex);
  pthread_mutex_destroy(&global_mutex);
  for(int i=0; i < MAX_NUM_CHANNELS; i++)
    {
      pthread_mutex_destroy(&(usimm_mutex[i]));
    }
  pthread_cond_destroy(&sync_cond);
  
  
  for(size_t i=0; i<cores.size(); i++){
    delete cores[i];
  }
  delete[] L2s;
  printf("\t <== Total time: %12.1f s ==>\n", clockdiff(start_time, clock()));

#if TRACK_LINE_STATS
  L1Cache* L10 = cores[0]->L1;
  int numLines = L10->cache_size>>L10->line_size;
  
  printf("Average reads per resident line\tAverage cycles resident\tTotal Accesses\tTotal Hits\n");
  
  for(int i=0; i < numLines; i++)
    printf("%f\t%f\t%lld\t%lld\n", (float)(L10->total_reads[i]) / (float)(L10->total_validates[i]), (float)(L10->current_cycle) / (float)(L10->total_validates[i]), L10->line_accesses[i], L10->total_reads[i]);
#endif


  return 0;
}
