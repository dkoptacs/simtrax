#ifndef __MEMORY_CONTROLLER_H__
#define __MEMORY_CONTROLLER_H__

#include "ThreadState.h"
#include "L1Cache.h"
#include "L2Cache.h"
#include "SimpleRegisterFile.h"
#include "Instruction.h"
#include <vector>

#define MAX_QUEUE_LENGTH 80

#define MAX_NUM_CHANNELS 16
#define MAX_NUM_RANKS 16
#define MAX_NUM_BANKS 32

#define BIG_ACTIVATION_WINDOW 1000000

// Moved here from main.c 
extern long long int *committed; // total committed instructions in each core
extern long long int *fetched;   // total fetched instructions in each core

extern int max_write_queue_length[MAX_NUM_CHANNELS];
extern int max_read_queue_length[MAX_NUM_CHANNELS];
extern long long int accumulated_read_queue_length[MAX_NUM_CHANNELS];

extern long long int update_mem_count;

//////////////////////////////////////////////////
//	Memory Controller Data Structures	//
//////////////////////////////////////////////////

// DRAM Address Structure
typedef struct draddr
{
  long long int actual_address; // physical_address being accessed
  int channel;	// channel id
  int rank;	// rank id
  int bank;	// bank id
  long long int row;	// row/page id
  int column;	// column id
} dram_address_t;

// DRAM Commands 
typedef enum {ACT_CMD, COL_READ_CMD, PRE_CMD, COL_WRITE_CMD, PWR_DN_SLOW_CMD, PWR_DN_FAST_CMD, PWR_UP_CMD, REF_CMD, NOP} command_t; 

// Request Types
typedef enum {READ, WRITE} optype_t;

// Single request structure self-explanatory


typedef struct trax_req
{
  reg_value result; 
  int which_reg;
  int trax_addr;
  ThreadState* thread;
  L1Cache* L1;
  L2Cache* L2;
} trax_request;

//  std::vector<int> testvec;

//DK: update this struct to hold the PC of issuing instruction
typedef struct req
{
  unsigned long long int physical_address;
  dram_address_t dram_addr;
  long long int arrival_time;     
  long long int dispatch_time; // when COL_RD or COL_WR is issued for this request
  long long int completion_time; //final completion time
  long long int latency; // dispatch_time-arrival_time
  int thread_id; // core that issued this request
  command_t next_command; // what command needs to be issued to make forward progress with this request
  int command_issuable; // can this request be issued in the current cycle
  optype_t operation_type; // Read/Write
  int request_served; // if request has it's final command issued or not
  int instruction_id; // 0 to ROBSIZE-1
  long long int instruction_pc; // phy address of instruction that generated this request (valid only for reads)

  //TRaX stuff
  std::vector<trax_request> trax_reqs;

  // The below all got moved in to a vector
  //int which_reg;
  //ThreadState* thread;
  //L1Cache* L1;
  //L2Cache* L2;
  //reg_value result;

  // TODO: I don't think we need this opcode
  Instruction::Opcode op;


  void * user_ptr; // user_specified data
  struct req * next;
} request_t;

// Bankstates
typedef enum 
{
  IDLE, PRECHARGING, REFRESHING, ROW_ACTIVE, PRECHARGE_POWER_DOWN_FAST, PRECHARGE_POWER_DOWN_SLOW, ACTIVE_POWER_DOWN
} bankstate_t;

// Structure to hold the state of a bank
typedef struct bnk
{
  bankstate_t state;
  long long int active_row;
  long long int next_pre;
  long long int next_act;
  long long int next_read;
  long long int next_write;
  long long int next_powerdown;
  long long int next_powerup;
  long long int next_refresh;
}bank_t;

extern struct robstructure * ROB;

extern int activation_record[MAX_NUM_CHANNELS][MAX_NUM_RANKS][BIG_ACTIVATION_WINDOW];

// contains the states of all banks in the system 
extern bank_t dram_state[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];

