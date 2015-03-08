#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <queue>
#include "Profiler.h"
#include "Assembler.h"


// Helpers
unsigned char readByte(const char* jump_table, int& addr);
uint16_t readHalf(const char* jump_table, int& addr);
uint32_t readWord(const char* jump_table, int& addr);
unsigned int readVarSize(const char* jump_table, int& addr, int size);
unsigned int readVarType(const char* jump_table, int& addr, int type);

Profiler::Profiler()
{
  rootRuntime = NULL;
}


// Using debug symbols, create a tree representation of the source info with program counter ranges
bool Profiler::BuildSourceTree(const std::vector<symbol*> labels, std::vector<symbol*> elf_vars,
			       const std::vector<symbol*> data_table, const char* jump_table, 
			       int jtable_size, int debug_start, int abbrev_start)
{
  int unit_ptr = debug_start;

  // Find the addresses of each DWARF abbreviation unit
  FindAbbrevCodes(jump_table, abbrev_start, jtable_size);
  
  // Read each compilation unit until done
  while(unit_ptr < abbrev_start)
    {
      // Read fixed header data
      int unit_length;
      int start_unit = unit_ptr;
      ReadUnitHeader(unit_length, jump_table, unit_ptr);

      // Read the rest of the unit data
      rootSource.children.push_back(ReadCompilationUnit(labels, elf_vars, data_table, 
							jump_table, jtable_size, unit_ptr, 
							debug_start, abbrev_start,
							start_unit, 0));
    }

  // Combine all units and their children in to one list
  for(size_t i = 0; i < rootSource.children.size(); i++)
    AddUnitList(&(rootSource.children[i]));

  // Update names of reference units (DW_AT_specification, DW_AT_abstract_origin)
  // N^2 algorithm, but who cares, N is small
  // Meanwhile, find "main"
  for(size_t i = 0; i < unit_list.size(); i++)
    {
      if(unit_list[i]->name.compare(std::string("main")) == 0)
	{
	  if(unit_list[i]->ranges.size() > 0)
	    {
	      rootRuntime = new RuntimeNode();
	      rootRuntime->source_node = (unit_list[i]);
	    }
	}
	
      // Check if the unit has a reference
      if(unit_list[i]->pointsTo >= 0)
	{
	  for(size_t j = 0; j < unit_list.size(); j++)
	    if(unit_list[j]->addr == unit_list[i]->pointsTo)
	      {
		unit_list[i]->name = unit_list[j]->name;
	      }
	}
    }

  if(rootRuntime == NULL)
    {
      printf("ERROR: (--profile): Found no \"main\" routine. Did you compile your TRaX project with -g?\n");
      exit(1);
    }
  
  // For debugging. May be useful
  // Writes a graphviz file representing the source tree
  WriteDot("sourcetree.dot");

  return true;
 
 }
 
 
