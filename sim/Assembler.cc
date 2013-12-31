#include "Assembler.h"
int Assembler::num_instructions = 0;
int Assembler::num_regs = 1;
int Assembler::start_wq = 0;
int Assembler::start_framebuffer = 0;
int Assembler::start_camera = 0;
int Assembler::start_scene = 0;
int Assembler::start_light = 0;
int Assembler::start_bg_color = 0;
int Assembler::start_matls = 0;
int Assembler::start_permutation = 0;

int jtable_size;
int ascii_table_size;

const char *delimeters = ", ()[\t\n"; // includes only left bracket for backwards compatibility with vector registers

int Assembler::LoadAssem(char *filename, std::vector<Instruction*>& instructions, std::vector<symbol*>& regs, int num_system_regs, std::vector<int>& jump_table, std::vector<std::string>& ascii_literals, int _start_wq, int _start_framebuffer, int _start_camera, int _start_scene, int _start_light, int _start_bg_color, int start_matls, int start_permutation, bool print_symbols)
{
  printf("Loading assembly file %s\n", filename);
  Assembler::start_wq = _start_wq;
  Assembler::start_framebuffer = _start_framebuffer;
  Assembler::start_camera = _start_camera;
  Assembler::start_scene = _start_scene;
  Assembler::start_light = _start_light;
  Assembler::start_bg_color = _start_bg_color;
  Assembler::start_matls = start_matls;
  Assembler::start_permutation = start_permutation;
 
  FILE *input = fopen(filename, "r");
  if(!input)
    {
      printf("ERROR: cannot open assembly file %s\n", filename);
      return 0;
    }

  num_instructions = 0;
  jtable_size = 0;
  ascii_table_size = 0;

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
  //std::vector<symbol> regs;
  int lineCount = 1;
  unsigned int i;
  // 1st pass
  while(!feof(input))
    {
      if(!handleLine(input, 1, instructions, labels, regs, jump_table, ascii_literals))
	{
	  printf("line %d\n", lineCount);
	  return 0;
	}
      lineCount++;
    }
  fclose(input);

  // Fix up the ascii literal labels' addresses now that the jtable size is known.
  // Ascii literals will go right after the jtable on the stack
  // This is sort of like pass 1.5
  for(i=0; i<labels.size(); i++)
    if(labels.at(i)->isAscii)
      labels.at(i)->address += jtable_size;
  

  input = fopen(filename, "r");
  lineCount = 1;
  // 2nd pass
  //jtable_size = 0; // reset jump table counter
  while(!feof(input))
    {
      if(!handleLine(input, 2, instructions, labels, regs, jump_table, ascii_literals))
	{
	  printf("line %d\n", lineCount);
	  return 0;
	}
      lineCount++;
    }

  if(print_symbols)
    {
      printf("Symbol table:\n");
      printf("Registers:\nName\tNumber\n");
      for(i=0; i<regs.size(); i++)
	printf("%s:\t%d\n", regs.at(i)->names.at(0), regs.at(i)->address);
      printf("Labels:\nName\tAddress\n");
      for(i=0; i<labels.size(); i++)
	{
	  if(labels.at(i)->isJumpTable)
	    printf("(jump table label): ");
	  printf("%s:\t%d\n", labels.at(i)->names.at(0), labels.at(i)->address);
	}
      printf("Jump table\n");
      for(i=0; i < jump_table.size(); i++)
	printf("%d: %d\n", (i * 4), jump_table[i]);
    }

  // fill in the symbol table for any unnamed registers
  for(; num_regs < num_system_regs; num_regs++)
    {
      symbol* temp = new symbol();      
      temp->names.push_back((char*)malloc(8));
      strcpy(temp->names.at(temp->names.size()-1), "unnamed");
      temp->address = num_regs;
      regs.push_back(temp);
    }    

  return num_regs;
}

