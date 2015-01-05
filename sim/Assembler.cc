#include "Assembler.h"
#include <regex>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cassert>
int Assembler::num_instructions = 0;
int Assembler::num_regs = 1;

int jtable_size;
int jtable_ptr;
int debug_start;
int ascii_table_size;

//-------------------------------------------------------------------------
// regex matchers for various assembly items
//-------------------------------------------------------------------------

// Whitespace
std::string expWhitespace( "[ \\t]" );
// Argument separator
std::string expSeparator( "[ \\t,]" );
// Comment
std::string expComment( "#.*" );
// Any valid name (must contain a non-digit)
std::string expSymbol( "[0-9]*[\\.a-zA-Z\\$_]+[\\.a-zA-Z\\$_0-9]*" );
// An integer literal
std::string expIntLiteral( "[-|\\+]?[0-9]+" ); 
// An integer offset
std::string expIntOffset( expSeparator + "+" + expIntLiteral + "\\(" );
// Integer arithmetic (different from offset since it gets collapsed in to another arg).
std::string expArithmetic( "\\)[\\+-]" );
// Declaring a register
std::string expRegister( "\\tREG" + expWhitespace + "+" + expSymbol );
// Declaring a label 
std::string expLabel( expSymbol + ":" );
// 1-byte type
std::string expByte( "\\.byte" );
// 2-byte type
std::string exp2Byte( "\\.2byte" );
// 4-byte type
std::string exp4Byte( "\\.4byte" );
// Arbitrary-byte type
std::string expSpace( "\\.space" );
// String type
std::string expAscii( "\\.asciz" );
// Global
std::string expGlobal( "\\.globl" );
// Constructors section
std::string expConstructors( "\\.ctors" );
// ---Ignored data items---
// Not that useful in the context of the simulator
std::string expIgnored( "(\\.data)|(\\.text)|(\\.previous)|(\\.align)|(\\.type)|(\\.ent)|(\\.frame)|(\\.mask)|(\\.fmask)|(\\.set)|(\\.size)|(\\.end)" );
// Declaring a piece of data
std::string expData( "((" + expByte + ")|(" + exp2Byte + ")|(" + exp4Byte + ")|(" + expSpace + ")|(" + expAscii + ")|(" + expGlobal + ")|(" + expIgnored + "))" );
// ---Special MIPS directives---
std::string expHi( "\%hi" );
std::string expLo( "\%lo" );
std::string expGpRel( "\%gp_rel" );
std::string expMipsDirective( "((" + expHi + ")|(" + expLo + ")|(" + expGpRel + "))\\(" + expSymbol + "\\)" );
// Any argument (label, register, integer literal, etc)
//std::string expAssignArg( "(" + expMipsDirective + ")|(" + expIntOffset + ")|(" + expSymbol + ")|(" + expIntLiteral + ")|(\\+|-)" );
std::string expArg( "(" + expMipsDirective + ")|(" + expIntOffset + ")|(" + expSymbol + ")|(" + expIntLiteral + ")|(" + expArithmetic + ")" );
//std::string expArg( "(" + expMipsDirective + ")|(" + expIntOffset + ")|(" + expSymbol + ")|(" + expIntLiteral + ")|(\\+|-)" );
// ---Debug info---
// Line info
std::string expSrcInfo( "\\.loc" );
// Declaring a source file
std::string expFile( "\\.file" );
// ELF assignments
std::string expAssign( "=" );
// String literal
std::string expString("\".+\"");



//-------------------------------------------------------------------------


using namespace std;
using std::regex;
using std::sregex_iterator;


//------------------------------------------------------------------------------

SourceInfo currentSourceInfo;