// Read one debugging information entry and its children
// Note that these are entries and not what DWARF technically calls "compilation units"
// DWARF compilation units are considered a debugging entry here
// jump_table is the program's raw data segment
// data_table is the "symbol" representation of the data segment (gives info about each entry)
CompilationUnit Profiler::ReadCompilationUnit(const std::vector<symbol*> labels, std::vector<symbol*> elf_vars, 
					      const std::vector<symbol*> data_table, const char* jump_table, 
					      int jtable_size, int& current_addr, 
					      int debug_start, int abbrev_start, int top_unit_start,
					      int top_unit_pc)
{
  
  if(current_addr < debug_start || current_addr >= abbrev_start)
    {
      printf("ERROR (--profile): Invalid profile data. Did you compile your TRaX project with -g?\n");
      exit(1);
    }

  CompilationUnit retval;

  retval.addr = current_addr;
  retval.top_level_addr = top_unit_start;

  // The unit's abbreviation code is always the first byte
  int abbrev_code = readByte(jump_table, current_addr);
  retval.abbrev = abbrev_code;

  // End of children marker
  if(abbrev_code == 0)
    return retval;
    
  // Find this unit's address in the vector of symbol objects created by the assembler.
  // This gives us more info about the jump_table (like the size of each entry)
  int dtable_ptr = -1;
  for(size_t i=0; i < data_table.size(); i++)
    if(current_addr == data_table[i]->address)
      {
	dtable_ptr = i;
	break;
      }

  // Make sure we found it
  if(dtable_ptr < 0)
    {
      printf("ERROR (--profile): Invalid profile data (did you compile your TRaX project with -g?)\n");
      exit(1);
    }

  // Find the DWARF abbreviation table entry
  int abbrev_addr = -1;
  for(size_t i=0; i < abbrevCodes.size(); i++)
    if(abbrevCodes[i].code == abbrev_code)
      {
	abbrev_addr = abbrevCodes[i].addr;
	break;
      }

  // Make sure we found it
  if(abbrev_addr < 0)
    {
      printf("ERROR (--profile): Invalid profile data (did you compile your TRaX project with -g?)\n");
      exit(1);
    }

  // Consume the abbrev code itself
  readByte(jump_table, abbrev_addr);
  // Tag is always next
  retval.tag = readByte(jump_table, abbrev_addr);
  // Children is next
  bool has_children = readByte(jump_table, abbrev_addr);

  // Read attribute/type pairs one at a time
  while(true)
    {
      // Read attribute code
      unsigned int attribute = readByte(jump_table, abbrev_addr);
      if(attribute == DW_AT_MIPS_linkage_name || attribute == DW_AT_APPLE_optimized)
	{
	  // These attributes are larger than others
	  // Throw away the extra byte associated with these
	  readByte(jump_table, abbrev_addr); 
	}
      // Read type code
      unsigned int type = readByte(jump_table, abbrev_addr);
      if(attribute == 0 && type == 0) // EOM marker
	break;

      // Read and parse the attribute data
      if(!ReadAttribute(retval, attribute, type, debug_start, jump_table, current_addr, data_table, dtable_ptr, top_unit_pc))
	exit(1);

    }
  
  // If this is the top-level unit corresponding to an object file (a compilation unit)
  // Its children may use its base address as an offset
  if(retval.tag == DW_TAG_compile_unit)
    {
      if(retval.ranges.size() == 1)
	top_unit_pc = retval.ranges[0].first;
      else
	top_unit_pc = 0;
    }

  // Recursively read this unit's children
  if(has_children)
    {
      while(true)
	{
	  CompilationUnit child = ReadCompilationUnit(labels, elf_vars, data_table, 
						      jump_table, jtable_size, current_addr, 
						      debug_start, abbrev_start, top_unit_start,
						      top_unit_pc);
	  // Propogate a class unit's name down to its children
	  if(retval.tag == DW_TAG_class_type && retval.name.size() > 0)
	    if(child.name.size() > 0)
	      child.name = retval.name + "::" + child.name;
	  
	  // End of children
	  if(child.abbrev == 0)
	    break;

	  retval.children.push_back(child);
	  
	}
    }

  return retval;  
}


