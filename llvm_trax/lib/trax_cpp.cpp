#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <pthread.h>
#include <fstream>

#include "WinCommonNixFcns.h"
#include "trax_cpp.hpp"
#include "LoadMemory.h"
#include "CustomLoadMemory.h"
#include "ReadLightfile.h"
#include "ReadViewfile.h"
#include "lodepng.h"


// Windows pipe stuff for trax_cleanup (needs stdio.h)
#ifdef WIN32
# ifndef popen
#   define popen _popen
#   define pclose _pclose
# endif
#endif


#ifndef FAR
#define FAR 1000
#endif


// Flag for when we want to run multiple pthreads for 1 TM
//#define MULTIPLE_THREADS_ONE_CORE


/// Pthreads stuff
const unsigned int numAtomicInc = 8;
pthread_t *threads = 0;
pthread_mutex_t mainMutex[ numAtomicInc ];
pthread_cond_t mainCond;
unsigned int numThreads = 0;
bool mainCondInit       = false;


// C versions of everything
FourByte * trax_memory_pointer;
FourByte * trax_global_registers;
#define MEMORY_SIZE 33554432			// 2^25


int loadi( int base, int offset ) {
  assert( base+offset < MEMORY_SIZE );
  return trax_memory_pointer[base+offset].ivalue;
}
float loadf( int base, int offset ) {
  assert( base+offset < MEMORY_SIZE );
  return trax_memory_pointer[base+offset].fvalue;
}
void storei( int value, int base, int offset ) {
  assert( base+offset < MEMORY_SIZE );
  trax_memory_pointer[base+offset].ivalue = value;
}
void storef( float value, int base, int offset ) {
  assert( base+offset < MEMORY_SIZE );
  trax_memory_pointer[base+offset].fvalue = value;
}
int trax_getid( int value ) {
  if (value == 1) {
#ifndef MULTIPLE_THREADS_ONE_CORE
  pthread_t thisID = pthread_self();
  for( unsigned int tid = 0; tid<numThreads; ++tid )
#ifdef WIN32
    if( thisID.p==threads[tid].p )
#else
    if( thisID==threads[tid] )
#endif
      return tid;
#endif
    return 0; // threadID, if not found
  } else if (value == 2) {
#ifdef MULTIPLE_THREADS_ONE_CORE
    pthread_t thisID = pthread_self();
    for( unsigned int tid = 0; tid<numThreads; ++tid )
  #ifdef WIN32
      if( thisID.p==threads[tid].p )
  #else
      if( thisID==threads[tid] )
        return tid;
  #endif
#endif
    return 0; // coreID
  } else if (value == 3) {
    return 0; // L2 ID
  }
  // for anything else
  return 0;
}

// Arithmetic
int atomicinc( int location) {
  if( threads ) {
    pthread_mutex_lock( &mainMutex[location] );
    unsigned int tmp = trax_global_registers[location].uvalue++;
    pthread_mutex_unlock( &mainMutex[location] );
    return tmp;
  }
  return trax_global_registers[location].uvalue++;
}
int global_reg_read( int location ) {
  return trax_global_registers[location].uvalue;
}
float min( float left, float right ) {
  return left < right ? left : right;
}
float max( float left, float right ) {
  return left > right ? left : right;
}

float invsqrt( float value ) {
  return 1.f / sqrt(value);
}


float trax_rand() {
  return drand48();
}
void barrier( int reg_num ) {
  if ( threads && mainCondInit ) {
    // Be counted
    pthread_mutex_lock( &mainMutex[reg_num] );
    trax_global_registers[reg_num].uvalue++;
    if ( trax_global_registers[reg_num].uvalue == numThreads ) {
      // if every thread finished, let them know and reset
      trax_global_registers[reg_num].uvalue = 0;
      pthread_cond_broadcast( &mainCond );
    } else {
      // wait
      pthread_cond_wait( &mainCond, &mainMutex[reg_num] );
    }
    pthread_mutex_unlock( &mainMutex[reg_num] );
  }
  return;
}
float trax_noise( float x, float y, float z ) {
  // ***TODO: put real noise function here
  return 1.0f;
}


// Debug
void profile( int prof_id ) {}
int loadl1( int base, int offset ) {return 0;}
void trax_printi(int value) { printf("Value is %d.\n", value);}
void trax_printf( float value ) { printf("Value is %f.\n", value);}
// printf for trax_cpp is defined in stdio

int start_framebuffer;