int Assembler::LoadAssem(char *filename,
                         std::vector<Instruction*>& instructions,
                         std::vector<symbol*>& regs,
                         int num_system_regs,
                         char*& jump_table,
                         std::vector<std::string>& ascii_literals,
			 std::vector<std::string>& sourceNames,
                         bool print_symbols)
{
  printf("Loading assembly file %s\n", filename);

  ifstream input(filename);
  if(!input.is_open())
  {
    printf("ERROR: cannot open assembly file %s\n", filename);
    return -1;
  }
  num_instructions = 0;
  jtable_size = 0;
  jtable_ptr = 0;
  debug_start = 0;
  ascii_table_size = 0;
  currentSourceInfo.lineNum = -1;
  currentSourceInfo.colNum = -1;
  currentSourceInfo.fileNum = -1;
 
  /* reserve 4 special purpose registers
     0 -- when used as argument to PRINT, prints a tab (\t)
     1 -- contains thread id
     2 -- contains core id
     3 -- contains L2 id
  */
  num_regs = 0;
  symbol *temp = new symbol();

  temp->names.push_back((char*)malloc(8));
  strcpy(temp->names.at(temp->names.size()-1), "$TAB");
  temp->address = num_regs++;
  regs.push_back(temp);

  temp = new symbol();
  temp->names.push_back((char*)malloc(8));
  strcpy(temp->names.at(temp->names.size()-1), "$TID");
  temp->address = num_regs++;
  regs.push_back(temp);

  temp = new symbol();
  temp->names.push_back((char*)malloc(8));
  strcpy(temp->names.at(temp->names.size()-1), "$CID");
  temp->address = num_regs++;
  regs.push_back(temp);

  temp = new symbol();
  temp->names.push_back((char*)malloc(8));
  strcpy(temp->names.at(temp->names.size()-1), "$L2ID");
  temp->address = num_regs++;
  regs.push_back(temp);

  std::vector<symbol*> labels;
  std::vector<symbol*> elf_vars;
  std::vector<symbol*> data_table; // holds all .data, for debug purposes
  int lineCount = 1;

  // Add a data entry for gnu_local_gp that always evaluates to 0
  temp = new symbol();
  temp->names.push_back((char*)malloc(15));
  strcpy(temp->names.at(temp->names.size()-1), "__gnu_local_gp");
  temp->address = 0;
  temp->isJumpTable = true;
  temp->size = 4;
  labels.push_back(temp);
  data_table.push_back(temp);

  jtable_size = 4; // accommodate gnu_local_gp

  // 1st pass
  while(!input.eof())
    {
      std::string line;
      getline(input, line);
      if(!HandleLine(line, 1, instructions, labels, regs, elf_vars, data_table, jump_table, ascii_literals, sourceNames))
	{
	  printf("Line %d: %s\n", lineCount, line.c_str());
	  return -1;
	}
      lineCount++;
    }
  
  lineCount = 1;

  // Interpass - allocate and pre-load gnu_local_gp
  jump_table = (char*)malloc(jtable_size);
  ((int32_t*)jump_table)[0] = (int32_t)(temp->address);
  jtable_ptr += 4;
  
  input.clear();
  input.seekg(0, ios::beg) ;

  // 2nd pass
  
  while(!input.eof())
    {
      std::string line;
      getline(input, line);
      if(!HandleLine(line, 2, instructions, labels, regs, elf_vars, data_table, jump_table, ascii_literals, sourceNames))
	{
	  printf("Line %d: %s\n", lineCount, line.c_str());
	  return -1;
	}
      lineCount++;
    }
  
  input.close();

  int end_data = debug_start != 0 ? debug_start : jtable_size;


  // Last label is .TRaX_INIT, but contains no code in the assembly file
  // Assembler must add initialization by hand

  AddTRaXInitialize(instructions, labels, regs, jump_table, end_data);


  if(print_symbols)
    PrintSymbols(regs, labels, data_table, jump_table, end_data);

  return end_data;
}

// Parses a single line of assembly
int Assembler::HandleLine(std::string line,
                          int pass,
                          std::vector<Instruction*>& instructions,
                          std::vector<symbol*>& labels,
                          std::vector<symbol*>& regs,
                          std::vector<symbol*>& elf_vars,
			  std::vector<symbol*>& data_table,
                          char*& jump_table,
                          std::vector<std::string>& ascii_literals,
			  std::vector<std::string>& sourceNames)
{

  std::smatch m;

  // First remove any comments 
  if(std::regex_search(line, m, std::regex(expComment)))
    {
      line = line.substr(0, line.find("#"));
    }

  // Ignore blank lines
  if(!std::regex_search(line, m, std::regex("\\S")))
    return 1;
  
  // Constructors section
  if(std::regex_search(line, m, std::regex(expConstructors)))
    {
      return HandleConstructors(line, pass, labels, regs, elf_vars);
    }  
  // Register declaration
  if(std::regex_search(line, m, std::regex(expRegister)))
    {
      return HandleRegister(line, pass, labels, regs, elf_vars);
    }
  // Label declaration
  if(std::regex_search(line, m, std::regex(expLabel)))
    {
      return HandleLabel(line, pass, labels, regs, elf_vars);
    }
  // Data section item
  if(std::regex_search(line, m, std::regex(expData)))
    {
      return HandleData(line, pass, labels, regs, elf_vars, data_table, jump_table);
    }
  // Debug line info
  if(std::regex_search(line, m, std::regex(expSrcInfo)))
    {
      return HandleSourceInfo(line, pass, labels, regs, elf_vars);
    }
  // Debug source file declaration
  if(std::regex_search(line, m, std::regex(expFile)))
    {
      return HandleFileName(line, pass, sourceNames);
    }
  // ELF assignment
  if(std::regex_search(line, m, std::regex(expAssign)))
    {
      return HandleAssignment(line, pass, labels, regs, elf_vars);
    }
  // Otherwise, it must be an instruction
  return HandleInstruction(line, pass, instructions, labels, regs, elf_vars);
}


