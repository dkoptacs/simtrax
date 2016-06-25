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

  static int HasSymbol(std::string, const std::vector<symbol*>& syms);  
  
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

#define SECTION_DATA           0
#define SECTION_DEBUG          1
#define SECTION_OTHER          2



//-------------------------------------------------------------------------
// regex matchers for various assembly items
//-------------------------------------------------------------------------

// Whitespace
extern std::string expWhitespace;
// Argument separator
extern std::string expSeparator;
// Comment
extern std::string expComment;
// Any valid name (must contain a non-digit)
extern std::string expSymbol;
// An integer literal
extern std::string expIntLiteral;
// An integer offset
extern std::string expIntOffset;
// Integer arithmetic (different from offset since it gets collapsed in to another arg).
extern std::string expArithmetic;
// Declaring a register
extern std::string expRegister;
// Declaring a label 
extern std::string expLabel;
// 1-byte type
extern std::string expByte;
// 2-byte type
extern std::string exp2Byte;
// 4-byte type
extern std::string exp4Byte;
// Arbitrary-byte type
extern std::string expSpace;
// String type (terminated)
extern std::string expAsciz;
// String type (non-terminated)
extern std::string expAscii;
// Global
extern std::string expGlobal;
// ELF Sections
extern std::string expSection;
// ---Ignored data items---
// Not that useful in the context of the simulator
extern std::string expIgnored;
// Declaring a piece of data
extern std::string expData;
// ---Special MIPS directives---
extern std::string expHi;
extern std::string expLo;
extern std::string expGpRel;
extern std::string expMipsDirective;
// Any argument (label, register, integer literal, etc)
extern std::string expArg;
// ---Debug info---
// Line info
extern std::string expSrcInfo;
// Declaring a source file
extern std::string expFile;
// ELF assignments
extern std::string expAssign;
// String literal
extern std::string expString;


#endif
