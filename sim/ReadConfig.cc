#include "ReadConfig.h"
#include "HardwareModule.h"
#include "FunctionalUnit.h"
#include <stdio.h>
#include <stdlib.h>

// Functional units
#include "Bitwise.h"
#include "BranchUnit.h"
#include "ConversionUnit.h"
#include "DebugUnit.h"
#include "FourByte.h"
#include "FPAddSub.h"
#include "FPCompare.h"
#include "FPDiv.h"
#include "FPInvSqrt.h"
#include "FPMinMax.h"
#include "FPMul.h"
#include "FunctionalUnit.h"
#include "Instruction.h"
#include "IntAddSub.h"
#include "IntMul.h"
#include "L1Cache.h"
#include "L2Cache.h"
#include "MainMemory.h"
#include "TraxCore.h"


ReadConfig::ReadConfig(const char* input_file, const char* _dcache_params_file,
		       L2Cache** L2s, size_t num_L2s, MainMemory*& mem,
		       double &size_estimate, bool disable_usimm, bool _memory_trace, bool _l1_off, bool _l2_off, bool _l1_read_copy) :
  input_file(input_file), memory_trace(_memory_trace), l1_off(_l1_off), l2_off(_l2_off), l1_read_copy(_l1_read_copy)
{
  FILE* input = fopen(input_file, "r");
  if (!input) {
    perror("Unable to open config file.");
    exit(1);
  }

  dcache_params_file = _dcache_params_file;

  // assume input is of the form UNIT_TYPE args
  char line_buf[1024];
  while (!feof(input) && fgets(line_buf, sizeof(line_buf), input)) 
    {
      if(line_buf[0] == '#')
	continue;
    char unit_type[100];
    unit_type[0] = '\0';
    sscanf(line_buf, "%s ", unit_type);
    std::string unit_string(unit_type);
    if (unit_string == "MEMORY") {
      // load mem
      int latency;
      int num_blocks;
      int max_bandwidth = 256;
      int scanvalue = sscanf(line_buf, "%*s %d %d %d", &latency, &num_blocks, &max_bandwidth);
      if(scanvalue < 2 || scanvalue > 3){
	printf("ERROR: MEMORY syntax is MEMORY <latency> <memory size> <bandwidth (optional)>\n");
	continue;
      }


      // Each L2 is allowed this much bandwidth
      max_bandwidth /= num_L2s;
      mem = new MainMemory(num_blocks, latency, max_bandwidth);
    } 
    else if (unit_string == "L2") {
      // load L2
      int hit_latency;
      int cache_size;
      int num_banks;
      int line_size;
      float unit_area = 0;
      float unit_energy = 0;

      int scanvalue = sscanf(line_buf, "%*s %d %d %d %d %f %f", &hit_latency,
			     &cache_size, &num_banks, &line_size, &unit_area, &unit_energy);
      if ( scanvalue < 4 || scanvalue > 6 ) {
	printf("ERROR: L2 syntax is L2 <hit latency> <cache size> <num banks> <line size(**2)> <cache area mm^2 (optional)> <energy nJ (optional)>\n");
	continue;
      }
      if (mem == NULL) {
	printf("ERROR: L2 declared before MEMORY!\n");
	continue;
      }

      if(scanvalue != 6)
	{
	  int line_size_bytes = (1 << line_size) * 4;
	  if(!ReadCacheParams(dcache_params_file, cache_size * 4, num_banks, line_size_bytes, unit_area, unit_energy, true))
	    perror("WARNING: Unable to find area and energy profile for specified L2\nAssuming 0\n");
	}


      // Loop to allocate num_L2s 
      for (size_t i = 0; i < num_L2s; ++i) {
	L2s[i] = new L2Cache(mem, cache_size, hit_latency,
			     disable_usimm, unit_area, unit_energy, num_banks, line_size,
			     memory_trace, l2_off, l1_off);
      }

    } else {
      // a per-core module line (skipped here)
    }
  }

  fclose(input);
}

