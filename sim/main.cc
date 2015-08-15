#include "Bitwise.h"
#include "BranchUnit.h"
#include "BVH.h"
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
#include "Profiler.h"
#include "Debugger.h"
#include "DwarfReader.h"
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
#include "lodepng.h"

#include "WinCommonNixFcns.h"
// Windows pipe stuff (needs stdio.h)
#ifdef WIN32
# ifndef popen
#   define popen _popen
#   define pclose _pclose
# endif
# ifndef strcasecmp
#   define strcasecmp _stricmp
# endif
#endif

#include <float.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <math.h>
#include <boost/chrono.hpp>

// relative paths between install (bin) folder for simtrax and scene folder
// CMAKE will define this parameter during generation, but if we use Makefiles,
// then this should be just ../
#ifndef REL_PATH_BIN_TO_SAMPLES
#  define REL_PATH_BIN_TO_SAMPLES "../"
#endif


pthread_mutex_t atominc_mutex;
pthread_mutex_t memory_mutex;
pthread_mutex_t sync_mutex;
pthread_mutex_t global_mutex;
pthread_mutex_t profile_mutex;
pthread_mutex_t usimm_mutex[MAX_NUM_CHANNELS];
pthread_cond_t sync_cond;
// for synchronization
int global_total_simulation_threads;
int current_simulation_threads;
bool disable_usimm;
bool wait_usimm;

// this branch delay is reset by code that finds the delay from the
// config file
int BRANCH_DELAY = 0;
L2Cache** L2s;
unsigned int num_L2s;

// global verbosity flag
int trax_verbosity;

// Assembler needs somewhere to put file names for debug info if compiled with -g
// Global because many units may want use of this for better error reporting
std::vector<std::string> source_names;
std::vector< std::vector< std::string > > source_lines;

void PrintProfile(const char* assem_file, std::vector<Instruction*>& instructions, 
		  std::vector<std::string> srcNames, FILE* profile_output, 
		  Profiler profiler, long long int total_thread_cycles);

void PrintElapsedTime(const char *heading, const boost::chrono::system_clock::time_point time_start) {
  boost::chrono::system_clock::time_point time_end = boost::chrono::system_clock::now();
  boost::chrono::system_clock::duration timeElapsed  = time_end - time_start;
  boost::chrono::system_clock::duration timeElapsed2 = timeElapsed;

  const boost::chrono::hours        hours        = boost::chrono::duration_cast<boost::chrono::hours>(timeElapsed);       timeElapsed -= hours;
  const boost::chrono::minutes      minutes      = boost::chrono::duration_cast<boost::chrono::minutes>(timeElapsed);     timeElapsed -= minutes;
  const boost::chrono::seconds      seconds      = boost::chrono::duration_cast<boost::chrono::seconds>(timeElapsed);     timeElapsed -= seconds;
  const boost::chrono::milliseconds milliSeconds = boost::chrono::duration_cast<boost::chrono::milliseconds>(timeElapsed);

  printf("\t <== %s: %lu.%03lu seconds (%d:%d:%lu.%03lu) ==>\n", heading,
         boost::chrono::duration_cast<boost::chrono::seconds>(timeElapsed2).count(), milliSeconds.count(),
         hours.count(), minutes.count(), seconds.count(), milliSeconds.count());
}

void SystemClockRise(std::vector<HardwareModule*>& modules) {
  for(size_t i = 0; i < modules.size(); i++) {
    modules[i]->ClockRise();
  }
}

void SystemClockFall(std::vector<HardwareModule*>& modules) {
  for(size_t i = 0; i < modules.size(); i++) {
    modules[i]->ClockFall();
  }
}

void PrintSystemInfo(long long int& cycle_num,
                     std::vector<HardwareModule*>& modules,
                     std::vector<std::string>& names) {
  printf("Cycle %lld:\n", cycle_num++);
  for(size_t i = 0; i < modules.size(); i++) {
    printf("\t%s: ", names[i].c_str());
    modules[i]->print();
    printf("\n");
  }
  printf("\n");
}

void TrackUtilization(std::vector<HardwareModule*>& modules,
                      std::vector<double>& sums)  {
  for(size_t i = 0; i < modules.size(); i++) {
    sums[i] += modules[i]->Utilization();
  }
}

void NormalizeUtilization(long long int cycle_num, std::vector<double>& sums) {
  for(size_t i = 0; i < sums.size(); i++) {
    sums[i] /= cycle_num;
  }
}

void PrintUtilization(std::vector<std::string>& module_names,
                      std::vector<double>& utilization, int numCores = 1) {
  printf("Module Utilization\n\n");
  for(size_t i = 0; i < module_names.size(); i++) {
    if(utilization[i] > 0.)
      printf("\t%20s:  %6.2f\n", module_names[i].c_str(), 100.f * (utilization[i] / (float)numCores));
  }
  printf("\n");
}