// Reads and parses a DWARF attribute and saves any relevant data that our profiler cares about
bool Profiler::ReadAttribute(CompilationUnit& retval, unsigned int attribute, unsigned int type, 
			     int debug_start, const char* jump_table, int& current_addr, 
			     const std::vector<symbol*> data_table, int& dtable_ptr,
			     int top_unit_pc)
{
  int value;
  int rangePtr = -1;
  int low_pc = -1;
  int high_pc = -1;
  bool unused_attribute = false;
  int tmpAddr;
  switch(attribute)
    {
    case DW_AT_name:
      value = readWord(jump_table, current_addr);
      retval.name = std::string((const char*)&(jump_table[value]));
      break;
    case DW_AT_APPLE_optimized:
      // These appear in the abbrev table but have no corresponding item in the debug entry.
      return true;
      break;
    case DW_AT_low_pc: // low program coutner (creates a pair)
      retval.ranges.push_back(std::pair<int, int>(readWord(jump_table, current_addr), -1));
      break;
    case DW_AT_high_pc: // high PC (requires low to be read first)
      if((retval.ranges.size() < 1) || 
	 (retval.ranges[retval.ranges.size()-1].first == -1) ||
	 (retval.ranges[retval.ranges.size()-1].second != -1))
	{
	  printf("ERROR (--profile): Invalid or inverted PC ranges\n");
	  exit(1);
	}
      retval.ranges[retval.ranges.size()-1].second = readWord(jump_table, current_addr);

      // If the type of high_pc is not a raw address, then it's an offset from low_pc
      if(type != DW_FORM_addr)
	retval.ranges[retval.ranges.size()-1].second += retval.ranges[retval.ranges.size()-1].first;
      break;

    case DW_AT_ranges: // a non-contiguous range of PCs
      rangePtr = readWord(jump_table, current_addr);
      for(size_t i = 0; i < data_table.size(); i++) // find the symbol for the start of the ranges
	if(data_table[i]->address == rangePtr)
	  {
	    rangePtr = i; // repurpose as an index in to the symbol vector now.
	    break;
	  }
      if(rangePtr == -1)
	{
	  printf("ERROR (--profile): Can't find debug ranges symbol\n");
	  exit(1);
	}
      while(true)// load all range pairs
	{
	  if((unsigned)rangePtr > (data_table.size()-2))
	    {
	      printf("ERROR (--profile): invalid debug ranges (DW_AT_rages)\n");
	      exit(1);
	    }
	  // Read low pc
	  tmpAddr = data_table[rangePtr + 0]->address;
	  low_pc = readVarSize(jump_table, tmpAddr, data_table[rangePtr + 0]->size);
	  // Read high pc
	  tmpAddr = data_table[rangePtr + 1]->address;
	  high_pc = readVarSize(jump_table, tmpAddr, data_table[rangePtr + 1]->size);
	  // EOM
	  if(low_pc == 0 && high_pc == 0)
	    break;
	  low_pc += top_unit_pc; // ranges are offsets from the containing compilation unit
	  high_pc += top_unit_pc;
	  retval.ranges.push_back(std::pair<int, int>(low_pc, high_pc));
	  rangePtr+=2;
	}
      break;
    case DW_AT_specification: // These two attributes indicate the unit references another for information
    case DW_AT_abstract_origin:
      if(type == DW_FORM_ref_addr)
	retval.pointsTo = readVarType(jump_table, current_addr, type) + debug_start;
      else
	retval.pointsTo = readVarType(jump_table, current_addr, type) + retval.top_level_addr;
      break;
    default:
      // Something our profiler doesn't care about
      unused_attribute = true;
      break;
    }

  // Advance/backtrack current_addr past this attribute if it contains any extra data
  // or back if it contained no data
  switch(type)
    {
    case DW_FORM_exprloc: // Variable-sized attributes
    case DW_FORM_block:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
      value = readVarSize(jump_table, current_addr, data_table[dtable_ptr]->size);
      current_addr += value;
      dtable_ptr += value;
      break;
    case DW_FORM_flag_present:
      // Flags in the abbrev table have no corresponding entry in the debug unit, don't advance current_addr
      dtable_ptr--; // undo the increment below
      break;
    default:
      if(unused_attribute) // Move past the unused data
	current_addr += data_table[dtable_ptr]->size;
      break;
    }

  dtable_ptr++;

  return true;

}

// Reads the header information for a DWARF compilation unit
void Profiler::ReadUnitHeader(int& unit_length, const char* jump_table, int& current_addr)
{
  unit_length = readWord(jump_table, current_addr);

  if(readHalf(jump_table, current_addr) != 4) // We need DWARF version 4
    {
      printf("ERROR (--profile): Debug data compiled with wrong DWARF version number. Can't build profiler.\n");
      exit(1);
    }

  readWord(jump_table, current_addr); // throw away abbrev offset

  if(readByte(jump_table, current_addr) != 4) // We need 32-bit addresses
    {
      printf("ERROR (--profile): Debug data contains code compiled for non-TRaX architecture (wrong address size). Can't build profiler.\n");
      exit(1);
    }
}