// Adds a register declaration
int Assembler::HandleRegister(std::string line, int pass, std::vector<symbol*>& labels, std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars)
{
  // register declarations handled on 1st pass
  if(pass == 2) 
    return 1;
  
  // Since "REG" is contained in expSymbol, must strip it out
  std::string stripped = line.substr(line.find("REG") + 3);

  std::smatch m;  
  if(!std::regex_search(stripped, m, std::regex(expSymbol)))
    {
      printf("ERROR: invalid register declaration\n");
      return 0;
    }
 
  // check for duplicates
  if(HasSymbol(m.str(), labels) >= 0 || 
     HasSymbol(m.str(), regs) >= 0 || 
     HasSymbol(m.str(), elf_vars) >= 0)
    {
      printf("ERROR: symbol %s previously declared\n", m.str().c_str());
      return 0;
    }
     
  symbol* r = MakeSymbol(m.str());
  r->address = num_regs++;
  regs.push_back(r);

  return 1;
}


// Adds a label declaration
int Assembler::HandleLabel(std::string line, int pass, std::vector<symbol*>& labels, 
			   std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars)
{

  // label declarations handled on 1st pass
  if(pass == 2) 
    return 1;


  // First get the name of the label
  std::smatch m;  
  if(!std::regex_search(line, m, std::regex(expSymbol)))
    {
      printf("ERROR: invalid label declaration\n");
      return 0;
    }
  
  // check for duplicates
  if(HasSymbol(m.str(), labels) >= 0 || 
     HasSymbol(m.str(), regs) >= 0 || 
     HasSymbol(m.str(), elf_vars) >= 0)
    {
      printf("ERROR: Symbol %s previously declared\n", m.str().c_str());
      return 0;
    }
  
  // Check for start of debug section
  // Must have end of text and data segments
  if(m.str().compare("$text_end") == 0)
    for(int i=0; i < labels.size(); i++)
      if(strcmp(labels[i]->names[0], "$data_end"))
	{
	  debug_start = jtable_size;
	  break;
	}
  if(m.str().compare("$data_end") == 0)
    for(int i=0; i < labels.size(); i++)
      if(strcmp(labels[i]->names[0], "$text_end"))
	{
	  debug_start = jtable_size;
	  break;
	}

  symbol* r = MakeSymbol(m.str());
  // Address defaults to a PC address. Data labels will have their addresses fixed when data is encountered
  r->address = num_instructions; 
  labels.push_back(r);
  
  return 1;
}


