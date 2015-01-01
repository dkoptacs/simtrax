#ifndef TRAX_TRAX_H
#define TRAX_TRAX_H


// Trax Intrinsics
extern "C"
{
  // Global Memory
  extern int loadi( int base, int offset = 0 ) asm("llvm.mips.loadi");
  extern float loadf( int base, int offset = 0 ) asm("llvm.mips.loadf");
  extern void storei( int value, int base, int offset = 0 ) asm("llvm.mips.storei");
  extern void storef( float value, int base, int offset = 0 ) asm("llvm.mips.storef");

  // Arithmetic
  extern int atomicinc( int location ) asm("llvm.mips.atominc");
  extern float min( float left, float right ) asm("llvm.mips.min");
  extern float max( float left, float right ) asm("llvm.mips.max");
  extern float invsqrt( float value ) asm("llvm.mips.invsqrt");

  // Misc
  extern int global_reg_read( int location ) asm("llvm.mips.globalrrd");
  extern int trax_getid( int value ) asm("llvm.mips.getid");
  
  // Random
  extern float trax_rand( ) asm("llvm.mips.rand");

  // Synchronization
  extern int trax_inc_reset( int reg_num ) asm("llvm.mips.increset");
  extern void trax_barrier( int reg_num ) asm("llvm.mips.barrier");
  extern void trax_semacq( int reg_num ) asm("llvm.mips.semacq");
  extern void trax_semrel( int reg_num ) asm("llvm.mips.semrel");

  // Noise
  extern void trax_write_srf( float reg1, float reg2, float reg3 ) asm("llvm.mips.writesrf");
  extern void trax_write_sri( int reg1, int reg2, int reg3 ) asm("llvm.mips.writesri");
  extern float trax_callnoise( ) asm("llvm.mips.callnoise");
  
  // Streams
  // Stream writes
  extern void start_stream_write( int stream_id ) asm("llvm.mips.startsw");
  extern void stream_writei( int value ) asm("llvm.mips.streamwi");
  extern void stream_writef( float value ) asm("llvm.mips.streamwf");
  extern void end_stream_write() asm("llvm.mips.endsw");
  // Stream reads
  extern int start_stream_read() asm("llvm.mips.startsr");
  extern int stream_readi() asm("llvm.mips.streamri");
  extern float stream_readf() asm("llvm.mips.streamrf");
  extern void end_stream_read() asm("llvm.mips.endsr");
  // Stream scheduling
  extern int stream_size( int stream_id ) asm("llvm.mips.streamsize");
  extern int stream_schedule( int schedule_id ) asm("llvm.mips.streamschedule");
  extern void set_stream_read_id( int stream_id ) asm("llvm.mips.setstreamid");

  // Debug
  extern void profile( int prof_id ) asm("llvm.mips.profile");
  extern int loadl1( int base, int offset = 0 )asm("llvm.mips.loadl1");
  //extern int loadl2( int base, int offset = 0 )asm("llvm.mips.loadl2");
  extern void trax_printi(int value) asm("llvm.mips.printi");      // print integer
  extern void trax_printf( float value ) asm("llvm.mips.printf");  // print float (unfortunately this uses the "printf" name)
  extern void trax_printformat(const char** string_addr) asm("llvm.mips.printfmstr"); // 'equivalent' to stdio::printf
  
}

// simplified printf for trax
extern int printf ( const char * format, ... );


inline int barrier( int reg_num ) {
  int reg_val = trax_inc_reset( reg_num );
  trax_barrier( reg_num );
  return reg_val;
}

inline float trax_noise( float x, float y, float z ) {
  trax_write_srf(x,y,z);
  return trax_callnoise();
}

// main function the user must define instead of "main"
void trax_main()__attribute__((noinline));

// the main function (in trax_trax.cpp just calls trax_main())
int main();

#endif
