#ifndef __SIMHWRT_DWARF_READER_H_
#define __SIMHWRT_DWARF_READER_H_
#include <map>
#include <string>
#include <vector>
#include "Assembler.h"

struct symbol;

struct AbbreviationCode
{
  int code;
  int addr;
};

struct Location
{
  enum LocType
    {
      UNDEFINED,
      REGISTER,
      FRAME_OFFSET,
      MEMBER_OFFSET
    };
  LocType type;
  int value;

Location() :
  type(UNDEFINED), value(-1){}

};

// This class represents any DWARF debugging entry (not just a DWARF "compilation unit")
class CompilationUnit
{
 public:
 CompilationUnit() :
  name(""), addr(-1), top_level_addr(-1), pointsTo(-1), num_instructions(0), 
    num_data_stalls(0), num_contention_stalls(0), typeUnit(NULL), typeRef(0),
    hasThisPointer(false){}
  
 CompilationUnit(std::string _name) :
  name(_name)
  {}
  /*
  void AddChild(CompilationUnit* c)
  {
    children.push_back(c);
  }
  */
  
  bool ContainsPC(int pc)
  {
    for(size_t i = 0; i < ranges.size(); i++)
      if(pc >= ranges[i].first && pc < ranges[i].second)
	return true;
    return false;
  }

  void SetParents();

  // Helpers for the Profiler/Debugger
  void AddInstructionContribution(Instruction* ins);
  void PrintUnitContribution(int indent_level, long long int total_thread_cycles);
  CompilationUnit* FindRelevantUnit(Instruction* ins);
  CompilationUnit* FindFunctionCall(Instruction* ins);
  CompilationUnit* SearchVariableDown(std::string varname);  
  CompilationUnit* GetTypeUnit();
  CompilationUnit* FindContainingFunction();
  CompilationUnit* GetVariable(std::string varname);

  std::string name;
  char tag;
  int addr;
  int top_level_addr;
  int pointsTo;
  int typeRef;
  int abbrev;
  int num_instructions; // These 3 counters are deprecated (now tracked in RuntimeNode)
  int num_data_stalls;
  int num_contention_stalls;
  bool hasThisPointer;
  Location loc;
  CompilationUnit* typeUnit;
  CompilationUnit* parent;
  std::vector<CompilationUnit*> children;
  std::vector<std::pair<int, int> > ranges;
};


// This class keeps track of the runtime debug information for a particular debug entry
class RuntimeNode
{
 public:
 RuntimeNode():
  source_node(NULL), parent(NULL), num_instructions(0), num_data_stalls(0), num_contention_stalls(0){}
  
  bool ContainsPC(int pc)
  {
    return source_node->ContainsPC(pc);
  }

  bool ContainsPCBelow( int PC )
  {
    for( size_t i = 0; i < children.size(); ++i )
    {
      if( children[i]->ContainsPC( PC ) )
	return true;
      if( children[i]->ContainsPCBelow( PC ) )
	return true;
    }
    return false;
  }

  void AddInstructionContribution(char stall_type);
  void DistributeTime();
  void Print(int indent_level, long long int total_thread_cycles, std::vector<Instruction*>& instructions);
  void WriteDot(const char* filename, long long int total_thread_cycles);
  void WriteDotRecursive(FILE* output, long long int total_thread_cycles);
  // Helpers for the Debugger
  void PrintBacktrace(int depth = 0);
  void UpdateExecutionPoint(SourceInfo info);

  CompilationUnit* source_node;
  RuntimeNode* parent;
  std::vector<RuntimeNode*> children;

  int num_instructions;
  int num_data_stalls;
  int num_contention_stalls;
  SourceInfo executionPoint;

  // Comparator for sort
  static bool compare(RuntimeNode* a, RuntimeNode* b)
  {
    return (a->num_instructions + a->num_data_stalls + a->num_contention_stalls) >
      (b->num_instructions + b->num_data_stalls + b->num_contention_stalls);
  }

};


class DwarfReader
{
 public:
  DwarfReader();
  
  bool BuildSourceTree(const std::vector<symbol*> labels, std::vector<symbol*> elf_vars, 
		       const std::vector<symbol*> data_table, const char* jump_table, 
		       int jtable_size, int debug_start, int debug_end, int abbrev_start);