// Preprocess step to find the locations of each entry in the DWARF abbreviations table
void Profiler::FindAbbrevCodes(const char* jump_table, int abbrev_start, int jtable_size)
{
  int addr = abbrev_start;
  AbbreviationCode ac;
  while(addr < jtable_size - 2) // this loop reads 2 at a time, must stop early to be safe
    {
      ac.addr = addr;
      ac.code = readByte(jump_table, addr);

      if(ac.code == 0) // EOM marker
	return;
      
      abbrevCodes.push_back(ac);
      
      // consume the tag and has_children (so they don't count as zeros for EOM detection)
      readByte(jump_table, addr);
      readByte(jump_table, addr);

      bool read_zero = false;
      while(true)
	{
	  if(addr >= jtable_size)
	    {
	      printf("ERROR (--profile): Invalid profile data (did you compile your TRaX project with -g?)\n");
	      exit(1);
	    }
	  if(readByte(jump_table, addr) == 0)
	    {
	      if(read_zero)
		break;
	      read_zero = true;
	    }
	  else
	    read_zero = false;
	}
    }
}


// Recursively combine a compilation unit and its children in to one list (the unit_list)
void Profiler::AddUnitList(CompilationUnit* cu)
{
  unit_list.push_back(cu);
  for(size_t i=0; i < cu->children.size(); i++)
    AddUnitList((&cu->children[i]));
}


// Accumulate all units' runtime in to their parents (a child's runtime adds to its parent's runtime)
// Also remove unnamed children (such as lexical blocks), by promoting their children to a direct child of the parent
void RuntimeNode::DistributeTime()
{


  std::queue<RuntimeNode*> Q;
  for(size_t i = 0; i < children.size(); i++)
    Q.push(children[i]);
  
  // We need a queue because a node's children may grow due to promotion of grandchildren
  while(!Q.empty())
    {
      RuntimeNode* child = Q.front();
      Q.pop();
      // Promote unnamed child's children to a direct child of this
      // Since unnamed children affect the order of children after sorting in Print(), 
      // The unnamed child's children would be printed out of order, since it's depth first
      if(child->source_node->name.compare("") == 0)
	{
	  for(size_t i = 0; i < child->children.size(); i++)
	    {
	      Q.push(child->children[i]);  
	      children.push_back(child->children[i]);
	    }
	  // Erase the unamed child from this
	  for(std::vector<RuntimeNode*>::iterator it=children.begin(); it!=children.end(); it++)
	    if((*it) == child)
	      {
		children.erase(it);
		break;
	      }
	}
      else
	child->DistributeTime();

      num_instructions += child->num_instructions;
      num_data_stalls += child->num_data_stalls;
      num_contention_stalls += child->num_contention_stalls;
    }

  // Or don't promote children (if we ever want to keep lexical blocks)
  /*
  for(size_t i = 0; i < children.size(); i++)
    {
      children[i]->DistributeTime();
      num_instructions += children[i]->num_instructions;
      num_data_stalls += children[i]->num_data_stalls;
      num_contention_stalls += children[i]->num_contention_stalls;
    }
  */

}

// Recursively print a single debug entry's profile information
void RuntimeNode::Print(int indent_level, long long int total_thread_cycles)
{
  if(num_instructions > 0)
    {
      if((source_node->tag != DW_TAG_compile_unit) && (source_node->name.compare(std::string(""))))
	{
	  for(int i=0; i < indent_level; i++) // print the depth separator
	    printf("| ");
	  printf("%s\t%2.2f\n", source_node->name.c_str(),
		 100.f * (float)(num_instructions + num_data_stalls + num_contention_stalls) / (float)total_thread_cycles);

	  // If we want to print the time consumed by separate stall types instead:
	  //printf("%s\t%2.2f (%2.2f) (%2.2f)\n", source_node->name.c_str(), 
	  //	 100.f * (float)(num_instructions + num_data_stalls + num_contention_stalls) / (float)total_thread_cycles,
	  //	 100.f * (float)(num_data_stalls) / (float)total_thread_cycles,
	  //	 100.f * (float)(num_contention_stalls) / (float)total_thread_cycles);
	  indent_level++;
	}

      // Sort by time contribution
      std::sort(children.begin(), children.end(), RuntimeNode::compare);

      for(size_t i = 0; i < children.size(); i++)
	children[i]->Print(indent_level, total_thread_cycles);
    }
}