// Adds data to the data segment (.byte, .asciz, etc)
int Assembler::HandleData(std::string line, int pass, std::vector<symbol*>& labels, 
			  std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars,
			  std::vector<symbol*>& data_table,
			  char*& jump_table)
{

  std::smatch m;
  std::regex_search(line, m, std::regex(expData));
  
  // 1st pass: update the label associated with this declaration if necessary
  if(pass == 1) 
    {
      // Find the label associated with the current item.
      // Set it as a data label
      if(labels.size() > 0 && !labels[labels.size()-1]->isJumpTable && !labels[labels.size()-1]->isAscii)
        {
	  // If the label associated with this line has instructions under it
	  if(labels[labels.size()-1]->isText)
	    {
	      printf("ERROR: Found label with mixed text/data: %s\n", labels[labels.size()-1]->names[0]);
	      return 0;
	    }
	  labels[labels.size()-1]->isJumpTable = true;
	  labels[labels.size()-1]->address = jtable_size;
	}

      // Update previous label if nothing underneath it
      if(labels.size() > 1 && !labels[labels.size()-1]->isJumpTable && !labels[labels.size()-1]->isAscii)
	{
	  labels[labels.size()-2]->isJumpTable = true;
	  labels[labels.size()-2]->address = jtable_size;
	}
      

      symbol* ds = new symbol();
      ds->address = jtable_size;      

      // Make space for the entry
      // Strings
      if(m.str().compare(".asciz") == 0)
	{
	  if(!std::regex_search(line, m, std::regex(expString)))
	    {
	      printf("ERROR: Found .asciz data item without a valid string\n");
	      return 0;
	    }
	  // String length, subtract 2 to get rid of quotes, add one back to accomodate terminating NULL char
	  // Total = length - 1
	  ds->size = EscapedToAscii(m.str()).length() - 1;
	  ds->isAscii = true;
	}
      // Arbitrary size
      else if(m.str().compare(".space") == 0) 
	{
	  std::string stripped = line.substr(line.find(m.str()) + m.str().length());
	  int args[4];
	  std::smatch exp;
	  // 2 pass assembler can only handle immediate integers for declaring .space
	  if(!std::regex_search(stripped, exp, std::regex(expSeparator + "+" + expIntLiteral)))
	    {
	      printf("ERROR: Unknown size for .space allocation\n");
	      return 0;
	    }
	  if(!GetArgs(stripped, args, labels, regs, elf_vars, expArg) ||
	     args[1] != 0 || // can only have one argument for data entries
	     args[2] != 0 ||
	     args[3] != 0)
	    {
	      printf("ERROR: Invalid .space allocation\n");
	      return 0;
	    }
	  ds->size = args[0];
	}
      else if(m.str().compare(".byte") == 0)
	ds->size = 1;
      else if(m.str().compare(".2byte") == 0)
	ds->size = 2;
      else
	ds->size = 4;

      data_table.push_back(ds);

      jtable_size += ds->size;

      // We will add the entry to the table on the 2nd pass
      // once all symbol values are known
      return 1;
    }

  else // 2nd pass: compute the value of the data
    {
      std::string stripped = line.substr(line.find(m.str()) + m.str().length());
      if(m.str().compare(".asciz") == 0) // strings
	{
	  std::regex_search(stripped, m, std::regex(expString));
	  // Remove quotes
	  std::string noQuotes = EscapedToAscii(m.str().substr(1, m.str().length() - 2));
	  
	  // Copy 1 extra char to get the NULL terminator
	  memcpy(&(jump_table[jtable_ptr]), noQuotes.c_str(), noQuotes.length() + 1);
	  jtable_ptr += noQuotes.length() + 1;
	}
      else // non-string
	{
	  int args[4];
	  std::smatch ign;
	  if(std::regex_search(m.str(), ign, std::regex(expIgnored)))
	    args[0] = 0;
	  else
	    {
	      if(!GetArgs(stripped, args, labels, regs, elf_vars, expArg) ||
		 args[1] != 0 || // can only have one argument for data entries
		 args[2] != 0 ||
		 args[3] != 0)
		{
		  printf("ERROR: Invalid data entry\n");
		  return 0;
		}
	    }

	  if(m.str().compare(".space") == 0)
	    {
	      int args[4];
	      // Error handling is done on 1st pass
	      GetArgs(stripped, args, labels, regs, elf_vars, expArg);
	      for(int i=0; i < args[0]; i++)
		jump_table[jtable_ptr++] = (char)(0);
	    }
	  else if(m.str().compare(".byte") == 0)
	    {
	      jump_table[jtable_ptr] = (char)(args[0]);
	      jtable_ptr += 1;
	    }
	  else if(m.str().compare(".2byte") == 0)
	    {
	      assert(sizeof(int16_t) == 2);
	      *((int16_t*)(jump_table + jtable_ptr)) = (int16_t)(args[0]);
	      jtable_ptr += 2;
	    }
	  else
	    {
	      assert(sizeof(int32_t) == 4);
	      *((int32_t*)(jump_table + jtable_ptr)) = (int32_t)(args[0]);
	      jtable_ptr += 4;
	    }
	}
      return 1;
    }
  return 0;
}