  CompilationUnit* ReadCompilationUnit(const std::vector<symbol*> labels, std::vector<symbol*> elf_vars, 
				      const std::vector<symbol*> data_table, const char* jump_table, 
				      int jtable_size, int& current_addr, 
				      int debug_start, int debug_end, int abbrev_start, int top_unit_start,
				      int top_unit_pc);

  void MergeInto(CompilationUnit* from, CompilationUnit* into);

  bool ReadAttribute(CompilationUnit* retval, unsigned int attribute, unsigned int type, 
		     int debug_start, const char* jump_table, int& current_addr, 
		     const std::vector<symbol*> data_table, int& dtable_ptr,
		     int top_unit_pc);

  void ReadUnitHeader(int& unit_length, const char* jump_table, int& current_addr);

  void FindAbbrevCodes(const char* jump_table, int abbrev_start, int jtable_size);

  void AddUnitList(CompilationUnit* cu);

  void WriteDot(const char* filename);
  void WriteDotRecursive(FILE* output, CompilationUnit* node);

  RuntimeNode* UpdateRuntime(Instruction* ins, RuntimeNode* current_runtime, char stall_type = 0);
  static RuntimeNode* MostRelevantAncestor(Instruction* ins, RuntimeNode* current);
  static RuntimeNode* MostRelevantDescendant(Instruction* ins, RuntimeNode* current);

  CompilationUnit rootSource;
  RuntimeNode* rootRuntime;
  std::vector<AbbreviationCode>abbrevCodes;
  std::vector<CompilationUnit*> unit_list;
  std::map<int, CompilationUnit*> unitsByAddress;
};


// DWARF attribute codes
#define DW_AT_sibling 0x01
#define DW_AT_location 0x02
#define DW_AT_name 0x03
#define DW_AT_ordering 0x09
#define DW_AT_byte_size 0x0b
#define DW_AT_bit_offset 0x0c
#define DW_AT_bit_size 0x0d
#define DW_AT_stmt_list 0x10
#define DW_AT_low_pc 0x11
#define DW_AT_high_pc 0x12
#define DW_AT_language 0x13
#define DW_AT_discr 0x15
#define DW_AT_discr_value 0x16
#define DW_AT_visibility 0x17
#define DW_AT_import 0x18
#define DW_AT_string_length 0x19
#define DW_AT_common_reference 0x1a
#define DW_AT_comp_dir 0x1b
#define DW_AT_const_value 0x1c
#define DW_AT_containing_type 0x1d
#define DW_AT_default_value 0x1e
#define DW_AT_inline 0x20
#define DW_AT_is_optional 0x21
#define DW_AT_lower_bound 0x22
#define DW_AT_producer 0x25
#define DW_AT_prototyped 0x27
#define DW_AT_return_addr 0x2a
#define DW_AT_start_scope 0x2c
#define DW_AT_bit_stride 0x2e
#define DW_AT_upper_bound 0x2f
#define DW_AT_abstract_origin 0x31
#define DW_AT_accessibility 0x32
#define DW_AT_address_class 0x33
#define DW_AT_artificial 0x34
#define DW_AT_base_types 0x35
#define DW_AT_calling_convention 0x36
#define DW_AT_count 0x37
#define DW_AT_data_member_location 0x38
#define DW_AT_decl_column 0x39
#define DW_AT_decl_file 0x3a
#define DW_AT_decl_line 0x3b
#define DW_AT_declaration 0x3c
#define DW_AT_discr_list 0x3d
#define DW_AT_encoding 0x3e
#define DW_AT_external 0x3f
#define DW_AT_frame_base 0x40
#define DW_AT_friend 0x41
#define DW_AT_identifier_case 0x42
#define DW_AT_macro_info 0x43
#define DW_AT_namelist_item 0x44
#define DW_AT_priority 0x45
#define DW_AT_segment 0x46
#define DW_AT_specification 0x47
#define DW_AT_static_link 0x48
#define DW_AT_type 0x49
#define DW_AT_use_location 0x4a
#define DW_AT_variable_parameter 0x4b
#define DW_AT_virtuality 0x4c
#define DW_AT_vtable_elem_location 0x4d
#define DW_AT_allocated 0x4e
#define DW_AT_associated 0x4f
#define DW_AT_data_location 0x50
#define DW_AT_byte_stride 0x51
#define DW_AT_entry_pc 0x52
#define DW_AT_use_UTF8 0x53
#define DW_AT_extension 0x54
#define DW_AT_ranges 0x55
#define DW_AT_trampoline 0x56
#define DW_AT_call_column 0x57
#define DW_AT_call_file 0x58
#define DW_AT_call_line 0x59
#define DW_AT_description0x5a
#define DW_AT_binary_scale 0x5b
#define DW_AT_decimal_scale 0x5c
#define DW_AT_small 0x5d
#define DW_AT_decimal_sign 0x5e
#define DW_AT_digit_count 0x5f
#define DW_AT_picture_string 0x60
#define DW_AT_mutable 0x61
#define DW_AT_threads_scaled 0x62
#define DW_AT_explicit 0x63
#define DW_AT_object_pointer 0x64
#define DW_AT_endianity 0x65
#define DW_AT_elemental 0x66
#define DW_AT_pure 0x67
#define DW_AT_recursive 0x68
#define DW_AT_signature 0x69
#define DW_AT_main_subprogram 0x6a
#define DW_AT_data_bit_offset 0x6b
#define DW_AT_const_expr 0x6c
#define DW_AT_enum_class 0x6d
#define DW_AT_linkage_name 0x6e
#define DW_AT_MIPS_linkage_name 0x87
#define DW_AT_APPLE_optimized 0xe1
#define DW_AT_lo_user 0x2000
#define DW_AT_hi_user 0x3fff