void ReadConfig::LoadConfig(L2Cache* L2, double &size_estimate) {
  FILE* input = fopen(input_file, "r");
  if (!input) {
    perror("Unable to open input file.");
    return;
  }

  std::vector<HardwareModule*>* modules = &(current_core->modules);
  std::vector<std::string>* module_names = &(current_core->module_names);
  std::vector<FunctionalUnit*>* functional_units = &(current_core->functional_units);


  //double size_estimate = 0;
  // assume input is of the form UNIT_TYPE args
  char line_buf[1024];
  while (!feof(input) && fgets(line_buf, sizeof(line_buf), input)) {
    if(line_buf[0] == '#')
      continue;
    char unit_type[100];
    sscanf(line_buf, "%s ", unit_type);
    std::string unit_string(unit_type);
    if (unit_string == "MEMORY" ||
        unit_string == "L2")
      {
      // already loaded
      } 
    else if (unit_string == "L1") {
      // load L1
      int hit_latency;
      int cache_size;
      int num_banks;
      int line_size;
      float unit_area = 0.0;
      float unit_energy = 0.0;
      
      // Try to read area and energy if specified in config file
      int scanvalue = sscanf(line_buf, "%*s %d %d %d %d %f %f", &hit_latency,
			     &cache_size, &num_banks, &line_size, &unit_area, &unit_energy);
      if ( scanvalue < 4 || scanvalue > 6) {
	printf("ERROR: L1 syntax is L1 <hit latency> <cache size> <num banks> <line size(**2)> <cache area mm^2 (optional)> <energy nJ (optional)>\n");
	continue;
      }

      if(scanvalue != 6)
	{
	  int line_size_bytes = (1 << line_size) * 4;
	  if(!ReadCacheParams(dcache_params_file, cache_size * 4, num_banks, line_size_bytes, unit_area, unit_energy, true))
	    perror("WARNING: Unable to find area and energy profile for specified L1\nAssuming 0\n");
	}
      

      current_core->L1 = new L1Cache(L2, hit_latency, cache_size, unit_area, unit_energy,
				     num_banks, line_size,
				     memory_trace, l1_off, l1_read_copy);
      

      modules->push_back(current_core->L1);

    }     
    else {
      // a simple module taking latency and issue width
      int latency;
      int issue_width;
      float unit_area = -1;
      float unit_energy = -1;

      int scanvalue = sscanf(line_buf, "%*s %d %d %f %f", &latency, &issue_width, &unit_area, &unit_energy);
      // see if everything is provided
      if ( scanvalue < 2 || scanvalue > 4) {
        // only one (or none) provided try again with just one param
        issue_width = 1;
        if (sscanf(line_buf, "%*s %d", &latency) != 1) {
          printf("ERROR: Found Unit %s but bad params (line_buf is %s)\n",
                 unit_type, line_buf);
          continue;
        }
        // otherwise it worked
      }


      // Add area to count if given
      if (unit_area >= 0) {
	size_estimate += unit_area * 1e-6 * issue_width;
      }

      if (unit_string == "FPADD") {
        FPAddSub* fp_addsub = new FPAddSub(latency, issue_width);

        modules->push_back(fp_addsub);
        functional_units->push_back(fp_addsub);
        module_names->push_back(std::string("FP AddSub"));
	
	// Hard-code default area/energy inline since they are unlikely to change
	// This is ugly but oh well
        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .003;
	    unit_energy = .0074;
	  }

      } else if (unit_string == "FPMIN") {
        FPMinMax* fp_minmax = new FPMinMax(latency, issue_width);
        modules->push_back(fp_minmax);
        functional_units->push_back(fp_minmax);
        module_names->push_back(std::string("FP MinMax"));

        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .000722;
	    unit_energy = .000536;
	  }
	
      } else if (unit_string == "FPCMP") {
        FPCompare* fp_compare = new FPCompare(latency, issue_width);
        modules->push_back(fp_compare);
        functional_units->push_back(fp_compare);
        module_names->push_back(std::string("FP Compare"));
	
        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .000722;
	    unit_energy = .000536;
	  }

      } else if (unit_string == "INTADD") {
        IntAddSub* int_addsub = new IntAddSub(latency, issue_width);
        modules->push_back(int_addsub);
        functional_units->push_back(int_addsub);
        module_names->push_back(std::string("Int AddSub"));

        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .00066;
	    unit_energy = .000572;
	  }

      } else if (unit_string == "FPMUL") {
        FPMul* fp_mul = new FPMul(latency, issue_width);
        modules->push_back(fp_mul);
        functional_units->push_back(fp_mul);
        module_names->push_back(std::string("FP Mul"));
	
        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .0165;
	    unit_energy = .0142;
	  }

      } else if (unit_string == "FPINV" || unit_string == "FPINVSQRT") {
        FPInvSqrt* fp_invsqrt = new FPInvSqrt(latency, issue_width);
        modules->push_back(fp_invsqrt);
        functional_units->push_back(fp_invsqrt);
        module_names->push_back(std::string("FP InvSqrt"));

	// ****** placeholder, add divide unit with same latency and width as the sqrt unit
        FPDiv* fp_div = new FPDiv(latency, issue_width);
        modules->push_back(fp_div);
        functional_units->push_back(fp_div);
        module_names->push_back(std::string("FP Div"));

        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .112;
	    unit_energy = .1563;
	  }

	// Set the area/energy here since there are two of them
	fp_invsqrt->area = unit_area;
	fp_invsqrt->energy = unit_energy;
	fp_div->area = unit_area;
	fp_div->energy = unit_energy;

      } else if (unit_string == "INTMUL") {
        IntMul* int_mul = new IntMul(latency, issue_width);
        modules->push_back(int_mul);
        functional_units->push_back(int_mul);
        module_names->push_back(std::string("Int Mul"));

        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .0117;
	    unit_energy = .0177;
	  }

      } else if (unit_string == "CONV") {
        ConversionUnit* converter = new ConversionUnit(latency, issue_width);
        modules->push_back(converter);
        functional_units->push_back(converter);
        module_names->push_back(std::string("Conversion Unit"));

        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .001814;
	    unit_energy = .001;
	  }
	
      } else if (unit_string == "BLT") {
        BranchUnit* branch = new BranchUnit(latency, issue_width);
        modules->push_back(branch);
        functional_units->push_back(branch);
        module_names->push_back(std::string("Branch Unit"));

        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .00066;
	    unit_energy = .000572;
	  }

      } else if (unit_string == "BITWISE") {
        Bitwise* bitwise = new Bitwise(latency, issue_width);
        modules->push_back(bitwise);
        functional_units->push_back(bitwise);
        module_names->push_back(std::string("Bitwise Ops"));

        if (unit_area < 0 || unit_energy < 0) 
	  {
	    unit_area = .00066;
	    unit_energy = .000572;
	  }

      }
            
      else if (unit_string == "DEBUG") {
        DebugUnit* debug = new DebugUnit(latency);
        modules->push_back(debug);
        functional_units->push_back(debug);
        module_names->push_back(std::string("Debug Unit"));

	// Debug unit is free since it's not part of the real chip
        if (unit_area < 0 || unit_energy < 0) 	  
	  {
	    unit_area = 0;
	    unit_energy = 0;
	  }

      } else {
        printf("ERROR: Unknown/unhandled unit string %s\n", unit_string.c_str());
        continue;
      }

      size_estimate += unit_area * issue_width;
      functional_units->at(functional_units->size()-1)->area = unit_area;
      functional_units->at(functional_units->size()-1)->energy = unit_energy;

    }
  }

  fclose(input);
}