// Argument struct for running a simulation thread
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
  if(current_simulation_threads > 0) {
    pthread_cond_wait(&sync_cond, &sync_mutex);
  }
  else {
    // Last thread sync caches
    for(size_t i = 0; i < num_L2s; i++) {
      L2s[i]->ClockRise();
      L2s[i]->ClockFall();
    }

    // Last thread updates the DRAM
    // Multiple DRAM cycles per trax cycle
    if(!disable_usimm) {
      for(int i=0; i < DRAM_CLOCK_MULTIPLIER; i++)
        usimmClock();
    }

    // Last thread signal the others to wake up
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
    std::vector<TraxCore*>::iterator tpIter = core_args->cores->begin() + core_args->start_core;
    for(int i = core_args->start_core; i < core_args->end_core; ++i, ++tpIter) {
//      long long int stall_cycles = (*core_args->cores)[i]->CountStalls();
      long long int stall_cycles = (*tpIter)->CountStalls();
      if(stall_cycles > max_stall_cycles) {
        start_core = i - core_args->start_core;
        max_stall_cycles = stall_cycles;
      }
    }

    const int num_cores = core_args->end_core - core_args->start_core;
    // start_core to num_cores
    tpIter = core_args->cores->begin() + (core_args->start_core + start_core);
    for(int i = 0; i < (num_cores - start_core); ++i, ++tpIter)
    {
      SystemClockRise((*tpIter)->modules);
      SystemClockFall((*tpIter)->modules);
    }
    // 0 to start_core
    tpIter = core_args->cores->begin() + core_args->start_core;
    for(int i = 0; i < start_core; ++i, ++tpIter)
    {
      SystemClockRise((*tpIter)->modules);
      SystemClockFall((*tpIter)->modules);
    }
//    for(int i = 0; i < num_cores; ++i, ++tpIter) {
//      int core_id = ((i + start_core) % num_cores) + core_args->start_core;
//      SystemClockRise((*core_args->cores)[core_id]->modules);
//      SystemClockFall((*core_args->cores)[core_id]->modules);
//    }

    SyncThread(core_args);

    bool all_done = true;
    tpIter = core_args->cores->begin() + core_args->start_core;
    for(int i = core_args->start_core; i < core_args->end_core; ++i, ++tpIter) {
      TraxCore *coreRef = *tpIter;
      TrackUtilization(coreRef->modules, coreRef->utilizations);
      coreRef->cycle_num++;
      if(!coreRef->issuer->halted) {
        all_done = false;
      }
    }
    
    if(wait_usimm && usimmIsBusy())
      all_done = false;
    
    if(core_args->cores->front()->cycle_num == stop_cycle || all_done) {
      break;
    }
//    bool all_done = true;
//    for(int i = core_args->start_core; i < core_args->end_core; ++i) {
//      TrackUtilization((*core_args->cores)[i]->modules, (*core_args->cores)[i]->utilizations);
//      (*core_args->cores)[i]->cycle_num++;
//      if(!(*core_args->cores)[i]->issuer->halted) {
//        all_done = false;
//      }
//    }
//    if((*core_args->cores)[0]->cycle_num == stop_cycle || all_done) {
//      break;
//    }
  }

  pthread_mutex_lock(&sync_mutex);
  global_total_simulation_threads--;
  current_simulation_threads--;
  if(current_simulation_threads == 0) {
    // if this was the last thread wake the others.
    current_simulation_threads = global_total_simulation_threads;
    pthread_cond_broadcast(&sync_cond);
  }
  pthread_mutex_unlock(&sync_mutex);
  return 0;
}


void SerialExecution(CoreThreadArgs* core_args, int num_cores) {
  printf("Cores 0 through %d running (serial execution mode)\n", num_cores - 1);

  while (true) {
    bool all_halted = true;
    //TODO: put the core pointers into a vector (or some kind of collection),
    // and remove them when they halt, to cut down this loop... (or just swap halted ones to the end of the array?
    std::vector<TraxCore*>* cores = core_args[0].cores;
    long long int stop_cycle = core_args[0].stop_cycle;
    for(int i=0; i < num_cores; i++) {
      TraxCore* core = (*cores)[i];
      if(core->issuer->halted)
        continue;

      all_halted = false;
      SystemClockRise(core->modules);
      SystemClockFall(core->modules);
      TrackUtilization(core->modules, core->utilizations);
      core->cycle_num++;
      if(core->cycle_num == stop_cycle)
        core->issuer->halted = true;
    }
    if(all_halted)
      break;
  }
}


// TODO: use popt.h instead of reinventing the wheel
void printUsage(char* program_name) {
  printf("%s\n", program_name);
  printf(" + Simulator Parameters:\n");
  printf("    --atominc-report       <(debug): number of cycles between reporting global registers -- default 0, 0 means off>\n");
  printf("    --debug                <(debug): run TRaX progrem in the simtrax debugger>\n");
  printf("    --ignore-dcache-area   <reported chip area will not include data caches>\n");
  printf("    --issue-verbosity      <level of verbosity for issue unit -- default 0>\n");
  printf("    --load-mem-file        [read memory dump from file]\n");
  printf("    --mem-file             <memory dump file name -- default memory.mem>\n");
  printf("    --print-instructions   [print contents of instruction memory]\n");
  printf("    --print-symbols        [print symbol table generated by assembler]\n");
  printf("    --profile              [print per-instruction execution info to \"profile.out\"]\n");
  printf("    --serial-execution     [use a single pthread to run simulation]\n");
  printf("    --simulation-threads   <number of simulator pthreads. -- default 1>\n");
  printf("    --stop-cycle           <stop the simulation on reaching this cycle number>\n");
  printf("    --verbose              enables output verbosity\n");
  printf("    --write-dot            <depth> generates a dot file for the BVH (bvh.dot). Depth should not exceed 8\n");
  printf("    --write-mem-file       [write memory dump to file]\n");

  printf("\n");
  printf("  + TRAX Specification:\n");
  printf("    --num-regs           <number of registers -- default 128>\n");
  printf("    --num-globals        <number of global registers -- default 8>\n");
  printf("    --num-thread-procs   <number of threads per TM -- default 4>\n");
  printf("    --num-TMs            <number of TMs (Thread Multiprocessors) -- default 1>\n");
  printf("    --num-l2s            <number of L2 blocks. All resources are multiplied by this number. -- default 1>\n");
  printf("    --l1-off             [turn off the L1 data cache and set latency to 0]\n");
  printf("    --l2-off             [turn off the L2 data cache and set latency to 0]\n");
  printf("    --num-icache-banks   <number of banks per icache -- default 1>\n");
  printf("    --num-icaches        <number of icaches in a TM. Should be a power of 2 -- default 1>\n");
  printf("    --disable-usimm      [use naive DRAM simulation instead of usimm]\n");
  printf("    --wait-usimm         [wait for all DRAM commands to finish before halting machine]\n");

  printf("\n");
  printf("  + Files:\n");
  printf("    --config-file     <config file name>\n");
  printf("    --dcacheparams    <data cache params file name>\n");
  printf("    --icacheparams    <instruction cache params file name>\n");
  printf("    --light-file      <light file name>\n");
  printf("    --load-assembly   <TRaX assembly file to execute>\n");
  printf("    --model           <model file name (.obj)>\n");
  printf("    --output-prefix   <prefix for image output. Be sure any directories exist>\n");
  printf("    --usimm-config    <usimm config file name>\n");
  printf("    --vi-file         <usimm chip config file name>\n");
  printf("    --view-file       <view file name>\n");

  printf("\n");
  printf("  + Scene Parameters:\n");
  printf("    --background      <r g b, background color -- default 0.561 0.729 0.988>\n");
  printf("    --epsilon         <small number pre-loaded to main memory, useful for various ray tracer offsets, default 1e-4>\n");
  printf("    --height          <framebuffer height in pixels -- default 128>\n");
  printf("    --no-png          [disable png output]\n");
  printf("    --no-scene        <specify there is no model, camera, or light. use for non-ray tracing programs>\n");
  printf("    --num-samples     <number of samples per pixel, pre-loaded to main memory -- default 1>\n");
  printf("    --ray-depth       <depth of rays, pre-loaded to main memory -- default 1>\n");
  printf("    --use-png-ext     [use png for file output -- default no (ppm instead)\\n");
  printf("    --width           <framebuffer width in pixels -- default 128>\n");


  printf("\n");
  printf("  + Other:\n");
  printf("    --pack-split-axis         [BVH nodes will pack split axis in the 2nd byte and num_children in the 1st byte (lsB) of 6th word]\n");
  printf("    --store-parent-pointers   [BVH parent pointers will be stored in a separate array starting at TRAX_START_PARENT_POINTERS]\n");
  printf("    --triangles-store-edges   [set flag to store 2 edge vecs in a tri instead of 2 verts -- default: off]\n");
}