void trax_setup( runrtParams_t &opts ) {

  // everything we can modify here has been set within the 
  // parameters struct opts

  // setup global registers
  trax_global_registers = new FourByte[opts.num_global_registers];
  for( unsigned int i = 0; i < opts.num_global_registers; ++i ) {
    trax_global_registers[i].ivalue = 0;
  }

  // ***IMPORTANT: If you set load_mem_from_file to true, be sure that the 
  // memory image has the right frame buffer size or you will get weird
  // images as your results
  if ( opts.read_from_mem_file ) {
    // load memory from dump
	  printf( "Attempting to load memory file '%s'.\n", opts.mem_file_name.c_str() );
    float *light_pos = new float[3];
    int num_blocks;
	  std::ifstream filein( opts.mem_file_name.c_str() );
    // check for errors                                                                                                                                                                                                                                                                                                      
    if (filein.fail()) { 
      printf( "Error opening memory file '%s' for reading.\n",  opts.mem_file_name.c_str() );
      exit(-1);
    }

    filein >> num_blocks;
    trax_memory_pointer = new FourByte[num_blocks];

    // Original memory file organization, for Backwards compatibility
    //filein >> start_wq ;
    //filein >> start_framebuffer ;
    //filein >> start_scene ;
    //filein >> start_matls ;
    //filein >> start_camera ;
    //filein >> start_bg_color ;
    //filein >> start_light ;
    //filein >> end_memory ;
    //filein >> light_pos[0] >> light_pos[1] >> light_pos[2] ;
    //filein >> start_permutation ;

    for (int i = 0; i < num_blocks; ++i) {
      filein >> trax_memory_pointer[i].ivalue;
    }
    printf( "Memory loading complete.\n" ); 

  } else { // load memory from setup files
    // set up memory
    trax_memory_pointer = new FourByte[MEMORY_SIZE]; 

    float epsilon         = 1e-4f;
    if (opts.custom_mem_loader) {
      // Custom memory loader
      CustomLoadMemory(trax_memory_pointer, MEMORY_SIZE, opts.img_width, opts.img_height, 
		       epsilon, opts.custom_mem_loader);
    } else {

      BVH* bvh;
      int grid_dimensions   = -1;
      Camera* camera        = ReadViewfile::LoadFile( opts.view_file_name.c_str(), FAR );
      int start_wq, start_scene, start_camera, start_bg_color, start_light, end_memory;
      int start_matls, start_permutation;
      float *light_pos      = new float[3];
      ReadLightfile::LoadFile( opts.light_file_name.c_str(), light_pos );
      int tile_width        = 16;
      int tile_height       = 16;
      int ray_depth         = opts.ray_depth;
      int num_samples       = opts.num_samples_per_pixel;
      int num_thread_procs  = opts.num_render_threads; // not sure why this is needed
      int num_cores         = 1; // this either
      bool duplicate_bvh    = false;
      bool tris_store_edges = opts.triangles_store_edges;
      const char* model = opts.no_scene ? NULL : opts.model_file_name.c_str();

      LoadMemory(trax_memory_pointer, bvh, MEMORY_SIZE, opts.img_width, opts.img_height, 
                 grid_dimensions,
                 camera, model,
                 start_wq, start_framebuffer, start_scene,
                 start_matls,
                 start_camera, start_bg_color, start_light, end_memory,
                 light_pos, start_permutation, tile_width, tile_height,
                 ray_depth, num_samples, num_thread_procs * num_cores, num_cores,
                 opts.subtree_size, epsilon, duplicate_bvh, tris_store_edges);
      
      
      // print out the DOT file?
      if( opts.bvh_dot_depth > 0 ) {
        bvh->writeDOT( "bvh.dot", start_scene, trax_memory_pointer, 1, opts.bvh_dot_depth );
      }
    }
  }
}