// Adds an instruction to the simulator's instruction memory
int Assembler::HandleInstruction(std::string line, int pass, std::vector<Instruction*>& instructions, 
				 std::vector<symbol*>& labels, std::vector<symbol*>& regs, 
				 std::vector<symbol*>& elf_vars)
{
  // Have to wait until all labels/data are known before generating instructions
  // First pass just updates the label associated with the instruction if necessary
  if(pass == 1)
    {
      // Find the label associated with the current item.
      if(labels.size() > 0 && (labels[labels.size()-1]->isJumpTable || labels[labels.size()-1]->isAscii))
        {
	      printf("ERROR: Found label with mixed text/data: %s\n", labels[labels.size()-1]->names[0]);
	      return 0;
	}
      // Set it as a text label
      if(labels.size() > 0)
	labels[labels.size()-1]->isText = true;
      // Update previous label if nothing underneath it
      if(labels.size() > 1 && !labels[labels.size()-2]->isJumpTable && !labels[labels.size()-2]->isAscii && !labels[labels.size()-2]->isText)
	labels[labels.size()-2]->isText = true;

      num_instructions++;
      return 1;
    }
  
  // 2nd pass
  // Get the op code
  std::smatch m;
  if(!std::regex_search(line, m, std::regex(expSymbol)))
    {
      printf("ERROR: Malformed assembly line\n");
      return 0;
    }

  for (Instruction::Opcode i = Instruction::ADD; i < Instruction::NUM_OPS; i = (Instruction::Opcode)(i + 1)) 
    {
      if(m.str().compare(Instruction::Opnames[i]) == 0)
	{
	  // Get args, first remove the op from the string
	  std::string stripped = line.substr(line.find(m.str()) + m.str().length());
	  int args[4];
	  if(!GetArgs(stripped, args, labels, regs, elf_vars, expArg))
	    {
	      printf("ERROR: Invalid arguments to op: %s\n", Instruction::Opnames[i].c_str());
	      return 0;
	    }
	  instructions.push_back(new Instruction(i, 
						 args[0], 
						 args[1], 
						 args[2], 
						 currentSourceInfo,
						 instructions.size()));
	  return 1;
	}
    }
  
  // HandleLine assumes Instruction is the default. If this fails, the whole line is invalid
  printf("ERROR: Malformed assembly line\n");
  return 0;
}


// Creates a symbol with given name. Uses c-style strings for compatability with old assembler
symbol* Assembler::MakeSymbol(std::string name)
{
  symbol *r = new symbol();
  r->names.push_back((char*)malloc(1000));
  strcpy(r->names.at(r->names.size()-1), name.c_str());
  return r;
}


// Parses/computes assembly arguments and places them in args
// Arguments can be either to an instruction, or an ELF variable
int Assembler::GetArgs(std::string line,
                       int* args,
                       std::vector<symbol*>& labels,
                       std::vector<symbol*>& regs,
                       std::vector<symbol*>& elf_vars,
		       std::string argMatcher)
{

  for(int i=0; i < 4; i++)
    args[i] = 0;

  int argNum = 0;
  int arg;
  std::regex args_regex(argMatcher);
  std::sregex_iterator it = std::sregex_iterator(line.begin(), line.end(), args_regex);
  std::sregex_iterator end = std::sregex_iterator();

  while(it != end)
    {
      // Consume operators and collapse result in to one argument
      std::smatch m;
      if(std::regex_search(it->str(), m, std::regex(expArithmetic)))
	{
	  it++;
	  argNum--;
	  if(!HandleArg(it->str(), labels, regs, elf_vars, arg))
	    return 0;
	  if((m.str().compare("+") == 0))
	    args[argNum] += arg;
	  else if(m.str().compare("-") == 0)
	    args[argNum] -= arg;
	}
      // Otherwise, it's a standalone argument
      else if(HandleArg(it->str(), labels, regs, elf_vars, arg))
	{
	  args[argNum] = arg;
	}
      else
	return 0;
      it++;
      argNum++;
    }

  return 1;
}


