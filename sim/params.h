#ifndef __PARAMS_H__
#define __PARAMS_H__

/********************/
/* Processor params */
/********************/
// number of cores in mulicore 
extern int NUMCORES;

// processor clock frequency multiplier : multiplying the
// DRAM_CLK_FREQUENCY by the following parameter gives the processor
// clock frequency 
extern int PROCESSOR_CLK_MULTIPLIER;

//size of ROB
 extern int ROBSIZE ;// 128;		

// maximum commit width
 extern int MAX_RETIRE ;// 2;

// maximum instruction fetch width
 extern int MAX_FETCH ;// 4;	

// depth of pipeline
 extern int PIPELINEDEPTH ;// 5;


/*****************************/
/* DRAM System Configuration */
/*****************************/
// total number of channels in the system
 extern int NUM_CHANNELS ;// 1;

// number of ranks per channel
 extern int NUM_RANKS ;// 2;

// number of banks per rank
 extern int NUM_BANKS ;// 8;

// number of rows per bank
 extern int NUM_ROWS ;// 32768;

// number of columns per rank
 extern int NUM_COLUMNS ;// 128;

// cache-line size (bytes)
 extern int CACHE_LINE_SIZE ;// 64;

// total number of address bits (i.e. indicates size of memory)
 extern int ADDRESS_BITS ;// 32;

/****************************/
/* DRAM Chip Specifications */
/****************************/

// dram frequency (not datarate) in MHz
 extern int DRAM_CLK_FREQUENCY ;// 800;

// All the following timing parameters should be 
// entered in the config file in terms of memory 
// clock cycles.

// RAS to CAS delay
 extern int T_RCD ;// 44;

// PRE to RAS
 extern int T_RP ;// 44;

// ColumnRD to Data burst
 extern int T_CAS ;// 44;

// RAS to PRE delay
 extern int T_RAS ;// 112;

// Row Cycle time
 extern int T_RC ;// 156;

// ColumnWR to Data burst
 extern int T_CWD ;// 20;

// write recovery time (COL_WR to PRE)
 extern int T_WR ;// 48;

// write to read turnaround
 extern int T_WTR ;// 24;

// rank to rank switching time
 extern int T_RTRS ;// 8;

// Data transfer
 extern int T_DATA_TRANS ;// 16;

// Read to PRE
 extern int T_RTP ;// 24;

// CAS to CAS
 extern int T_CCD ;// 16;

// Power UP time fast
 extern int T_XP ;// 20;

// Power UP time slow
 extern int T_XP_DLL ;// 40;

// Power down entry
 extern int T_CKE ;// 16;

// Minimum power down duration
 extern int T_PD_MIN ;// 16;

// rank to rank delay (ACTs to same rank)
 extern int T_RRD ;// 20;

// four bank activation window
 extern int T_FAW ;// 128;

// refresh extern interval
 extern int T_REFI;

 // refresh cycle time
 extern int T_RFC;

/****************************/
/* VOLTAGE & CURRENT VALUES */
/****************************/

extern float VDD;

extern float IDD0;

extern float IDD1;

extern float IDD2P0;

extern float IDD2P1;

extern float IDD2N;

extern float IDD3P;

extern float IDD3N;

extern float IDD4R;

extern float IDD4W;

extern float IDD5;

/******************************/
/* MEMORY CONTROLLER Settings */
/******************************/

// maximum capacity of write queue (per channel)
 extern int WQ_CAPACITY ;// 64;

//  int ADDRESS_MAPPING mode
// 1 is consecutive cache-lines to same row
// 2 is consecutive cache-lines striped across different banks 
 extern int ADDRESS_MAPPING ;// 1;

 // WQ associative lookup 
 extern int WQ_LOOKUP_LATENCY;


#endif // __PARAMS_H__