// DWARF data formats
#define DW_FORM_addr 0x01
#define DW_FORM_block2 0x03
#define DW_FORM_block4 0x04
#define DW_FORM_data2 0x05
#define DW_FORM_data4 0x06
#define DW_FORM_data8 0x07
#define DW_FORM_string 0x08
#define DW_FORM_block 0x09
#define DW_FORM_block1 0x0a
#define DW_FORM_data1 0x0b
#define DW_FORM_flag 0x0c
#define DW_FORM_sdata 0x0d
#define DW_FORM_strp 0x0e
#define DW_FORM_udata 0x0f
#define DW_FORM_ref_addr 0x10
#define DW_FORM_ref1 0x11
#define DW_FORM_ref2 0x12
#define DW_FORM_ref4 0x13
#define DW_FORM_ref8 0x14
#define DW_FORM_ref_udata 0x15
#define DW_FORM_indirect 0x16
#define DW_FORM_sec_offset 0x17
#define DW_FORM_exprloc 0x18
#define DW_FORM_flag_present 0x19
#define DW_FORM_ref_sig8 0x20

// DWARF tags (only ones we care about for now)
#define DW_TAG_class_type 0x2
#define DW_TAG_formal_parameter 0x05
#define DW_TAG_lexical_block 0x0b
#define DW_TAG_member 0x0d
#define DW_TAG_pointer_type 0x0f
#define DW_TAG_reference_type 0x10
#define DW_TAG_variable 0x34
#define DW_TAG_compile_unit 0x11 
#define DW_TAG_base_type 0x24
#define DW_TAG_subprogram 0x2e


// DWARF opcodes
#define DW_OP_plus_uconst 0x23
#define DW_OP_reg0 0x50
#define DW_OP_reg1 0x51
#define DW_OP_reg2 0x52
#define DW_OP_reg3 0x53
#define DW_OP_reg4 0x54
#define DW_OP_reg5 0x55
#define DW_OP_reg6 0x56
#define DW_OP_reg7 0x57
#define DW_OP_reg8 0x58
#define DW_OP_reg9 0x59
#define DW_OP_reg10 0x5a
#define DW_OP_reg11 0x5b
#define DW_OP_reg12 0x5c
#define DW_OP_reg13 0x5d
#define DW_OP_reg14 0x5e
#define DW_OP_reg15 0x5f
#define DW_OP_reg16 0x60
#define DW_OP_reg17 0x61
#define DW_OP_reg18 0x62
#define DW_OP_reg19 0x63
#define DW_OP_reg20 0x64
#define DW_OP_reg21 0x65
#define DW_OP_reg22 0x66
#define DW_OP_reg23 0x67
#define DW_OP_reg24 0x68
#define DW_OP_reg25 0x69
#define DW_OP_reg26 0x6a
#define DW_OP_reg27 0x6b
#define DW_OP_reg28 0x6c
#define DW_OP_reg29 0x6d
#define DW_OP_reg30 0x6e
#define DW_OP_reg31 0x6f
#define DW_OP_regx 0x90 // offset from frame_base
#define DW_OP_fbreg 0x91 // offset from frame_base
#endif  //__SIMHWRT_DWARF_READER_H_