int main(int argc, char* argv[]) {

  boost::chrono::system_clock::time_point time_start = boost::chrono::system_clock::now();
  bool print_system_info                = false;
  bool run_profile                      = false;
  bool run_debugger                     = false;
  bool needs_debug_symbols              = false;
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
  bool use_png_ext_for_output           = false;
  float far                             = 1000;
  int dot_depth                         = 0;
  simtrax::Camera* camera               = NULL;
  float *light_pos                      = NULL;
  int tile_width                        = 16;
  int tile_height                       = 16;
  int ray_depth                         = 1;
  int num_samples                       = 1;
  float background_color[3];
  float epsilon                         = 1e-4f;
  bool print_instructions               = false;
  bool no_scene                         = false;
  int issue_verbosity                   = 0;
  bool ignore_dcache_area               = false;
  long long int atominc_report_period   = 0;
  int num_icaches                       = 1;
  int icache_banks                      = 1;
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
  bool pack_split_axis                  = false;
  bool pack_stream_boundaries           = false;
  bool store_parent_pointers            = false;
  disable_usimm                         = false; // globally defined for use above
  wait_usimm                            = false;
  BVH* bvh;
  //Animation *animation                  = NULL;
  ThreadProcessor::SchedulingScheme scheduling_scheme = ThreadProcessor::SIMPLE;
  int total_simulation_threads          = 1;
  char *usimm_config_file               = NULL;
  char *usimm_vi_file                   = NULL;
  char *dcache_params_file              = NULL;
  char *icache_params_file              = NULL;
  Profiler profiler;
  Debugger debugger;
  DwarfReader dwarfReader;

  background_color[0] = 0.561f;
  background_color[1] = 0.729f;
  background_color[2] = 0.988f;

  // Verify Instruction.h matches Instruction.cc in size.
  if (strncmp(Instruction::Opnames[Instruction::xor_m].c_str(), "xor_m", 5) != 0) 
    {
      printf("Instruction.h does not match Instruction.cc. Please fix them.\n");
      exit(-1);
    }

  // If no arguments passed to simulator, print help and exit
  if (argc == 1) 
    {
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
    } else if (strcmp(argv[i], "--num-cores") == 0 || 
               strcmp(argv[i], "--num-TMs") == 0) {
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
    } else if (strcmp(argv[i], "--use-png-ext") == 0) {
      use_png_ext_for_output = true;
    } else if (strcmp(argv[i], "--tile-width") == 0) {
      tile_width = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--tile-height") == 0) {
      tile_height = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--ray-depth") == 0) {
      ray_depth = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--background") == 0) {
      background_color[0] = atof(argv[++i]);
      background_color[1] = atof(argv[++i]);
      background_color[2] = atof(argv[++i]);
    } else if (strcmp(argv[i], "--epsilon") == 0) {
      epsilon = static_cast<float>( atof(argv[++i]) );
    } else if (strcmp(argv[i], "--num-samples") == 0) {
      num_samples = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--verbose") == 0) {
      trax_verbosity = 1;
    } else if (strcmp(argv[i], "--write-dot") == 0) {
      dot_depth = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--print-instructions") == 0) {
      print_instructions = true;
    } else if (strcmp(argv[i], "--no-scene") == 0) {
      no_scene = true;
    } else if (strcmp(argv[i], "--issue-verbosity") == 0) {
      issue_verbosity = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--ignore-dcache-area") == 0) {
      ignore_dcache_area = true;
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
    } else if (strcmp(argv[i], "--profile") == 0) {
      run_profile = true;
      needs_debug_symbols = true;
    } else if (strcmp(argv[i], "--debug") == 0) {
      run_debugger = true;
      needs_debug_symbols = true;
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
    } else if (strcmp(argv[i], "--store-parent-pointers") == 0) {
      store_parent_pointers = true;
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
    } else if (strcmp(argv[i], "--vi-file") == 0) {
      usimm_vi_file = argv[++i];
    } else if (strcmp(argv[i], "--dcacheparams") == 0) {
      dcache_params_file = argv[++i];
    } else if (strcmp(argv[i], "--icacheparams") == 0) {
      icache_params_file = argv[++i];
    } else if (strcmp(argv[i], "--disable-usimm") == 0) {
      disable_usimm = 1;
    } else if (strcmp(argv[i], "--wait-usimm") == 0) {
      wait_usimm = true;
    }
    else {
      printf(" Unrecognized option %s\n", argv[i]);
      printUsage(argv[0]);
      return -1;
    }
    if(argnum!=i)
      printf(" %s", argv[i]);
    printf("\n");
  }


  if(rebuild_frequency > 0)
    duplicate_bvh = false;
  
  // size estimates
  double core_size = 0;
  // deprecated
  double L2_size = 0;


  // Keep track of register names
  std::vector<symbol*> regs;

  // Assembler needs somewhere to store the jump table and string literals, to be loaded in to local stores after assembly
  char* jump_table;
  std::vector<std::string> ascii_literals;

  // Instruction Memory
  std::vector<Instruction*> instructions;
  std::vector<TraxCore*> cores;
  L2s = new L2Cache*[num_L2s];
  MainMemory* memory;
  GlobalRegisterFile globals(num_globals, num_thread_procs * threads_per_proc * num_cores * num_L2s, atominc_report_period);

  if (config_file == NULL) {
    config_file = (char*)REL_PATH_BIN_TO_SAMPLES"samples/configs/default.config";
    printf("No configuration specified, using default: %s\n", config_file);
  }
  if(dcache_params_file == NULL)
    dcache_params_file = (char*)REL_PATH_BIN_TO_SAMPLES"samples/configs/dcacheparams.txt";
  if(icache_params_file == NULL)
    icache_params_file = (char*)REL_PATH_BIN_TO_SAMPLES"samples/configs/icacheparams.txt";


  //if (config_file != NULL) {
  // Set up memory from config (L2 and main memory)
  ReadConfig config_reader(config_file, dcache_params_file, L2s, num_L2s, memory, L2_size, disable_usimm, memory_trace, l1_off, l2_off, l1_read_copy);

  // loop through the L2s
  for(size_t l2_id = 0; l2_id < num_L2s; ++l2_id) {
    L2Cache* L2 = L2s[l2_id];
    // load the cores
    for(size_t i = 0; i < num_cores; ++i) {
      size_t core_id = i + num_cores * l2_id;
      if(trax_verbosity)
        printf("Loading core %d.\n", (int)core_id);

      // only one computation is needed... we'll end up with the last one after the loop
      core_size = 0;
      TraxCore *current_core = new TraxCore(num_thread_procs, threads_per_proc, num_regs, 
					    scheduling_scheme, &instructions, L2, core_id, 
					    l2_id, run_profile, &profiler, &debugger);
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

      // Link the FPAdder to the FPMul for fmad ops
      // Similarly for integers
      FPMul* fpmul = NULL;
      FPAddSub* fpadd = NULL;
      IntMul* intmul = NULL;
      IntAddSub* intadd = NULL;
      for(size_t unit_id = 0; unit_id < current_core->functional_units.size(); unit_id++)
	{
	  if(fpmul == NULL)
	    fpmul = dynamic_cast<FPMul*>(current_core->functional_units[unit_id]);
	  if(fpadd == NULL)
	    fpadd = dynamic_cast<FPAddSub*>(current_core->functional_units[unit_id]);
	  if(intmul == NULL)
	    intmul = dynamic_cast<IntMul*>(current_core->functional_units[unit_id]);
	  if(intadd == NULL)
	    intadd = dynamic_cast<IntAddSub*>(current_core->functional_units[unit_id]);
	}
      if(fpmul != NULL && fpadd != NULL)
	fpmul->SetAdder(fpadd);
      if(intmul != NULL && intadd != NULL)
	intmul->SetAdder(intadd);

      cores.push_back(current_core);
    }
  }

  // set up L1 snooping if enabled
  if(cache_snoop) {
    for(size_t i = 0; i*4 < num_cores * num_L2s; ++i) {
      L1Cache* L1_0=NULL, *L1_1=NULL, *L1_2=NULL, *L1_3=NULL;
      L1_0 = cores[i*4]->L1;
      if(num_cores > i*4+1) {
        L1_1 = cores[i*4+1]->L1;
        L1_1->L1_1 = L1_0;
        L1_0->L1_1 = L1_1;
      }
      if(num_cores > i*4+2) {
        L1_2 = cores[i*4+2]->L1;
        L1_2->L1_1 = L1_0;
        L1_2->L1_2 = L1_1;
        L1_1->L1_2 = L1_2;
        L1_0->L1_2 = L1_2;
      }
      if(num_cores > i*4+3) {
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
  // only for backwards compatability with old trax compilers
  std::vector<FunctionalUnit*> functional_units = cores[0]->functional_units;
  for(size_t i = 0; i < functional_units.size(); i++) {
    BranchUnit* brancher = dynamic_cast<BranchUnit*>(functional_units[i]);
    if(brancher) {
      int latency = brancher->GetLatency();
      if(latency > BRANCH_DELAY)
        BRANCH_DELAY = latency;
    }
  }

  int start_wq, start_framebuffer, start_scene, start_camera, start_bg_color, start_light, end_memory;
  int start_matls, start_permutation;

  if(load_mem_file && mem_file != NULL) {
    // load memory from file
    printf("Loading Memory from file '%s'.\n", mem_file);
    memory->LoadMemory( mem_file, start_wq, start_framebuffer, start_scene, 
            start_matls, start_camera, start_bg_color, start_light, end_memory,
            light_pos, start_permutation );
    printf("Memory Loaded.\n");

    start_framebuffer = memory->data[7].ivalue;
  }
  else { // (need to skip a lot of these if memory loading really works)

    if(custom_mem_loader) {
      // run custom loader
      end_memory = 0;
      CustomLoadMemory(memory->getData(), memory->getSize(), image_width, image_height,
               epsilon, custom_mem_loader);
    } else {
      // default memory loader 
      if(view_file != NULL) {
        camera = ReadViewfile::LoadFile(view_file, far);
      }
      else if(!no_scene){
        printf("ERROR: No camera file supplied.\n");
        return -1;
      }

      // just set the model file equal to the keyframe file and load it the same way
      // first frame will be loaded exactly the same way, memory loader doesn't need to change.
      // subsequent frames will be updated by the Animation, directly in simulator's memory
      //if(keyframe_file != NULL)
      //model_file = keyframe_file;

      if(!no_scene && model_file == NULL) {
        printf("ERROR: No model data supplied.\n");
        return -1;
      }

      if(light_file != NULL) {
        light_pos = new float[3];
        ReadLightfile::LoadFile(light_file, light_pos);
      }
      else {
        light_pos = new float[3];
        //sponza light as default
        light_pos[0] = 7.97f;
        light_pos[1] = 1.4f;
        light_pos[2] = -1.74f;
      }

      LoadMemoryParams paramsForLoadMemory;
      paramsForLoadMemory.mem                       = memory->getData();
      paramsForLoadMemory.mem_size                  = memory->getSize();
      paramsForLoadMemory.image_width               = image_width;
      paramsForLoadMemory.image_height              = image_height;
      paramsForLoadMemory.grid_dimensions           = grid_dimensions;
      paramsForLoadMemory.camera                    = camera;
      paramsForLoadMemory.model_file                = model_file;
      paramsForLoadMemory.background_color[0]       = background_color[0];
      paramsForLoadMemory.background_color[1]       = background_color[1];
      paramsForLoadMemory.background_color[2]       = background_color[2];
      paramsForLoadMemory.light_pos[0]              = light_pos[0];
      paramsForLoadMemory.light_pos[1]              = light_pos[1];
      paramsForLoadMemory.light_pos[2]              = light_pos[2];
      paramsForLoadMemory.tile_width                = tile_width;
      paramsForLoadMemory.tile_height               = tile_height;
      paramsForLoadMemory.ray_depth                 = ray_depth;
      paramsForLoadMemory.num_samples               = num_samples;
      paramsForLoadMemory.num_rotation_threads      = num_thread_procs * num_cores;
      paramsForLoadMemory.num_TMs                   = num_cores;
      paramsForLoadMemory.subtree_size              = subtree_size;
      paramsForLoadMemory.epsilon                   = epsilon;
      paramsForLoadMemory.duplicate_bvh             = duplicate_bvh;
      paramsForLoadMemory.triangles_store_edges     = triangles_store_edges;
      paramsForLoadMemory.pack_split_axis           = pack_split_axis;
      paramsForLoadMemory.pack_stream_boundaries    = pack_stream_boundaries;
      paramsForLoadMemory.store_parent_pointers     = store_parent_pointers;

      LoadMemory(paramsForLoadMemory);

      // returned values
      bvh               = paramsForLoadMemory.bvh;
      start_wq          = paramsForLoadMemory.start_wq;
      start_framebuffer = paramsForLoadMemory.start_framebuffer;
      start_scene       = paramsForLoadMemory.start_scene;
      start_matls       = paramsForLoadMemory.start_matls;
      start_camera      = paramsForLoadMemory.start_camera;
      start_bg_color    = paramsForLoadMemory.start_bg_color;
      start_light       = paramsForLoadMemory.start_light;
      start_permutation = paramsForLoadMemory.start_permutation;
      end_memory        = paramsForLoadMemory.end_memory;
    }
  } // end else for memory dump file


  // Set up incremental output if option is specified
  if(incremental_output) {
    memory->image_width = image_width;
    memory->image_height = image_height;
    memory->incremental_output = incremental_output;
    memory->start_framebuffer = start_framebuffer;
    memory->stores_between_output = stores_between_output;
  }

  int memory_size = memory->getSize();

  if(end_memory > memory_size) {
    printf("ERROR: Scene requires %d blocks while memory_size is only %d\n", end_memory, memory_size);
    exit(-1);
  }



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

  int jtable_size = 0;
  if(assem_file != NULL) {
    jtable_size = Assembler::LoadAssem(assem_file, instructions, regs, num_regs, jump_table, ascii_literals, source_names, source_lines, print_symbols, needs_debug_symbols, &dwarfReader);
    if(jtable_size < 0) {
      printf("assembler returned an error, exiting\n");
      exit(-1);
    }
    if(trax_verbosity)
      printf("data segment: %d bytes\n", jtable_size);
  }
  else {
    printf("Error: no assembly program specified\n");
    return -1;
  }

  if(run_profile)
    profiler.setDwarfReader(&dwarfReader);
  
  int last_instruction = static_cast<int>(instructions.size());
  if(trax_verbosity)
    printf("Number of instructions: %d\n\n", last_instruction);

  // Write the bvh to a dot file if desired
  if(dot_depth > 0)
    bvh->writeDOT("bvh.dot", start_scene, memory->getData(), 1, dot_depth);

  if(print_instructions) {
    printf("Instruction listing:\n");
    for (int i = 0; i < last_instruction; ++i) {
      instructions[i]->print();
      printf("\n");
    }
  }

  // initialize the cores
  for (size_t i = 0; i < cores.size(); ++i) {
    cores[i]->initialize(icache_params_file, issue_verbosity, num_icaches, icache_banks, simd_width, jump_table, jtable_size, ascii_literals);
  }

  // Check that there are units for each instruction in the program
  for (int i = 0; i < last_instruction; ++i) {
    // Exceptions for register file and misc
    if(instructions[i]->op == Instruction::HALT ||
       instructions[i]->op == Instruction::BARRIER ||
       instructions[i]->op == Instruction::MOV ||
       instructions[i]->op == Instruction::LOADIMM ||
       instructions[i]->op == Instruction::lui ||
       instructions[i]->op == Instruction::MOVINDRD ||
       instructions[i]->op == Instruction::MOVINDWR ||
       instructions[i]->op == Instruction::NOP ||
       instructions[i]->op == Instruction::nop ||
       instructions[i]->op == Instruction::mfc1 ||
       instructions[i]->op == Instruction::mfhi ||
       instructions[i]->op == Instruction::mflo ||
       instructions[i]->op == Instruction::movf ||
       instructions[i]->op == Instruction::move ||
       instructions[i]->op == Instruction::movn ||
       instructions[i]->op == Instruction::movt ||
       instructions[i]->op == Instruction::movt_s ||
       instructions[i]->op == Instruction::movf_s ||
       instructions[i]->op == Instruction::mov_s ||
       instructions[i]->op == Instruction::movn_s ||
       instructions[i]->op == Instruction::movz_s ||
       instructions[i]->op == Instruction::movz ||
       instructions[i]->op == Instruction::mtc1 ||
       instructions[i]->op == Instruction::fill_w ||
       instructions[i]->op == Instruction::splati_w ||
       instructions[i]->op == Instruction::ldi_b ||
       instructions[i]->op == Instruction::ldi_w ||
       instructions[i]->op == Instruction::insve_w ||
       instructions[i]->op == Instruction::move_v ||
       instructions[i]->op == Instruction::insert_w ||
       instructions[i]->op == Instruction::copy_s_w ||
       instructions[i]->op == Instruction::PROF ||
       instructions[i]->op == Instruction::SETBOXPIPE ||
       instructions[i]->op == Instruction::SETTRIPIPE ||
       instructions[i]->op == Instruction::SLEEP) {
      continue;
    }
    bool op_found = false;
    for(size_t j = 0; j < cores[0]->issuer->units.size(); j++) {
      if (cores[0]->issuer->units[j]->SupportsOp(instructions[i]->op)) {
        op_found = true;
      }
    }
    if(!op_found) {
      printf("Instruction not supported in current config. Try a different config or add the following instruction to the exception list in main_new.cc:\n");
      instructions[i]->print();
      printf("\n");
      exit(-1);
    }
  }

  if(!disable_usimm) {
    if(usimm_config_file == NULL) {
      //usimm_config_file = (char*)REL_PATH_BIN_TO_SAMPLES"samples/configs/usimm_configs/1channel.cfg";
      //usimm_config_file = (char*)REL_PATH_BIN_TO_SAMPLES"samples/configs/usimm_configs/4channel.cfg";
      //usimm_config_file = (char*)REL_PATH_BIN_TO_SAMPLES"samples/configs/usimm_configs/gddr5.cfg";
      usimm_config_file = (char*)REL_PATH_BIN_TO_SAMPLES"samples/configs/usimm_configs/gddr5_8ch.cfg";
      printf("No USIMM configuration specified, using default: %s\n", usimm_config_file);
    }
    if(usimm_setup(usimm_config_file, usimm_vi_file) < 0)
      {
	printf("unable to initialize usimm\n");
	exit(1);
      }
  }

  // Limit simulation threads to the number of TMs
  if(total_simulation_threads > (int)(num_cores * num_L2s)) 
    total_simulation_threads = num_cores * num_L2s;

  // Need at least 1
  if(total_simulation_threads < 1) 
    total_simulation_threads = 1;

  global_total_simulation_threads = total_simulation_threads;

  // set up simulator thread arguments 
  pthread_attr_t attr;
  pthread_t *threadids = new pthread_t[total_simulation_threads];
  CoreThreadArgs *args = new CoreThreadArgs[total_simulation_threads];
  pthread_mutex_init(&atominc_mutex, NULL);
  pthread_mutex_init(&memory_mutex, NULL);
  pthread_mutex_init(&sync_mutex, NULL);
  pthread_mutex_init(&global_mutex, NULL);
  pthread_mutex_init(&profile_mutex, NULL);
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
  for(int i = 0; i < total_simulation_threads; ++i) {
    if(remainder_threads == i) {
      cores_per_thread--;
    }
    args[i].start_core = start_core;
    args[i].end_core   = start_core + cores_per_thread;
    start_core        += cores_per_thread;
    args[i].thread_num = i;
    args[i].stop_cycle = stop_cycle;
    args[i].cores      = &cores;
  }
  // Have the last thread do the remainder
  args[total_simulation_threads - 1].end_core = num_cores * num_L2s;

  PrintElapsedTime("Setup time", time_start);

  // Now run the simulation

  // Allow the debugger a chance to interrupt right before simulation starts 
  // so users can enter initial commands (breakpoints, etc).
  if(run_debugger)
    {
      debugger.setDwarfReader(&dwarfReader);
      debugger.setRegisterSymbols(&regs);
      debugger.enable();
    }

  // Allow for the user to set up initial breakpoints
  debugger.run(NULL, NULL); // null args to indicate first invocation 

  //while(true) { // put a loop here for multiple frames
  boost::chrono::system_clock::time_point prev_frame_time = boost::chrono::system_clock::now();
  if(serial_execution)
    SerialExecution(args, num_cores * num_L2s);
  
  else {
    for(int i = 0; i < total_simulation_threads; ++i) {
      printf("Creating thread %d...\n", (int)i);
      pthread_create( &threadids[i], &attr, CoreThread, (void *)&args[i] );
    }
    
    // Wait for machine to halt
    for(int i = 0; i < total_simulation_threads; ++i) {
      pthread_join( threadids[i], NULL );
    }
  }
  PrintElapsedTime("Frame time", prev_frame_time);

  delete[] args;

  // After reaching this point, the machine has halted.
  // Take a look and print relevant stats
  
  // get highest cycle count
  long long int cycle_count = 0;
  for(size_t i = 0; i < num_cores * num_L2s; ++i) {
    if(cores[i]->cycle_num > cycle_count) 
      cycle_count = cores[i]->cycle_num;
  }

  if(run_profile)
    {
      FILE* profile_output = fopen("profile.out", "w");      
      if(!profile_output)
	{
	  printf("Error: could not open \"profile.out\". Profiling data not written.\n");
	}
      else
	{
	  PrintProfile(assem_file, instructions, source_names, profile_output, profiler, cycle_count * num_cores * num_L2s * num_thread_procs);
	  fclose(profile_output);
	}
    }
  
  
  if(print_cpi) {
    
    // Print the resting state of each core? (they should all be halted)
    if(trax_verbosity) {
      for (size_t i = 0; i < num_cores * num_L2s; ++i) {
	printf("<=== Core %d ===>\n", (int)i);
	cores[i]->issuer->print();
      }
    }
    
    // Count the system-wide stats, sum/average of all cores
    for (size_t i = 0; i < num_cores * num_L2s; ++i) {
      if(trax_verbosity)
	printf("\n ## Core %d ##\n", (int)i);
      
      NormalizeUtilization(cores[i]->cycle_num, cores[i]->utilizations);

      if(trax_verbosity)
	PrintUtilization(cores[i]->module_names, cores[i]->utilizations);
      
      //if(trax_verbosity)
      //cores[i]->L1->PrintStats();
      
      // After core 0 has been printed, use it to hold the sums of all other cores' stats
      if(i != 0)
	cores[0]->AddStats(cores[i]);
    }
    
    // Print system-wide stats
    if((num_cores * num_L2s) > 1 || !trax_verbosity) {
      printf("System-wide instruction stats (sum/average of all cores):\n");
      cores[0]->issuer->print(num_cores * num_L2s);
      PrintUtilization(cores[0]->module_names, cores[0]->utilizations, num_cores * num_L2s);
    }
    
    printf("System-wide L1 stats (sum of all TMs):\n");
    cores[0]->L1->PrintStats();
    printf("\n");
    
    
    // Print L2 stats and gather agregate data
    long long int L2_accesses = 0;
    long long int L2_misses = 0;
    double L2_area = 0;
    double L2_energy = 0;
    for(size_t i = 0; i < num_L2s; ++i) {
      printf(" -= L2 #%d =-\n", (int)i);
      L2s[i]->PrintStats();
      L2_accesses += L2s[i]->accesses;
      L2_misses += L2s[i]->misses;
      printf("\n");
      L2_area += L2s[i]->area;
      L2_energy += L2s[i]->energy * (L2s[i]->accesses - L2s[i]->stores);
    }
    L2_energy /= 1000000000.f;
    
    // for linesize just use the first L2
    L2Cache* L2 = L2s[0];
    
    // print bandwidth numbers
    int word_size = 4;
    int L1_line_size = (int)pow( 2.f, static_cast<float>(cores[0]->L1->line_size) );
    int L2_line_size = (int)pow( 2.f, static_cast<float>(L2->line_size) );
    float Hz = 1000000000;
    printf("Bandwidth numbers for %dMHz clock (GB/s):\n", static_cast<int>(Hz/1000000));
    printf("   L1 to register bandwidth: \t %f\n", static_cast<float>(cores[0]->L1->accesses) * word_size / cycle_count);
    printf("   L2 to L1 bandwidth: \t\t %f\n", static_cast<float>(cores[0]->L1->bus_transfers) * word_size * L1_line_size / cycle_count);
    
    double DRAM_BW;
    if(disable_usimm) {
      DRAM_BW = static_cast<float>(L2_misses) * word_size * L2_line_size / cycle_count;
    }
    else {
      long long int total_lines_transfered = 0;
      for(int c=0; c < NUM_CHANNELS; c++)
	total_lines_transfered += stats_reads_completed[c];
      DRAM_BW = static_cast<float>(total_lines_transfered) * L2_line_size * word_size / cycle_count;
    }
    printf("   memory to L2 bandwidth: \t %f\n", DRAM_BW);
    
    
    if(trax_verbosity) {
      printf("Final Global Register values: imgSize=%d\n", image_width*image_height);
      globals.print();
    }
    
    //printf("L1 energy(J) = %f, area(mm2) = %f\n", (cores[0]->L1->energy * cores[0]->L1->accesses) / 1000000000.f, cores[0]->L1->area * num_cores * num_L2s);
    
    LocalStore* ls_unit = NULL;
    for(size_t i = 0; i < cores[0]->issuer->units.size(); i++) {
      ls_unit = dynamic_cast<LocalStore*>(cores[0]->issuer->units[i]);
      if(ls_unit) 
	break;
    }
    
    double L1_area = cores[0]->L1->area * num_cores * num_L2s;
    double icache_area = cores[0]->issuer->GetArea() * num_cores * num_L2s;
    double compute_area = core_size * num_cores * num_L2s;
    double register_area = cores[0]->thread_procs[0]->thread_states[0]->registers->GetArea() * cores[0]->num_thread_procs * num_cores * num_L2s;
    double localstore_area = 0;
    if(!ls_unit)
      printf("Warning: could not find localstore unit to compute area/energy\n");
    else
      localstore_area = ls_unit->GetArea() * num_cores * num_L2s;
    
    if(ignore_dcache_area)
      {
	L1_area = 0;
	L2_area = 0;
      }
    
    
    double total_area = L1_area + L2_area + compute_area + icache_area + localstore_area + register_area;
    
    printf("\n");
    
    printf("Chip area (mm2):\n");
    printf("   Functional units: \t %f\n", compute_area);
    printf("   L1 data caches: \t %f\n", L1_area);
    printf("   L2 data caches: \t %f\n", L2_area);
    printf("   Instruction caches: \t %f\n", icache_area);
    printf("   Localstore units: \t %f\n", localstore_area);
    printf("   Register files: \t %f\n", register_area);
    printf("   ------------------------------\n");
    printf("   Total: \t\t %f\n", total_area);
    
    printf("\n");
    
    // core 0 contains the aggregate accesses
    double L1_energy = (cores[0]->L1->energy * (cores[0]->L1->accesses - cores[0]->L1->stores)) / 1000000000.f;
    
    double compute_energy = 0;
    double icache_energy = 0;
    double localstore_energy = 0;
    double register_energy = 0;
    double DRAM_power = 0;
    double DRAM_energy = 0;
    
    // Calculate compute energy and icache energy
    for(int i = 0; i < Instruction::NUM_OPS; i++) {
      
      // icache energy, 1 activation per instruction
      icache_energy += cores[0]->issuer->GetEnergy() * cores[0]->issuer->instruction_bins[i];
      
      // localstore energy, 1 activation per localstore op
      if(ls_unit->SupportsOp((Instruction::Opcode)(i)))
	localstore_energy += ls_unit->GetEnergy() * cores[0]->issuer->instruction_bins[i];
      
      // RF access energy, each instruction assumed to cause 3 activates (2 reads, 1 write)
      if(i != Instruction::NOP)
	register_energy += cores[0]->thread_procs[0]->thread_states[0]->registers->GetEnergy() * cores[0]->issuer->instruction_bins[i] * 3;
      
      // compute energy, 1 FU activation per op
      for(size_t j = 0; j < cores[0]->issuer->units.size(); j++) {
	if(cores[0]->issuer->units[j]->SupportsOp((Instruction::Opcode)(i))) 
	  compute_energy += cores[0]->issuer->instruction_bins[i] * cores[0]->issuer->units[j]->GetEnergy();
      }
    }
    
    // convert nanojoules to joules
    compute_energy /= 1000000000.f;
    icache_energy /= 1000000000.f;
    localstore_energy /= 1000000000.f;
    register_energy /= 1000000000.f;
    
    DRAM_power = getUsimmPower() / 1000;
    
    double FPS = Hz/static_cast<double>(cycle_count);
    
    DRAM_energy = DRAM_power / FPS;
    double total_energy = compute_energy + L1_energy + L2_energy + icache_energy + localstore_energy + register_energy + DRAM_energy;
    
    
    printf("Energy consumption (Joules):\n");
    printf("   Functional units: \t %f\n", compute_energy);
    printf("   L1 data caches: \t %f\n", L1_energy);
    printf("   L2 data caches: \t %f\n", L2_energy);
    printf("   Instruction caches: \t %f\n", icache_energy);
    printf("   Localstore units: \t %f\n", localstore_energy);
    printf("   Register files: \t %f\n", register_energy);
    printf("   DRAM: \t\t %f\n", DRAM_energy);
    printf("   ------------------------------\n");
    printf("   Total: \t\t %f\n", total_energy);
    printf("   Power draw (watts): \t %f\n\n", (total_energy / (1.f / FPS)));
    
    printf("FPS Statistics:\n");
    printf("   Total clock cycles: \t\t %lld\n", cycle_count);
    printf("   FPS assuming %dMHz clock: \t %.4lf\n", (int)Hz / 1000000, FPS);
    
    printf("\n\n");
    
    if(!disable_usimm)
      printUsimmStats();
  }  // end print_cpu
  
  fflush(stdout);
  
  if(print_png) {

    const int imgNameLen = strlen(output_prefix);
    bool outputPrefixHasExtension = false;
    for(int i=imgNameLen-1; i>=0; --i) {
      if(output_prefix[i]=='\\' || output_prefix[i]=='/')
        break;
      if(output_prefix[i]=='.') {
        outputPrefixHasExtension = true;
        break;
      }
    }

    // Get output filename with appropriate extension
    char outputName[512];
    const char *outputExt = (use_png_ext_for_output ? "png" : "ppm");
    const bool outputPrefixHasExt = (imgNameLen > 3 &&
                                     output_prefix[imgNameLen-3] == outputExt[0] &&
                                     output_prefix[imgNameLen-2] == outputExt[1] &&
                                     output_prefix[imgNameLen-1] == outputExt[2]);

    // if not extension - just add it at the end
    if(outputPrefixHasExt) {
      sprintf(outputName, "%s", output_prefix);
    }
    else {
      sprintf(outputName, "%s.%s", output_prefix, outputExt);
    }

    // Write out using PNG extension?
    if(use_png_ext_for_output) {

      // save using lodePNG
      unsigned char *imgRGBA = new unsigned char[4*image_width*image_height];
      unsigned char *curImgPixel = imgRGBA;
      for(int j = (int)(image_height - 1); j >= 0; j--) {
        for(int i = 0; i < image_width; i++, curImgPixel+=4) {
          const int index = start_framebuffer + 3 * (j * image_width + i);

          float rgb[3];
          rgb[0] = memory->getData()[index + 0].fvalue;
          rgb[1] = memory->getData()[index + 1].fvalue;
          rgb[2] = memory->getData()[index + 2].fvalue;

          curImgPixel[0] = (char)(int)(rgb[0] * 255);
          curImgPixel[1] = (char)(int)(rgb[1] * 255);
          curImgPixel[2] = (char)(int)(rgb[2] * 255);
          curImgPixel[3] = 255;
        }
      }

      unsigned char* png;
      size_t pngsize;

      unsigned error = lodepng_encode32(&png, &pngsize, imgRGBA, image_width, image_height);
      if(!error)
        lodepng_save_file(png, pngsize, outputName);
      else
        printf("Error %u: %s\n", error, lodepng_error_text(error));

      free(png);
      delete[] imgRGBA;
    }

    // Write out using PPM extension
    else {

      FILE* output = fopen(outputName, "wb");
      if(!output)
        printf("Error: Failed to open image output file: %s\n", outputName);

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
        for(int j = image_height - 1; j >= 0; j--) {
          for(int i = 0; i < image_width; i++) {
            const int index = start_framebuffer + 3 * (j * image_width + i);

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
        } // end for j
        fclose(output);
      }
    } // end if print_png


  // reset the cores for a fresh frame
  globals.Reset();
  for(size_t i = 0; i < num_cores * num_L2s; ++i) {
    cores[i]->Reset();
  }
  
  for(size_t i = 0; i < num_L2s; i++)
    L2s[i]->Reset();
  
  //} // put a loop around all this for multiple frames (see loop comment above)
  
  // clean up thread stuff
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&atominc_mutex);
  pthread_mutex_destroy(&memory_mutex);
  pthread_mutex_destroy(&sync_mutex);
  pthread_mutex_destroy(&global_mutex);
  pthread_mutex_destroy(&profile_mutex);
  for(int i=0; i < MAX_NUM_CHANNELS; i++) {
    pthread_mutex_destroy(&(usimm_mutex[i]));
  }
  pthread_cond_destroy(&sync_cond);
  
  
  for(size_t i=0; i<cores.size(); i++){
    delete cores[i];
  }
  delete[] L2s;
  PrintElapsedTime("Total time", time_start);

#if TRACK_LINE_STATS
  L1Cache* L10 = cores[0]->L1;
  int numLines = L10->cache_size>>L10->line_size;

  printf("Average reads per resident line\tAverage cycles resident\tTotal Accesses\tTotal Hits\n");

  for(int i=0; i < numLines; i++)
    printf("%f\t%f\t%lld\t%lld\n", (float)(L10->total_reads[i]) / (float)(L10->total_validates[i]),
                                   (float)(L10->current_cycle) / (float)(L10->total_validates[i]),
                                   L10->line_accesses[i],
                                   L10->total_reads[i]);
#endif


  return 0;
}

void StrReplace(char* str, char old, char replace)
{
  while(*str != '\0')
    {
      if(*str == old)
	*str = replace;
      str++;
    }
}

void PrintProfile(const char* assem_file, std::vector<Instruction*>& instructions, std::vector<std::string> srcNames, FILE* profile_output, Profiler profiler, long long int total_thread_cycles)
{

  profiler.PrintProfile(instructions, total_thread_cycles);

  FILE* input = fopen(assem_file, "r");
  if(!input)
    {
      printf("Error: could not read assembly file: %s. Profiling data not written.\n", assem_file);
    }
  else
    {
      fprintf(profile_output, "Assembly:\tNum Executions\tData Stall Cycles\tSource File\tSource Line\n");
      int line_num = 0;
      bool isInstruction = false;
      int instruction_num = 0;
      const char *delimeters = ", ()[\t\n"; // includes only left bracket for backwards compatibility with vector registers
      char line[1000];
      char temp[1000]; // for tokenizing
      while(!feof(input))
	{
	  isInstruction = false;
	  if(!fgets(line, 1000, input))
	    {
	      continue;
	    }

	  line_num++;
	  memcpy(temp, line, strlen(line) + 1);
	  char *token = strtok(temp, delimeters);
	  if(token==NULL)
	    {
	      continue;
	    }
	  if(token[0]=='#')
	    continue;

	  if(token[strlen(token)-1]==':') // label symbol
	    {
	      token = strtok(NULL, delimeters); // get rid of any empty space, so the instruction is next
	    }
	  
	  //printf("instruction_num = %d, size = %d\n", instruction_num, instructions.size());
	  if(instruction_num < (int)instructions.size() && token != NULL && strcmp(token, Instruction::Opnames[instructions[instruction_num]->op].c_str()) == 0)
	    {
	      isInstruction = true;
	    }


	  StrReplace(line, '\t', ' ');
	  StrReplace(line, '\n', ' ');
	  
	  fprintf(profile_output, "%d: ", line_num);
      fprintf(profile_output, "%s", line);
	  
	  if(isInstruction)
	    {
	      if(instructions[instruction_num]->srcInfo.fileNum < 0)
		{
		  fprintf(profile_output, "\t%lld\t%lld", 
			  instructions[instruction_num]->executions, instructions[instruction_num]->data_stalls);
		}
	      else
		{
		  fprintf(profile_output, "\t%lld\t%lld\t%s\t%d", 
			  instructions[instruction_num]->executions, instructions[instruction_num]->data_stalls,
			  srcNames.at(instructions[instruction_num]->srcInfo.fileNum).c_str(), instructions[instruction_num]->srcInfo.lineNum);
		}
	      instruction_num++;
	    }
	  fprintf(profile_output, "\n");
	}
    }
}