int Assembler::handleLine(FILE *input, int pass, std::vector<Instruction*>& instructions, std::vector<symbol*>& labels, std::vector<symbol*>& regs, std::vector<int>& jump_table, std::vector<std::string>& ascii_literals)
{
  char line[1000];
  if(!fgets(line, 1000, input))
    return 1;
  char *token = strtok(line, delimeters);
  if(token==NULL)
    return 1;
  if(token[0]=='#')
    return 1;
  

  if(token[strlen(token)-1]==':') // label symbol
    {      
      token[strlen(token)-1] = '\0'; // remove the ':'
      if(pass==1)
	{
	  if(hasSymbol(token, labels)>=0 || hasSymbol(token, regs)>=0) // check for duplicates
	    {
	      printf("Assem: ERROR, symbol %s previously declared\n", token);
	      return 0;
	    }
	  symbol *l = new symbol();	  
	  l->names.push_back((char*)malloc(1000));
	  strcpy(l->names.at(l->names.size()-1), token);
	  l->address = num_instructions;
	  labels.push_back(l);
	}
      token = strtok(NULL, delimeters); // get rid of any empty space, so the instruction is next
    }
  if(token==NULL)
    {
      return 1;
    }
  // declaring a register name
  if(pass == 1 && strcmp(token, "REG")==0)
    {
      int vec = 1;
      token = strtok(NULL, delimeters);
      if(token[0]=='V' && token[1]=='E' && token[2]=='C')
	{
	  vec = atoi(token + 3);
	  token = strtok(NULL, delimeters);
	}
      if(hasSymbol(token, labels)>=0 || hasSymbol(token, regs)>=0)
	{
	  printf("Assem: ERROR, symbol %s previously declared\n", token);
	  return 0;
	}
      symbol *r = new symbol();
      r->names.push_back((char*)malloc(1000));      
      strcpy(r->names.at(r->names.size()-1), token);
      r->address = num_regs;
      regs.push_back(r);
      num_regs+=vec;
      return 1;
    }

  // instruction or other
  if(strcmp(token, "REG")!=0)
    {
      if(pass==1)
	{
	  // Jump tables
	  if(strcmp(token, ".long") == 0)
	    {
	      // if this is the first .long, set the address of the jump table
	      if(!labels[labels.size()-1]->isJumpTable)
		{
		  labels[labels.size()-1]->isJumpTable = true;
		  labels[labels.size()-1]->address = jtable_size;
		}
	      // Make space for the entry
	      jtable_size += 4; // local store is byte-addressed, assign 1 word for each entry

	      // We will add the entry to the table on the 2nd pass once all label addresses are known
	    }
	  // String literals
	  else if(strcmp(token, ".asciz") == 0)
	    {
	      // if this is the first .asciz, set the address of the string
	      // string addresses will be offset once the full jump table size is known
	      if(!labels[labels.size()-1]->isAscii)
		{
		  labels[labels.size()-1]->isAscii = true;
		  labels[labels.size()-1]->address = ascii_table_size;
		}
	      // Get the next token which will be the string itself
	      // Don't use normal delimeters since strings can contain them
	      token = strtok(NULL, "\t\"");
	      std::string newString(token);
	      // Convert "\n" to '\n' and so-forth
	      newString = escapedToAscii(newString);
	      // Add it to the table (string literals handled completely by first pass)
	      ascii_literals.push_back(newString);
	      // Make space for it
	      ascii_table_size += newString.length() + 1;
	    }
	  else
	    num_instructions++;
	}
      else // pass 2
	{
	  // check for ".long" jump table entries
	  if(strcmp(token, ".long") == 0)
	    {
	      token[strlen(token)] = ' '; // reconstitute the line (broken up by strtok)
	      int args[4];  
	      if(!getArgs(token, args, labels, regs) || args[1] != 0 || args[2] != 0 || args[3] != 0)
		{
		  printf("WARNING: failed to add jump table entry: %s. This likely means the source uses inheritance, which is not supported, and will fail.\n", token);
		  return 1;
		}
	      jump_table.push_back(args[0]); // add the label's address to the jump table
	    }
	  else if(strcmp(token, ".asciz") == 0)
	    {
	      // .asciz directives are all handled by the first pass
	      return 1;
	    }
	  else
	    {
	      token[strlen(token)] = ' '; // reconstitute the line (broken up by strtok)
	      if(!addInstruction(token, instructions, labels, regs))
		{
		  printf("failed to add instruction: %s\n", token);
		  return 0;
		}
	    }
	}
    }
  return 1;
}

int Assembler::hasSymbol(char *name, std::vector<symbol*>& syms)
{
  unsigned int i, j;
  for(i=0; i<syms.size(); i++)
    for(j=0; j<syms.at(i)->names.size(); j++)
      if(strcmp(syms.at(i)->names.at(j), name)==0)
	return (int)i;
  return -1;
}