void trax_cleanup( runrtParams_t &opts ){

  unsigned found = opts.output_prefix_name.find_last_of("/\\");
  std::string path = opts.output_prefix_name.substr(0, found + 1);
  std::string baseName = opts.output_prefix_name.substr(found + 1);

  bool usePNGLibrary = true;
  if(baseName.rfind(".") == std::string::npos) {
    baseName += ".png";
  }
  else {
    usePNGLibrary = false;
  }
  baseName = path + baseName;

  // save using lodePNG
  if(usePNGLibrary) {
    unsigned char *imgRGBA = new unsigned char[4*opts.img_width*opts.img_height];
    unsigned char *curImgPixel = imgRGBA;
    for(int j = (int)(opts.img_height - 1); j >= 0; j--) {
      for(unsigned int i = 0; i < opts.img_width; i++, curImgPixel+=4) {
        const int index = start_framebuffer + 3 * (j * opts.img_width + i);

        float rgb[3];
        rgb[0] = trax_memory_pointer[index + 0].fvalue;
        rgb[1] = trax_memory_pointer[index + 1].fvalue;
        rgb[2] = trax_memory_pointer[index + 2].fvalue;

        curImgPixel[0] = (char)(int)(rgb[0] * 255);
        curImgPixel[1] = (char)(int)(rgb[1] * 255);
        curImgPixel[2] = (char)(int)(rgb[2] * 255);
        curImgPixel[3] = 255;
      }
    }

    unsigned char* png;
    size_t pngsize;

    unsigned error = lodepng_encode32(&png, &pngsize, imgRGBA, opts.img_width, opts.img_height);
    if(!error)
      lodepng_save_file(png, pngsize, baseName.c_str());
    else
      printf("error %u: %s\n", error, lodepng_error_text(error));

    free(png);
    delete[] imgRGBA;
  }

  // otherwise, save with calling convert
  else {
    // output the image
    char command_buf[512];
	sprintf(command_buf, "convert PPM:- %s", baseName.c_str());
#ifndef WIN32
    FILE* output = popen(command_buf, "w");
#else
    FILE* output = popen(command_buf, "wb");
#endif
    if (!output) {
	  char errorMsg[512];
      sprintf(errorMsg, "Failes to open %s", baseName.c_str());
      perror(errorMsg);
	}

    fprintf(output, "P6\n%d %d\n%d\n", opts.img_width, opts.img_height, 255);
    for (int j = static_cast<int>(opts.img_height - 1); j >= 0; j--) {
      for (unsigned int i = 0; i < opts.img_width; i++) {
        int index = start_framebuffer + 3 * (j * opts.img_width + i);
	    
        float rgb[3];
        // for gradient/colors we have the result
        rgb[0] = trax_memory_pointer[index + 0].fvalue;
        rgb[1] = trax_memory_pointer[index + 1].fvalue;
        rgb[2] = trax_memory_pointer[index + 2].fvalue;
        fprintf(output, "%c%c%c",
	        (char)(int)(rgb[0] * 255),
	        (char)(int)(rgb[1] * 255),
	        (char)(int)(rgb[2] * 255));
      }
    }
    pclose(output);
  }
}



/// creates a bunch of threads to run rendering function
bool trax_start_render_threads( void* (*renderFunc)(void*), const int &_numThreads ) {

  // just in case
  if( threads )
    return false;

  numThreads  = (_numThreads>0) ? _numThreads : 1;
  threads	  = new pthread_t[numThreads];


  // init mutexes
  for( unsigned int m=0; m<numAtomicInc; ++m ) {
    if( pthread_mutex_init( &mainMutex[m], NULL ) ) {
      printf( "Unable to initialize mutex %d\n", m );
      return false;
    }
  }

  // init cond
  if( pthread_cond_init( &mainCond, NULL ) != 0 ) {
    printf( "Could not create conditional for barrier\n" );
    return false;
  }
  mainCondInit = true;


  // start up the rendering...
  for( unsigned int i=0; i<numThreads; ++i ) {
    if( pthread_create( &threads[i], NULL, renderFunc, 0 ) ) {
      printf( "Could not create thread %d\n", i );
      return false;
    }
  }


  // make sure all the threads exit upon completion
  for( unsigned int i=0; i<numThreads; ++i ) {
    if( pthread_join( threads[i], NULL ) ) {
      printf( "Could not join thread %d\n", i );
      return false;
    }
  }

  // clear out the memory
  delete[] threads;
  threads = 0;
  for( unsigned int m=0; m<numAtomicInc; ++m )
    pthread_mutex_destroy( &mainMutex[m] );
  pthread_cond_destroy( &mainCond );
  mainCondInit = false;
  return true;
}


// main function
int main( int argc, char* argv[] ) {

  // parse some input, like number of processors, etc
  runrtParams_t opts;
  programOptParser( opts, argc, argv );

  trax_setup( opts );

  if( trax_start_render_threads( &trax_mainPThreads, opts.num_render_threads ) == false ) {
    std::cout << "No threads available for rendering -> Ray Queues won't work!\n";

    // time to perform one centralized cleanup :D
    delete[] threads;
    threads = 0;
    for( unsigned int m=0; m<numAtomicInc; ++m )
      pthread_mutex_destroy( &mainMutex[m] );
    pthread_cond_destroy( &mainCond );
    mainCondInit = false;

    // do single threaded execution
    std::cout << "Executing single threaded!\n";
    trax_main();
  }

  trax_cleanup( opts );
  return 0;
}