extern long long int total_col_reads[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int total_pre_cmds[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int total_single_col_reads[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int current_col_reads[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];

//[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
// total number of col reads
// total number of pre cmds
// total number of single col reads

// command issued this cycle to this channel
extern int command_issued_current_cycle[MAX_NUM_CHANNELS];

// cas command issued this cycle to this channel
extern int cas_issued_current_cycle[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS]; // 1/2 for COL_READ/COL_WRITE

// Per channel read queue
extern request_t * read_queue_head[MAX_NUM_CHANNELS];

// Per channel write queue
extern request_t * write_queue_head[MAX_NUM_CHANNELS];

// issuables_for_different commands
extern int cmd_precharge_issuable[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern int cmd_all_bank_precharge_issuable[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int cmd_powerdown_fast_issuable[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int cmd_powerdown_slow_issuable[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int cmd_powerup_issuable[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int cmd_refresh_issuable[MAX_NUM_CHANNELS][MAX_NUM_RANKS];


// refresh variables
extern long long int next_refresh_completion_deadline[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int last_refresh_completion_deadline[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int forced_refresh_mode_on[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int refresh_issue_deadline[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int issued_forced_refresh_commands[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern int num_issued_refreshes[MAX_NUM_CHANNELS][MAX_NUM_RANKS];

extern long long int read_queue_length[MAX_NUM_CHANNELS];
extern long long int write_queue_length[MAX_NUM_CHANNELS];

// Stats
extern long long int num_read_merge ;
extern long long int num_write_merge ;
extern long long int stats_reads_merged_per_channel[MAX_NUM_CHANNELS];
extern long long int stats_writes_merged_per_channel[MAX_NUM_CHANNELS];
extern long long int stats_reads_seen[MAX_NUM_CHANNELS];
extern long long int stats_writes_seen[MAX_NUM_CHANNELS];
extern long long int stats_reads_completed[MAX_NUM_CHANNELS];
extern long long int stats_writes_completed[MAX_NUM_CHANNELS];

extern double stats_average_read_latency[MAX_NUM_CHANNELS];
extern double stats_average_read_queue_latency[MAX_NUM_CHANNELS];
extern double stats_average_write_latency[MAX_NUM_CHANNELS];
extern double stats_average_write_queue_latency[MAX_NUM_CHANNELS];

extern long long int stats_page_hits[MAX_NUM_CHANNELS];
extern double stats_read_row_hit_rate[MAX_NUM_CHANNELS];

// Time spent in various states
extern long long int stats_time_spent_in_active_standby[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_time_spent_in_active_power_down[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_time_spent_in_precharge_power_down_fast[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_time_spent_in_precharge_power_down_slow[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_time_spent_in_power_up[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int last_activate[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int last_refresh[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern double average_gap_between_activates[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern double average_gap_between_refreshes[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_time_spent_terminating_reads_from_other_ranks[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_time_spent_terminating_writes_to_other_ranks[MAX_NUM_CHANNELS][MAX_NUM_RANKS];

// Command Counters
extern long long int stats_num_activate_read[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int stats_num_activate_write[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int stats_num_activate_spec[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int stats_num_activate[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_num_precharge[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int stats_num_read[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int stats_num_write[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];
extern long long int stats_num_powerdown_slow[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_num_powerdown_fast[MAX_NUM_CHANNELS][MAX_NUM_RANKS];
extern long long int stats_num_powerup[MAX_NUM_CHANNELS][MAX_NUM_RANKS];



// functions

// to get log with base 2
unsigned int log_base2(unsigned int new_value);

// initialize memory_controller variables
void init_memory_controller_vars();

// called every cycle to update the read/write queues
void update_memory();

// activate to bank allowed or not
int is_activate_allowed(int channel, int rank, int bank);

// precharge to bank allowed or not
int is_precharge_allowed(int channel, int rank, int bank);

// all bank precharge allowed or not
int is_all_bank_precharge_allowed(int channel, int rank);

// autoprecharge allowed or not
int is_autoprecharge_allowed(int channel,int rank,int bank);

// power_down fast allowed or not
int is_powerdown_fast_allowed(int channel,int rank);

// power_down slow allowed or not
int is_powerdown_slow_allowed(int channel,int rank);

// powerup allowed or not
int is_powerup_allowed(int channel,int rank);

// refresh allowed or not
int is_refresh_allowed(int channel,int rank);


// issues command to make progress on a request
int issue_request_command(request_t * req);

// power_down command
int issue_powerdown_command(int channel, int rank, command_t cmd);

// powerup command
int issue_powerup_command(int channel, int rank);

// precharge a bank
int issue_activate_command(int channel, int rank, int bank, long long int row);

// precharge a bank
int issue_precharge_command(int channel, int rank, int bank);

// precharge all banks in a rank
int issue_all_bank_precharge_command(int channel, int rank);

// refresh all banks
int issue_refresh_command(int channel, int rank);

// autoprecharge all banks
int issue_autoprecharge(int channel, int rank, int bank);

// find if there is a matching write request
int read_matches_write_or_read_queue(long long int physical_address, request_t*& existing_request);

// find if there is a matching request in the write queue
int write_exists_in_write_queue(long long int physical_address);

// enqueue a read into the corresponding read queue (returns ptr to new node)
request_t* insert_read(const dram_address_t &physical_address, int trax_address, long long int arrival_cycle, int thread_id, int instruction_id, long long int instruction_pc, 
		       int which_reg, reg_value result, Instruction::Opcode op, ThreadState* thread, L1Cache* L1, L2Cache* L2);

// enqueue a write into the corresponding write queue (returns ptr to new_node)
request_t* insert_write(const dram_address_t &physical_address, int trax_address, long long int arrival_time, int thread_id, int instruction_id, long long int instruction_pc,
			int which_reg, reg_value result, Instruction::Opcode op, ThreadState* thread, L1Cache* L1, L2Cache* L2);

dram_address_t calcDramAddr(long long int physical_address);

// update stats counters
void gather_stats(int channel);

// print statistics
extern void print_stats();

// calculate power for each channel
float calculate_power(int channel, int rank, int print_stats_type, int chips_per_rank);
#endif // __MEM_CONTROLLER_HH__