int Assembler::addInstruction(char *line, std::vector<Instruction*>& instructions, std::vector<symbol*>& labels, std::vector<symbol*>& regs)
{
  int args[4];  
  if(!getArgs(line, args, labels, regs))
    {
      return 0;
    }
  Instruction::Opcode opcode = getOpcode(line);
  if(opcode==Instruction::NUM_OPS)
    {
      return 0;
    }
  instructions.push_back(new Instruction(opcode,
					 args[0],
					 args[1],
					 args[2], 
					 instructions.size()));
  return 1;
}

Instruction::Opcode Assembler::getOpcode(char *line)
{
  for (Instruction::Opcode i = Instruction::ADD; i < Instruction::NUM_OPS; i = (Instruction::Opcode)(i + 1)) {
    if (strcmp(line, Instruction::Opnames[i].c_str()) == 0)
      return i;
  }
  printf("ERROR - unrecognized instruction: %s\n", line);
  return Instruction::NUM_OPS;
}

int Assembler::getArgs(char *line, int* args, std::vector<symbol*>& labels, std::vector<symbol*>& regs)
{  
  char *token = strtok(line, delimeters);
  int i;
  for(i=0; i<4; i++)
    {
      token = strtok(NULL, delimeters);
      if(token=='\0')
	{
	  args[i] = 0;
	  continue;
	}
      if(isImmediateInt(token))
	{
	  args[i] = atoi(token);
	}
      else if(isImmediateFloat(token))
	{
	  union {
	    float f_val;
	    int i_val;
	  };
	  f_val = static_cast<float>( atof(token) );
	  args[i] = i_val;
	}
      else
	{
	  int key_address;
	  if(isKeyword(token, key_address))
	    {
	      args[i] = key_address;
	      continue;
	    }
	  int hasLabel = hasSymbol(token, labels);
	  int hasReg = hasSymbol(token, regs);
	  if(hasLabel<0 && hasReg<0)
	    {
	      printf("ERROR: undefined symbol: %s\n", token);
	      return 0;
	    }
	  
	  // at most one of these can be true
	  if(hasLabel>=0)
	    args[i] = labels.at(hasLabel)->address;
	  if(hasReg>=0)
	    {
	      int offset = 0;
	      int closing_brace = strlen(token)+2;
	      if(token + (closing_brace-1)!=NULL && token[closing_brace]==']')
		{
		  token[closing_brace] = '\0';
		  offset = atoi(token + (closing_brace-1));
		  token[closing_brace-1] = ' ';
		  token[closing_brace] = ' ';
		}
	      args[i] = regs.at(hasReg)->address + offset;
	    }
	}
    }
  return 1; 
}

int Assembler::isImmediateInt(char *token)
{
  unsigned int i;
  for(i=0; i<strlen(token); i++)
    if(((int)(token[i]) < 48 || (int)(token[i]) > 57) && token[i]!='-')
	return 0;
  return 1;
}

int Assembler::isImmediateFloat(char *token)
{
  unsigned int i;
  for(i=0; i<strlen(token); i++)
    if(((int)(token[i]) < 48 || (int)(token[i]) > 57) && (token[i]!='-' && token[i]!='.'))
	return 0;
  return 1;
}

int Assembler::isKeyword(char *token, int& value)
{
  if(strcmp(token, "start_wq")==0)
    {
      value = Assembler::start_wq;
      return 1;
    }
  if(strcmp(token, "start_framebuffer")==0)
    {
      value = Assembler::start_framebuffer;
      return 1;
    }
  if(strcmp(token, "start_camera")==0)
    {
      value = Assembler::start_camera;
      return 1;
    }
  if(strcmp(token, "start_scene")==0)
    {
      value = Assembler::start_scene;
      return 1;
    }
  if(strcmp(token, "start_light")==0)
    {
      value = Assembler::start_light;
      return 1;
    }
  if(strcmp(token, "start_bg_color")==0)
    {
      value = Assembler::start_bg_color;  
      return 1;
    }
  if(strcmp(token, "start_matls")==0)
    {
      value = Assembler::start_matls;  
      return 1;
    }
  if(strcmp(token, "start_permutation")==0)
    {
      value = Assembler::start_permutation;  
      return 1;
    }
  return 0;
}

// Converts strings containing literally escaped characters to their actual ascii equivalent,
// such as: 
// "a\nb" 
// becomes 
// "a
//  b"
std::string Assembler::escapedToAscii(std::string input)
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