// Parses/computes a single argument
// Arguments can be either to an instruction, or an ELF variable
int Assembler::HandleArg(std::string arg,
			 std::vector<symbol*>& labels,
			 std::vector<symbol*>& regs,
			 std::vector<symbol*>& elf_vars,
			 int &retVal)
{
  
  std::smatch m;  
  // Mips directives
  if(std::regex_search(arg, m, std::regex(expMipsDirective)))
    {
      return HandleMipsDirective(arg, labels, regs, elf_vars, retVal);
    }

  // Symbols (must contain a non-digit)
  if(std::regex_search(arg, m, std::regex(expSymbol)))
    {
      int symbolIndex;
      symbol* symb = NULL;
      if((symbolIndex = HasSymbol(m.str(), regs)) >= 0)
	symb = regs[symbolIndex];
      if((symbolIndex = HasSymbol(m.str(), labels)) >= 0)
	symb = labels[symbolIndex];
      if((symbolIndex = HasSymbol(m.str(), elf_vars)) >= 0)
	symb = elf_vars[symbolIndex];

      if(symb == NULL)
	{
	  printf("ERROR: use of undeclared symbol: %s\n", m.str().c_str()); 
	  return 0;
	}

      retVal = symb->address;
      return 1;
    }

  // Integer literals
  if(std::regex_search(arg, m, std::regex(expIntLiteral)))
    {
      retVal = stoul(m.str());
      return 1;
    }

  printf("ERROR: invalid argument: %s\n", arg.c_str());
  return 0;
}
  

// Handles arguments encapsulated int mips modifiers %hi(), %lo(), and %gp_rel()
int Assembler::HandleMipsDirective(std::string arg,
				   std::vector<symbol*>& labels,
				   std::vector<symbol*>& regs,
				   std::vector<symbol*>& elf_vars,
				   int &retVal)
{
  std::smatch dirType;
  if(!std::regex_search(arg, dirType, std::regex(expHi)))
    if(!std::regex_search(arg, dirType, std::regex(expLo)))
      if(!std::regex_search(arg, dirType, std::regex(expGpRel)))
	{
	  printf("ERROR: Unknown MIPS directive\n");
	  return 0;
	}
 
  // Remove the directive
  std::string stripped = arg.substr(arg.find(dirType.str()) + dirType.str().length());

  // Get the argument name
  std::smatch m;
  if(!std::regex_search(stripped, m, std::regex(expArg)))
    {
      printf("ERROR: Invalid argument to mips directive\n");
      return 0;
    }    

  // Can't have registers in mips directives
  if(HasSymbol(m.str(), regs) >= 0)
    {
      printf("ERROR: Found register argument in MIPS directive\n");
      return 0;
    }

  // Get the argument value
  int args[4];
  if(!GetArgs(m.str(), args, labels, regs, elf_vars, expArg) ||
     args[1] != 0 || // can only have one argument for directives
     args[2] != 0 ||
     args[3] != 0)
    {
      printf("ERROR: Invalid mips directive entry\n");
      return 0;
    }
  
  int labelNum = HasSymbol(m.str(), labels);
  if(labelNum >= 0)
    {
      // Compiler should never try to take the upper/lower half of a PC address
      if (!labels.at(labelNum)->isJumpTable && !labels.at(labelNum)->isAscii)
	{
	  printf("ERROR: Expected data label in MIPS directive: label = %s\n", 
		 labels.at(labelNum)->names[0]);
	  
	  return 0;
	}
    }
  
  if (!dirType.str().compare(expHi))
    retVal = args[0] >> 16;
  else if (!dirType.str().compare(expLo))
    retVal = args[0] & 0x0000FFFF;
  else if (!dirType.str().compare(expGpRel))
    retVal = args[0];

  return 1;
}


// Debug info: (.loc) associates a file:line:column with an instruction if assembly contains debug info
int Assembler::HandleSourceInfo(std::string line, int pass,
				std::vector<symbol*>& labels,
				std::vector<symbol*>& regs,
				std::vector<symbol*>& elf_vars)
{
  // Source info only matters when instructions are generated (on pass 2).
  if(pass == 1)
    return 1;
  
  // Strip out ".loc"
  std::smatch m;  
  std::regex_search(line, m, std::regex(expSrcInfo));
  std::string stripped = line.substr(line.find(m.str()) + m.str().length());

  // Strip out non-digits (such as "prologue_end")
  if(std::regex_search(stripped, m, std::regex("[\\S\\D]")))
    {
      stripped = stripped.substr(0, stripped.find(m.str()));
    }
    

  int args[4];
  if(!GetArgs(stripped, args, labels, regs, elf_vars, expArg))
    {
      printf("ERROR: Invalid source line info (compiled with -g)\n");
      return 0;
    }
    
  currentSourceInfo.fileNum = args[0];
  currentSourceInfo.lineNum = args[1];
  currentSourceInfo.colNum = args[2];

  return 1;
}


