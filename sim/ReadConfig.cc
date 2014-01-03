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

ReadConfig::ReadConfig(const char* input_file, L2Cache** L2s, size_t num_L2s, MainMemory*& mem,
		       double &size_estimate, bool disable_usimm, bool _memory_trace, bool _l1_off, bool _l2_off, bool _l1_read_copy) :
  input_file(input_file), memory_trace(_memory_trace), l1_off(_l1_off), l2_off(_l2_off), l1_read_copy(_l1_read_copy)
{
  FILE* input = fopen(input_file, "r");
  if (!input) {
    perror("Unable to open config file.");
    exit(1);
  }

  // assume input is of the form UNIT_TYPE args
  char line_buf[1024];
  while (!feof(input) && fgets(line_buf, sizeof(line_buf), input)) {
    char unit_type[100];
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
      float unit_power;

      int scanvalue = sscanf(line_buf, "%*s %d %d %d %d %f %f", &hit_latency,
			     &cache_size, &num_banks, &line_size, &unit_area, &unit_power);
      if ( scanvalue < 4 || scanvalue > 6 ) {
	printf("ERROR: L2 syntax is L2 <hit latency> <cache size> <num banks> <line size(**2)> <cache area um^2 (optional)> <power mW (optional)>\n");
	continue;
      }
      if (mem == NULL) {
	printf("ERROR: L2 declared before MEMORY!\n");
	continue;
      }

      // Insert loop to allocate num_L2s here
      for (size_t i = 0; i < num_L2s; ++i) {
	L2s[i] = new L2Cache(mem, cache_size, hit_latency,
			     disable_usimm, num_banks, line_size,
			     memory_trace, l2_off, l1_off);
      }

      // 5/3mm on a side for 8k cache
      //size_estimate += 25./9./8192.*cache_size;
      // 1761578 mm^2 for 2k 16byte ?
      //size_estimate += 1.761578/8192.*cache_size;
      // 65nm lp: 804063 mm^2 for 2k 16byte. Divide by 8192 to get area/word
      //size_estimate += 0.804063/8192.*cache_size;
      // L2 size removed to use Cacti numbers
      if (!l2_off) {
	size_estimate += unit_area*1e-6;
      }
    } else {
      // a per-core module line (skipped here)
    }
  }
  //printf(" Size estimate (L2):\t%.4lf\n", size_estimate);

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
      float unit_power = 0.0;
      
      int scanvalue = sscanf(line_buf, "%*s %d %d %d %d %f %f", &hit_latency,
			     &cache_size, &num_banks, &line_size, &unit_area, &unit_power);
      if ( scanvalue < 4 || scanvalue > 6) {
	printf("ERROR: L1 syntax is L1 <hit latency> <cache size> <num banks> <line size(**2)> <cache area um^2 (optional)> <power mW (optional)>\n");
	continue;
      }

      current_core->L1 = new L1Cache(L2, hit_latency, cache_size,
				     num_banks, line_size,
				     memory_trace, l1_off, l1_read_copy);
      
      modules->push_back(current_core->L1);
      // 5/3mm on a side for 8k cache
      //size_estimate += 25./9./8192.*cache_size;
      // 1761578 mm^2 for 2k 16byte ?
      //size_estimate += 1.761578/8192.*cache_size;
      // 65nm lp: 804063 mm^2 for 2k 16byte. Divide by 8192 to get area/word
      //size_estimate += 0.804063/8192.*cache_size;
      // L1 size removed to use Cacti numbers
      if (!l1_off) {
	size_estimate += unit_area*1.0e-6;
      }
    }     
    else {
      // a simple module taking latency and issue width
      int latency;
      int issue_width;
      float unit_area = -1;
      float unit_power = 0;

      int scanvalue = sscanf(line_buf, "%*s %d %d %f %f", &latency, &issue_width, &unit_area, &unit_power);
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
	
        if (unit_area < 0) {
	  // 0.2mm on a side
	  //size_estimate += .04 * issue_width;
	  // 142 um^2
	  //size_estimate += .000142 * issue_width;
	  // 65nm lp: 2596um^2
	  //size_estimate += .002596 * issue_width;
	  // 65nm sp: 3311um^2
	  size_estimate += .003311 * issue_width;
	}
      } else if (unit_string == "FPMIN") {
        FPMinMax* fp_minmax = new FPMinMax(latency, issue_width);
        modules->push_back(fp_minmax);
        functional_units->push_back(fp_minmax);
        module_names->push_back(std::string("FP MinMax"));

        if (unit_area < 0) {
	  // 0.18mm on a side
	  //size_estimate += .0324 * issue_width;
	  // 190 um^2
	  //size_estimate += .000190 * issue_width;
	  // 65nm lp: 690um^2 (same as compare)
	  //size_estimate += .000690 * issue_width;
	  // 65nm sp: 722um^2
	  size_estimate += .000722 * issue_width;
	}
      } else if (unit_string == "FPCMP") {
        FPCompare* fp_compare = new FPCompare(latency, issue_width);
        modules->push_back(fp_compare);
        functional_units->push_back(fp_compare);
        module_names->push_back(std::string("FP Compare"));
	
        if (unit_area < 0) {
	  // 0.18mm on a side
	  //size_estimate += .0324 * issue_width;
	  // 61 um^2
	  //size_estimate += .000061 * issue_width;
	  // 65nm lp: 690um^2
	  //size_estimate += .000690 * issue_width;
	  // 65nm sp: 722um^2
	  size_estimate += .000722 * issue_width;
	}
      } else if (unit_string == "INTADD") {
        IntAddSub* int_addsub = new IntAddSub(latency, issue_width);
        modules->push_back(int_addsub);
        functional_units->push_back(int_addsub);
        module_names->push_back(std::string("Int AddSub"));

        if (unit_area < 0) {
	  // 0.08mm on a side
	  //size_estimate += 0.0064 * issue_width;
	  // 58 um^2
	  //size_estimate += 0.000058 * issue_width;
	  // 65nm lp: 577um^2
	  //size_estimate += .000577 * issue_width;
	  // 65nm sp: 658um^2
	  size_estimate += .000658 * issue_width;
	}
      } else if (unit_string == "FPMUL") {
        FPMul* fp_mul = new FPMul(latency, issue_width);
        modules->push_back(fp_mul);
        functional_units->push_back(fp_mul);
        module_names->push_back(std::string("FP Mul"));
	
        if (unit_area < 0) {
	  // 0.4mm on a side
	  //size_estimate += 0.16 * issue_width;
	  // 226 um^2
	  //size_estimate += 0.000226 * issue_width;
	  // 65nm lp: 8980um^2
	  //size_estimate += .008980 * issue_width;
	  // 65nm sp: 10327um^2
	  size_estimate += .010327 * issue_width;
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

        if (unit_area < 0) {
	  // 0.8mm on a side
	  //size_estimate += 0.64 * issue_width;
	  // 452 um^2
	  //size_estimate += 0.000452 * issue_width;
	  // 65nm lp: 44465um^2
	  //size_estimate += .044465 * issue_width;
	  // 65nm sp: 112818um^2
	  size_estimate += .112818 * issue_width;
	}
      } else if (unit_string == "INTMUL") {
        IntMul* int_mul = new IntMul(latency, issue_width);
        modules->push_back(int_mul);
        functional_units->push_back(int_mul);
        module_names->push_back(std::string("Int Mul"));

        if (unit_area < 0) {
	  // 0.4mm on a side
	  //size_estimate += 0.16 * issue_width;
	  // 226 um^2
	  //size_estimate += 0.000226 * issue_width;
	  // 65nm lp: 12690um^2
	  //size_estimate += .012690 * issue_width;
	  // 65nm sp: 11694um^2
	  size_estimate += .011694 * issue_width;
	}
      } else if (unit_string == "CONV") {
        ConversionUnit* converter = new ConversionUnit(latency, issue_width);
        modules->push_back(converter);
        functional_units->push_back(converter);
        module_names->push_back(std::string("Conversion Unit"));
	// ignore size
      } else if (unit_string == "BLT") {
        BranchUnit* branch = new BranchUnit(latency, issue_width);
        modules->push_back(branch);
        functional_units->push_back(branch);
        module_names->push_back(std::string("Branch Unit"));

        if (unit_area < 0) {
	  // 0.18mm on a side
	  //size_estimate += .0324 * issue_width;
	  // 61 um^2
	  //size_estimate += 0.000061 * issue_width;
	  // 65nm lp: 690um^2 (same as compare)
	  //size_estimate += .000690 * issue_width;
	  // 65nm sp: 722um^2
	  size_estimate += .000722 * issue_width;
	}
      } else if (unit_string == "BITWISE") {
        Bitwise* bitwise = new Bitwise(latency, issue_width);
        modules->push_back(bitwise);
        functional_units->push_back(bitwise);
        module_names->push_back(std::string("Bitwise Ops"));

        if (unit_area < 0) {
	  // 0.05mm on a side
	  size_estimate += .000722 * issue_width;
	  // didn't have better numbers, chose the size of the compare.
	}
      }
            
      else if (unit_string == "DEBUG") {
        DebugUnit* debug = new DebugUnit(latency);
        modules->push_back(debug);
        functional_units->push_back(debug);
        module_names->push_back(std::string("Debug Unit"));
	// ignore size
      } else {
        printf("ERROR: Unknown/unhandled unit string %s\n", unit_string.c_str());
        continue;
      }


    }
  }
  
  //printf(" Size estimate (HW config):\t%.4lf\n", size_estimate);


  fclose(input);
}