// Prints the full runtime profile information, regardless of how insignificant a unit's contribution was
void Profiler::PrintProfile(std::vector<Instruction*>& instructions, long long int total_thread_cycles)
{

  rootRuntime->DistributeTime();
  rootRuntime->Print(0, total_thread_cycles);

  // Write out a graphviz file representing the profile information
  rootRuntime->WriteDot("runtime.dot", total_thread_cycles);

}


// Finds a function call (when the instruction's program counter is not within the previous unit)
CompilationUnit* CompilationUnit::FindFunctionCall(Instruction* ins)
{
  if(ContainsPC(ins->pc_address))
    {
      if(tag != DW_TAG_compile_unit)
	return this;
      else
	{
	  for(size_t i = 0; i < children.size(); i++)
	    {
	      CompilationUnit* cu = children[i].FindFunctionCall(ins);
	      if(cu != NULL)
		return cu;
	    }
	}
    }
  return NULL;
}


// Traverse up the tree from the current unit until any unit containing the instruction
RuntimeNode* Profiler::MostRelevantAncestor(Instruction* ins, RuntimeNode* current)
{

  if(current == NULL)
    return NULL;
  
  // Compile units aren't functions, don't want them acting as contributors to the profile
  if(current->ContainsPC(ins->pc_address) && (current->source_node->tag != DW_TAG_compile_unit))
    return current;
  
  if(current->parent == NULL)
    return NULL;
    
  return MostRelevantAncestor(ins, current->parent);
}


// Finds the most specific unit containing the instruction
// Assumes that current's range contains the instruction's PC
// Creates new RuntimeNodes if needed to match the most relevant CompilationUnit
RuntimeNode* Profiler::MostRelevantDescendant(Instruction* ins, RuntimeNode* current)
{
  for(size_t i = 0; i < current->children.size(); i++)
    if(current->children[i]->ContainsPC(ins->pc_address))
      return MostRelevantDescendant(ins, current->children[i]);

  // If no existing runtime node, find the source tree node that contains the PC, and make a new runtime node
  for(size_t i = 0; i < current->source_node->children.size(); i++)
    {
      if(current->source_node->children[i].ContainsPC(ins->pc_address))
	{
	  RuntimeNode* rn = new RuntimeNode();
	  rn->source_node = &(current->source_node->children[i]);
	  rn->parent = current;
	  current->children.push_back(rn);
	  return MostRelevantDescendant(ins, rn);
	}
    }
  return current;
}


// Increments contribution based on stall type
void RuntimeNode::AddInstructionContribution(char stall_type)
{
  switch(stall_type)
    {
    case STALL_EXECUTE:
      num_instructions++;
      break;
    case STALL_DATA:
      num_data_stalls++;
      break;
    case STALL_CONTENTION:
      num_contention_stalls++;
      break;
    }
}


