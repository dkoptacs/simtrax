#include "FourByte.h"
#include "MainMemory.h"
#include "L2Cache.h"
#include "Instruction.h"
#include "ThreadState.h"
#include "SimpleRegisterFile.h"

#include <pthread.h>

// use this if writes need to be atomic
extern pthread_mutex_t memory_mutex;

// Windows pipe stuff (needs stdio.h)
#ifdef WIN32
# ifndef popen
#	define popen _popen
#	define pclose _pclose
# endif
#endif


MainMemory::MainMemory(int _num_blocks,  int _latency)
 :latency(_latency) {
  num_blocks = _num_blocks;
  issued_atominc = false;
  data = new FourByte[num_blocks];
  store_count = 0;
  start_framebuffer = 23;
  stores_between_output = 64;
  image_height = 0;
  image_width = 0;
  incremental_output = false;
}


bool MainMemory::IssueInstruction(Instruction* ins, L2Cache* L2, ThreadState* thread,
				  int& ret_latency, long long int current_cycle) {
  if (ins->op == Instruction::STORE) {
    // maybe should have some limit for number of ops here.
    ret_latency = latency;
    // this is okay because the read was registered in the L1Cache
    reg_value arg0;
    Instruction::Opcode failop = Instruction::NOP;
    if (!thread->ReadRegister(ins->args[0], current_cycle, arg0, failop))
      {
	printf("%s unable to read\n", Instruction::Opnames[failop].c_str());
      }
    int address = arg0.idata + ins->args[2];
    //    printf(" Store: %x: %d\n", address, ins->args[1]);
    if (address < 0 || address > num_blocks) {
      printf("Memory address out of bounds for write!\n");
      return true;
    }
    reg_value arg1;
    if (!thread->ReadRegister(ins->args[1], current_cycle, arg1, failop)) {
      // bad stuff happened
      printf("Error in Main Memory. Should have passed.\n");
    }    
    pthread_mutex_lock(&memory_mutex);
    data[address].uvalue = arg1.udata;
    store_count++;
    pthread_mutex_unlock(&memory_mutex);
    return true;
  }

  // Output as we go along
  if(incremental_output && store_count%stores_between_output==0) {
    char command_buf[512];
    sprintf(command_buf, "convert PPM:- out%d.png", store_count/stores_between_output);
    FILE* output = popen(command_buf, "w");
    if (!output)
      perror("Failed to open out.png");
    
    fprintf(output, "P6\n%d %d\n%d\n", image_width, image_height, 255);
    for (int j = image_height - 1; j >= 0; j--) {
      for (int i = 0; i < image_width; i++) {
	int index = start_framebuffer + 3 * (j * image_width + i);
	float rgb[3];
	// for gradient/colors we have the result
	rgb[0] = L2->mem->getData()[index + 0].fvalue;
	rgb[1] = L2->mem->getData()[index + 1].fvalue;
	rgb[2] = L2->mem->getData()[index + 2].fvalue;
	fprintf(output, "%c%c%c",
		(char)(int)(rgb[0] * 255),
		(char)(int)(rgb[1] * 255),
		(char)(int)(rgb[2] * 255));
      }
    }
    pclose(output);  
  }
  
  return false;
}

int MainMemory::GetLatency(Instruction* ins) {
  return latency;
}