int ReadCacheParams(const char* file, int capacityBytes, int numBanks, int lineSizeBytes, float& area, float& energy, bool is_data_cache)
{
  FILE* input;
  input = fopen(file, "r");
  if(!input)
    {
      if(is_data_cache)
	perror("WARNING: Unable to find dcacheparams.txt (should be in samples/configs/).\n Use --dcacheparams <path to file> to specify the location of the file. Area and power may be wrong.\n");
      else
	perror("WARNING: Unable to find dcacheparams.txt (should be in samples/configs/).\n Use --icacheparams <path to file> to specify the location of the file. Area and power may be wrong.\n");

      area = 0;
      energy = 0;
      return 0;
    }
  
  char line[1000];
  
  while(!feof(input))
    {      
      if(!fgets(line, 1000, input))
	{
	  fclose(input);
	  area = 0;
	  energy = 0;
	  return 0;
	}

    if(strlen(line) < 1)
      continue;
    
    int capacity = 0;
    int banks = 0;
    int linesize = 0;
    float a = 0;
    float e = 0;


    if (sscanf(line, "%d\t%d\t%d\t%f\t%f", &capacity, &banks, &linesize, &a, &e) != 5) 
      continue;
      
    if(capacity == capacityBytes &&
       banks == numBanks &&
       linesize == lineSizeBytes)
      {
	area = a;
	energy = e;
	fclose(input);
	return 1;
      }
    }

  fclose(input);
  area = 0;
  energy = 0;
  return 0;
}