// Updates and tracks the runtime profile for a single runtime cycle - invoked by the IssueUnit
// Must be invoked at simulator real time to track the call stack
RuntimeNode* Profiler::UpdateRuntime(Instruction* ins, RuntimeNode* current_runtime, char stall_type)
{

  // Can only happen on the first instruction
  // Set the runtime node to "main"
  if(current_runtime == NULL)
    {
      rootRuntime->AddInstructionContribution(stall_type);
      return rootRuntime;
    }

  // Find the most relevant runtime node for the instruction
  RuntimeNode* mostRelevant = MostRelevantAncestor(ins, current_runtime);
  // If this found NULL, either we are still in the preamble, or we found a non-inlined function call
  if(mostRelevant == NULL)
    {
      // Check if the function has already been called within this node (possibly by another thread)
      mostRelevant = MostRelevantDescendant(ins, current_runtime);
      if(mostRelevant != current_runtime)
	{
	  mostRelevant->AddInstructionContribution(stall_type);
	  return mostRelevant;
	}
      
      // Find the function call
      CompilationUnit* cu = NULL;
      for(size_t i = 0; i < rootSource.children.size(); i++)
	{
	  cu = rootSource.children[i].FindFunctionCall(ins);
	  if(cu != NULL)
	    break;
	}
      
      // Found no unit containing the instruction (this can happen in the preamble for example)
      // Just put it in to "main"
      if(cu == NULL)
	{
	  mostRelevant = rootRuntime;
	}
      else // otherwise we found a function call from a different debug unit
	{
	  mostRelevant = new RuntimeNode();
	  mostRelevant->parent = current_runtime;
	  mostRelevant->source_node = cu;
	  current_runtime->children.push_back(mostRelevant);
	  // Descend to the most specific unit within the new unit
	  mostRelevant = MostRelevantDescendant(ins, mostRelevant);
	}
    }
  else
    {
      // Find the most specific child, will also create new nodes as needed
      mostRelevant = MostRelevantDescendant(ins, mostRelevant);
    }
  
  // Now update the cycle count for that node
  mostRelevant->AddInstructionContribution(stall_type);
  return mostRelevant;
}


// Writes a graphviz file representing the debug source tree
void Profiler::WriteDot(const char* filename)
{
  
  FILE *output = fopen(filename, "w");
  if(output == NULL)
    {
      printf("Error (--profile), could not open DOT file for writing: %s\n", filename);
      exit(1);
    }

   fprintf(output, "graph SourceTree {\n");

   for(size_t i = 0; i < rootSource.children.size(); i++)
     {
       WriteDotRecursive(output, rootSource.children[i]);
     }

   fprintf(output, "}\n");
   fclose(output);
}

// Recursive helper for the above
void Profiler::WriteDotRecursive(FILE* output, CompilationUnit node)
{
 
  static int nodeID = -1;
  int myID = ++nodeID;
  fprintf(output, "Node%d [label=\"%s\\nt=%d\\nr=", myID, node.name.c_str(), node.tag);
  for(size_t i=0; i < node.ranges.size(); i++)
    fprintf(output, "%d-%d, ", node.ranges[i].first, node.ranges[i].second);
  fprintf(output, "\\na=%d\\nr=%d\\nbrev=%d\"];\n", node.addr, node.pointsTo, node.abbrev);
  
  for(size_t i=0; i < node.children.size(); i++)
    {
      volatile int childID = nodeID + 1;
      WriteDotRecursive(output, node.children[i]);
      fprintf(output, "Node%d -- Node%d;\n", myID, childID);
    }
}


// Writes a graphviz file representing the runtime profile information
void RuntimeNode::WriteDot(const char* filename, long long int total_thread_cycles)
{
    
  FILE *output = fopen(filename, "w");
  if(output == NULL)
    {
      printf("Error, could not open DOT file for writing: %s\n", filename);
      exit(1);
    }

   fprintf(output, "graph RuntimeTree {\n");

   WriteDotRecursive(output, total_thread_cycles);

   fprintf(output, "}\n");
   fclose(output);
}

// Recursive helper for the above
void RuntimeNode::WriteDotRecursive(FILE* output, long long int total_thread_cycles)
{
  static int nodeID = -1;
  int myID = ++nodeID;
  
  float cost = 100.f * (float)(num_instructions + num_data_stalls + num_contention_stalls) / (float)total_thread_cycles;
  unsigned char red = (unsigned char)(cost * 2.55);
  
  fprintf(output, "Node%d [label=\"%s\\n%2.2f\" color=\"#%02x0000\"];\n", myID, source_node->name.c_str(), cost, red);
  
  for(size_t i=0; i < children.size(); i++)
    {
      volatile int childID = nodeID + 1;
      children[i]->WriteDotRecursive(output, total_thread_cycles);
      cost = 100.f * (float)(children[i]->num_instructions + children[i]->num_data_stalls + children[i]->num_contention_stalls) / (float)total_thread_cycles;
      red = (unsigned char)(cost * 2.55);
      fprintf(output, "Node%d -- Node%d [label=\"\" color=\"#%02x0000\"];\n", myID, childID, red);
    }
}

