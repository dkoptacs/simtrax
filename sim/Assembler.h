#ifndef __SIMHWRT_ASSEMBLER_H_
#define __SIMHWRT_ASSEMBLER_H_
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Instruction.h"
#include "DwarfReader.h"

class DwarfReader;

// A label or register name
struct symbol
{
  std::vector<char*> names;
  int address;
  int size;
  bool isText;
  bool isJumpTable; // (this means its in the data segment)
  bool isAscii;

symbol() : 
  address(-1), isText(false), isJumpTable(false), isAscii(false)
  {}
};


class Assembler
{
 public:
  static int num_instructions, num_regs;
  static int LoadAssem(char *filename, std::vector<Instruction*>& instructions, 
		       std::vector<symbol*>& regs, int num_system_regs, char*& jump_table, 
		       std::vector<std::string>& ascii_literals, std::vector<std::string>& sourceNames, 
		       std::vector< std::vector< std::string > >& sourceLines,
		       bool print_symbols, bool needs_debug_symbols, DwarfReader* dwarfReader);
 private:
  static int HandleLine(std::string line, int pass, int lineNum, std::vector<Instruction*>& instructions, 
			std::vector<symbol*>& labels, std::vector<symbol*>& regs, 
			std::vector<symbol*>& elf_vars, std::vector<symbol*>& data_table, 
			char*& jump_table, std::vector<std::string>& ascii_literals, 
			std::vector<std::string>& sourceNames,
			std::vector< std::vector< std::string> >& sourceLines);

  static int HandleRegister(std::string line, int pass, std::vector<symbol*>& labels, 
			    std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars);

  static int HandleLabel(std::string line, int pass, std::vector<symbol*>& labels, 
			 std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars);

  static int HandleData(std::string line, int pass, std::vector<symbol*>& labels, 
			std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars, 
			std::vector<symbol*>& data_table, char*& jump_table);

  static int HandleInstruction(std::string line, int pass, int lineNum, std::vector<Instruction*>& instructions, 
			       std::vector<symbol*>& labels, std::vector<symbol*>& regs, 
			       std::vector<symbol*>& elf_vars);

  static int HandleArg(std::string arg,
		       std::vector<symbol*>& labels,
		       std::vector<symbol*>& regs,
		       std::vector<symbol*>& elf_vars,
		       int &retVal);

  static int HandleMipsDirective(std::string arg,
				 std::vector<symbol*>& labels,
				 std::vector<symbol*>& regs,
				 std::vector<symbol*>& elf_vars,
				 int &retVal);

  static int HandleSourceInfo(std::string line, int pass, 
			      std::vector<symbol*>& labels,
			      std::vector<symbol*>& regs,
			      std::vector<symbol*>& elf_vars);

  static int HandleFileName(std::string line, int pass, std::vector<std::string>& sourceNames,
			    std::vector< std::vector< std::string > >& sourceLines);

  static int HandleAssignment(std::string line, int pass,
			      std::vector<symbol*>& labels,
			      std::vector<symbol*>& regs,
			      std::vector<symbol*>& elf_vars);
    
  static int HasSymbol(std::string, std::vector<symbol*>& syms);  


  static int HandleConstructors(std::string line, int pass, std::vector<symbol*>& labels, 
				std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars);

  static int HandleSection(std::string line, int pass, std::vector<symbol*>& labels, 
			   std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars);
    
  static symbol* MakeSymbol(std::string name);

  static int GetArgs(std::string line, int* args, std::vector<symbol*>& labels, 
		     std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars, 
		     std::string argMatcher);


  static void AddTRaXInitialize(std::vector<Instruction*>& instructions,
			 std::vector<symbol*>& labels,
			 std::vector<symbol*>& regs,
			 char*& jump_table,
			 int end_data);

  static void PrintSymbols(std::vector<symbol*>& regs, std::vector<symbol*>& labels, 
			   std::vector<symbol*>& data_table, char* jump_table, int end_data);

  static std::string EscapedToAscii(std::string input);

  static void AddFileLines(std::string, std::vector< std::vector< std::string > >& sourceLines);

};

#define SECTION_DATA          0
#define SECTION_OTHER         1


#endif
