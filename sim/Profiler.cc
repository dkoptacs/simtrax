#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <queue>
#include "Profiler.h"


// Helpers
unsigned char readByte(const char* jump_table, int& addr);
uint16_t readHalf(const char* jump_table, int& addr);
uint32_t readWord(const char* jump_table, int& addr);
unsigned int readVarSize(const char* jump_table, int& addr, int size);
unsigned int readVarType(const char* jump_table, int& addr, int type);

Profiler::Profiler()
{
}

void Profiler::setDwarfReader(DwarfReader* _dwarfReader)
{
  dwarfReader = _dwarfReader;
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

  dwarfReader->rootRuntime->DistributeTime();
  dwarfReader->rootRuntime->Print(0, total_thread_cycles);

  // Write out a graphviz file representing the profile information
  dwarfReader->rootRuntime->WriteDot("runtime.dot", total_thread_cycles);

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
      dwarfReader->rootRuntime->AddInstructionContribution(stall_type);
      return dwarfReader->rootRuntime;
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
      for(size_t i = 0; i < dwarfReader->rootSource.children.size(); i++)
	{
	  cu = dwarfReader->rootSource.children[i].FindFunctionCall(ins);
	  if(cu != NULL)
	    break;
	}
      
      // Found no unit containing the instruction (this can happen in the preamble for example)
      // Just put it in to "main"
      if(cu == NULL)
	{
	  mostRelevant = dwarfReader->rootRuntime;
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