// ----- Helpers -----
unsigned char readByte(const char* jump_table, int& addr)
{
  char retval = jump_table[addr];
  addr++;
  return retval; 
}

uint16_t readHalf(const char* jump_table, int& addr)
{
  int16_t retval = *((int16_t*)(jump_table + addr));
  addr+=2;
  return retval;
}

uint32_t readWord(const char* jump_table, int& addr)
{
  int32_t retval = *((int32_t*)(jump_table + addr));
  addr += 4;
  return retval;
}

unsigned int readVarSize(const char* jump_table, int& addr, int size)
{
  switch(size)
    {
    case 1:
      return readByte(jump_table, addr);
      break;
    case 2:
      return readHalf(jump_table, addr);
      break;
    case 4:
      return readWord(jump_table, addr);
      break;
    default:
      printf("ERROR (--profile): Debug data with invalid size: %d (should be 1, 2, or 4)\n", size);
      exit(-1);
      break;
    }
}

unsigned int readVarType(const char* jump_table, int& addr, int type)
{
  switch(type)
    {
      // 4 bytes
    case DW_FORM_addr:
    case DW_FORM_data4:
    case DW_FORM_strp:
    case DW_FORM_ref_addr:
    case DW_FORM_ref4:
      return readWord(jump_table, addr);
      break;

      // 2 bytes
    case DW_FORM_data2:
    case DW_FORM_ref2:
      return readHalf(jump_table, addr);
      break;      

      // 1 byte
    case DW_FORM_data1:
    case DW_FORM_ref1:
      return readByte(jump_table, addr);
      break;

    default:
      printf("ERROR (--profile): Debug data contains unhandled DWARF format specifier: %x\n", type);
      exit(1);
      break;
    }
}




// During simulation runtime, add a single instruction's contribution to the time consumed.
// DEPRECATED (profiles based on the source tree, no the call stack)
void CompilationUnit::AddInstructionContribution(Instruction* ins)
{
  for(size_t i = 0; i < ranges.size(); i++)
    if(ins->pc_address >= ranges[i].first && ins->pc_address < ranges[i].second)
      {
	num_instructions += ins->executions;
	num_data_stalls += ins->data_stalls;
	for(size_t i = 0; i < children.size(); i++)
	  children[i].AddInstructionContribution(ins);
	break;
      }
}


// Recursively print a unit's contribution
// DEPRECATED (profiles based on the source tree, no the call stack)
void CompilationUnit::PrintUnitContribution(int indent_level, long long int total_thread_cycles)
{
  if(num_instructions > 0)
    {
      if(name.compare(std::string("")))
	{
	  for(int i=0; i < indent_level; i++)
	    printf("  ");
	  printf("%s\t%2.2f\n", name.c_str(), 
	       100.f * (float)(num_instructions + num_data_stalls) / (float)total_thread_cycles);
	  indent_level++;
	}
      for(size_t i = 0; i < children.size(); i++)
	children[i].PrintUnitContribution(indent_level, total_thread_cycles);
    }
}


// Find the most specific containing unit for the instruction's PC
// DEPRECATED (profiles based on the source tree, no the call stack)
CompilationUnit* CompilationUnit::FindRelevantUnit(Instruction* ins)
{
  for(size_t i = 0; i < ranges.size(); i++)
    {
      if(ins->pc_address >= ranges[i].first && ins->pc_address < ranges[i].second)
	{
	  for(size_t j = 0; j < children.size(); j++)
	    {
	      CompilationUnit* cuChild = children[j].FindRelevantUnit(ins);
	      if(cuChild != NULL)
		return cuChild;
	    }
	  // only care about named units (don't want lexical blocks for example)
	  if(name.length() > 0) 
	    return this;
	}
    }
  return NULL;
}