// Debug info: (.file) declares a path to one of the source files, used by .loc
int Assembler::HandleFileName(std::string line, int pass, std::vector<std::string>& sourceNames)
{

  if(pass == 2) // ignore .file on pass 2
    return 1;

  // Strip out the ".file"
  std::smatch m;  
  std::regex_search(line, m, std::regex(expFile));
  std::string stripped = line.substr(line.find(m.str()) + m.str().length());
  
  // Strip out file number (implied by order of appearance)
  if(std::regex_search(stripped, m, std::regex(expIntOffset)))
    stripped = stripped.substr(stripped.find(m.str()) + m.str().length());

  // Get the file name
  if(!std::regex_search(stripped, m, std::regex(expString)))
    {
      printf("ERROR: Invalid file name declaration (compiled with -g)\n");
      return 0;
    }

  // Strip out the enclosing quotes
  sourceNames.push_back(m.str().substr(1, m.str().length() - 2));

  return 1;
}


// Assignment of temporary assembler-only variable
int Assembler::HandleAssignment(std::string line, int pass,
				std::vector<symbol*>& labels,
				std::vector<symbol*>& regs,
				std::vector<symbol*>& elf_vars)
{

  // 1st pass: make an entry for it
  if(pass == 1)
    {
      std::smatch m;  
      std::regex_search(line, m, std::regex(expSymbol));

      // check for duplicates
      if(HasSymbol(m.str(), labels) >= 0 || 
	 HasSymbol(m.str(), regs) >= 0 || 
	 HasSymbol(m.str(), elf_vars) >= 0)
	{
	  printf("ERROR: Symbol %s previously declared\n", m.str().c_str());
	  return 0;
	}

      // Then add it to the list
      symbol* s = MakeSymbol(m.str());
      s->address = -1;
      elf_vars.push_back(s);

    }
  
  // 2nd pass: calculate its value
  else
    {
      // Strip out the symbol name
      std::smatch m;  
      std::regex_search(line, m, std::regex(expSymbol));
      std::string stripped = line.substr(line.find(m.str()) + m.str().length());
      
      int varID = HasSymbol(m.str(), elf_vars);
      if(varID < 0)
	{
	  printf("ERROR: 2nd pass couldn't find ELF assignment symbol: %s\n", m.str().c_str());
	  return 0;
	}

      // Get the argument value
      int args[4];
      // expAssignArg includes '+'/'-' in the argument matcher, GetArgs will handle them
      //if(!GetArgs(stripped, args, labels, regs, elf_vars, expAssignArg) ||
      if(!GetArgs(stripped, args, labels, regs, elf_vars, expArg) ||
	 args[1] != 0 || // can only have one argument for assignments (the assigned value)
	 args[2] != 0 ||
	 args[3] != 0)
	{
	  printf("ERROR: Invalid ELF assignment\n");
	  return 0;
	}
      
      // Update the symbol's value
      elf_vars.at(varID)->address = args[0];
    }

  return 1; 
  
}


int Assembler::HandleConstructors(std::string line, int pass, std::vector<symbol*>& labels, 
				  std::vector<symbol*>& regs, std::vector<symbol*>& elf_vars)
{
  if(pass == 2)
    return 1;
  // Create a special label that will hold the address of the constructors code
  symbol* r = MakeSymbol(".ctors");
  r->address = -1;
  labels.push_back(r);

  return 1;
}


// If simulator invoked with --print-symbols
// Prints register names, label names, and the data segment
// For debug purposes
void Assembler::PrintSymbols(std::vector<symbol*>& regs, std::vector<symbol*>& labels, std::vector<symbol*>& data_table, char* jump_table, int end_data)
{
  int i;
  printf("Registers:\nName\tNumber\n");
  for(i=0; i<regs.size(); i++)
    printf("%s:\t%d\n", regs.at(i)->names.at(0), regs.at(i)->address);
  printf("Instruction labels:\nName\tPC\n");
  for(i=0; i<labels.size(); i++)
    {
      if(labels.at(i)->isText)
	printf("%s:\t%d\n", labels.at(i)->names.at(0), labels.at(i)->address);
    }
  printf("Data segment:\nName\tAddress\tValue\n");
  
  int current_address = 0;
  for(i=0; i < data_table.size(); i++ )
    {
      // No need to print debug info
      if(current_address >= end_data)
	break;
      // Find corresponding symbol (if any)
      for(int j = 0; j < labels.size(); j++)
	if((!labels[j]->isText) && (labels[j]->address == current_address))
	  {
	      printf("%s", labels[j]->names[0]);
	      break;
	  }
      printf("\t");

      printf("%d\t", current_address);

      if(data_table[i]->isAscii)
	printf("\"%s\"\n", &(jump_table[current_address]));
      else if(data_table[i]->size == 1)
	printf("%d\n", *((char*)(jump_table + current_address)));
      else if(data_table[i]->size == 2)
	printf("%d\n", *((short*)(jump_table + current_address)));
      else if(data_table[i]->size == 4)
	printf("%d\n", *((int*)(jump_table + current_address)));
      else
	printf("(%d bytes)\n", data_table[i]->size);

      current_address += data_table[i]->size;
    }
}