// prints help menu
void printHelp( char* progName ) {

  // for defaults
  runrtParams_t def;

  printf( "Usage for: %s\n", progName );
  printf( "\t--help              [prints help menu]\n" );
  printf( "\t--custom-mem-loader <custom loader to use -- default: %d (off)\n", def.custom_mem_loader );
  printf( "\t--height            <height in pixels -- default: %d>\n", def.img_height );
  printf( "\t--light-file        <light file name -- default: %s>\n", def.light_file_name.c_str() );
  printf( "\t--mem-file          <memory file to load -- default: %s>\n", def.mem_file_name.c_str() );
  printf( "\t--model             <model file name (.obj) -- default: %s>\n", def.model_file_name.c_str() );
  printf( "\t--num-cores         <number of render threads -- default: %d>\n", def.num_render_threads );
  printf( "\t--num-globals       <number of global registers -- default %d>\n", def.num_global_registers );
  printf( "\t--num-samples       <number of samples per pixel -- default: %d>\n", def.num_samples_per_pixel );
  printf( "\t--output-prefix      <prefix for image output. Be sure any directories exist> -- default: %s\n", def.output_prefix_name.c_str() );
  printf( "\t--ray-depth         <depth of rays -- default: %d>\n", def.ray_depth );
  printf( "\t--subtree-size      <minimum size in words of BVH subtrees -- default: %d (won't build subtrees)>\n", def.subtree_size );
  printf( "\t--triangles-store-edges [set flag to store 2 edge vecs in a tri instead of 2 verts -- default: off]\n" );
  printf( "\t--view-file         <view file name -- default: %s>\n", def.view_file_name.c_str() );
  printf( "\t--width             <width in pixels -- default: %d>\n", def.img_width );
  printf( "\t--write-dot         <depth>  generates dot files for the tree after each frame. Depth should not exceed 8\n");
}


// options parser
void programOptParser( runrtParams_t &params, int argc, char* argv[] ) {

  for( int i=1; i<argc; ++i ) {
    printf( "%s ", argv[i] );
    int argnum = i;

    // print help
    if( strcmp(argv[i], "--help")==0 ) {
      printHelp( argv[0] );
      exit( 0 );
    }

    // custom memory loader?
    else if( strcmp(argv[i], "--custom-mem-loader")==0 ) {
      params.custom_mem_loader     = atoi( argv[++i] );
    }

    // image height
    else if( strcmp(argv[i], "--height")==0 ) {
      params.img_height            = atoi( argv[++i] );
    }

    // light file
    else if( strcmp(argv[i], "--light-file")==0 ) {
      params.light_file_name       = argv[++i];
    }

    // read in memory file?
    else if( strcmp(argv[i], "--mem-file")==0 ) {
      params.read_from_mem_file    = true;
      params.mem_file_name         = argv[++i];
    }

    // model file name
    else if( strcmp(argv[i], "--model")==0 ) {
      params.model_file_name       = argv[++i];
    }

    // set number of cores for rendering
    else if( strcmp(argv[i], "--num-cores")==0 ) {
      params.num_render_threads    = atoi( argv[++i] );
    }

    // number of global registers
    else if( strcmp(argv[i], "--num-globals")==0 ) {
      params.num_global_registers  = atoi( argv[++i] );
    }

    // number of samples per pixel
    else if( strcmp(argv[i], "--num-samples")==0 ) {
      params.num_samples_per_pixel = atoi( argv[++i] );
    }

	// output file prefix
	else if( strcmp(argv[i], "--output-prefix")==0 ) {
		params.output_prefix_name  = argv[++i];
	}

    // ray depth for traversal
    else if( strcmp(argv[i], "--ray-depth")==0 ) {
      params.ray_depth             = atoi( argv[++i] );
    }

    // store edges instead of points for triangle?
    else if( strcmp(argv[i], "--triangles-store-edges")==0 ) {
      params.triangles_store_edges = true;
    }

    // view file
    else if( strcmp(argv[i], "--view-file")==0 ) {
      params.view_file_name        = argv[ ++i ];
    }

    // image width
    else if( strcmp(argv[i], "--width")==0 ) {
      params.img_width             = atoi( argv[++i] );
    }

    // write dot file for BVH
    else if( strcmp(argv[i], "--write-dot")==0 ) {
      params.bvh_dot_depth         = atoi( argv[++i] );
    }

    // don't load a model
    else if( strcmp(argv[i], "--no-scene")==0 ) {
      params.no_scene              = true;;
    }

    // badness - help?
    else {
      printf( "Unrecognized option %s\n", argv[i] );
      printHelp( argv[0] );
      exit( -1 );
    }

    if( argnum!=i )
      printf( " %s", argv[i] );
    printf( "\n" );
  }
}


// pthreads render-horsie
void * trax_mainPThreads( void * dummyPtr ) {
  trax_main();
  return 0;
}