//------------------------------------------------------------------------------


// Checks whether or not the given set of symbols (labels, registers, elf_vars), has the specified symbol
// Returns index or -1
int Assembler::HasSymbol(std::string name, std::vector<symbol*>& syms)
{
  unsigned int i, j;
  for(i=0; i<syms.size(); i++)
    for(j=0; j<syms.at(i)->names.size(); j++)
    {
      if(strcmp(syms.at(i)->names.at(j), name.c_str())==0)
	return (int)i;
    }
  return -1;
}


// Adds instructions that the compiler doesn't generate.
// Namely, branches to global constructors followed by a brach to main
void Assembler::AddTRaXInitialize(std::vector<Instruction*>& instructions,
				  std::vector<symbol*>& labels,
				  std::vector<symbol*>& regs,
				  char*& jump_table,
				  int end_data)
{

  labels[labels.size()-1]->isText = true;

  SourceInfo tmpSrcInfo;

  // Create branches to global constructors
  int ctorsLabelID = HasSymbol(".ctors", labels);
  if(ctorsLabelID >= 0)
    {
      int startCtors = labels[ctorsLabelID]->address;
      int endCtors = end_data;
      for(int i = ctorsLabelID + 1; i < labels.size(); i++)
	if(labels[i]->isJumpTable)
	  {
	    endCtors = labels[i]->address;
	    break;
	  }
      
      while(startCtors < endCtors)
	{
	  // Add a jump and link with nop branch delay for each constructor
	  instructions.push_back(new Instruction(Instruction::jal,
						 *((int*)(jump_table + startCtors)),
						 0, 
						 0, 
						 tmpSrcInfo,
						 instructions.size()));
	  instructions.push_back(new Instruction(Instruction::nop,
						 0,
						 0, 
						 0, 
						 tmpSrcInfo,
						 instructions.size()));
	  startCtors += 4; // instruction addresses have to be 4 bytes
	}
    }

  // Return to the preamble created by the linker
  int startID = HasSymbol(".start", labels);
  if(startID < 0)
    {
      printf("ERROR: Assembler found no .start label\n");
      exit(1);
    }
  instructions.push_back(new Instruction(Instruction::j,
					 labels[startID]->address,
					 0, 
					 0, 
					 tmpSrcInfo,
					 instructions.size()));

  instructions.push_back(new Instruction(Instruction::nop,
					 0,
					 0, 
					 0, 
					 tmpSrcInfo,
					 instructions.size()));
}



//------------------------------------------------------------------------------

// Converts strings containing literally escaped characters to their ascii equivalent,
// such as:
// "a\nb"
// becomes
// "a
//  b"
std::string Assembler::EscapedToAscii(std::string input)
{
  std::string retval;
  std::string::iterator it = input.begin();
  while (it != input.end())
  {
    if (*it == '\\')
    {
      it++;
      if(it == input.end())
        break;
      switch (*it)
      {
        case '\'':
          retval += '\'';
          break;
        case '\"':
          retval += '\"';
          break;
        case '?':
          retval += '?';
          break;
        case '\\':
          retval += '\\';
          break;
        case '0':
          retval += '\0';
          break;
        case 'a':
          retval += '\a';
          break;
        case 'b':
          retval += '\b';
          break;
        case 'f':
          retval += '\f';
          break;
        case 'n':
          retval += '\n';
          break;
        case 'r':
          retval += '\r';
          break;
        case 't':
          retval += '\t';
          break;
        case 'v':
          retval += '\v';
          break;
        default:
          continue;
      }
    }
    else
      retval += *it;

    it++;
  }
  return retval;
}
