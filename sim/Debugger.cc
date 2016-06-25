#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <queue>
#include <string>
#include <stdlib.h>
#include <boost/regex.hpp>
#include "Assembler.h"
#include "Debugger.h"
#include "Profiler.h"
#include "LocalStore.h"
// Command regex matchers

extern std::vector<std::string> source_names;
extern std::vector< std::vector< std::string > > source_lines;


std::string expFilename( "[^( :)]+" ); // pretty loose definition here
std::string expInteger( "[0-9]+" ); 
std::string expNext( "(next)|(n)" );
std::string expQuit( "(quit)|(q)" );
std::string expBreak( "((break)|(b))");
std::string expBreakFile( " " + expFilename + ":" + expInteger );
std::string expRun( "(run)|(r)|(continue)|(c)" );
std::string expPrint( "((print)|(p))" );
std::string expInfo( "((info)|(i))" );
std::string expBacktrace( "((backtrace)|(bt))" );
std::string expHexLiteral( "0x[0-9a-fA-F]+" );
std::string expWatch( "((watch)|(w))[\\s]+");

Debugger::Debugger()
{
  currentSourceInfo = SourceInfo();
  currentAsmLine = 0;
  lastCommand = START;
  enabled = false;
  currentFrame = NULL;
  reg0Id = -1;
}

void Debugger::setDwarfReader(DwarfReader* _dwarfReader)
{
  dwarfReader = _dwarfReader;
  currentFrame = NULL;
}

void Debugger::setRegisterSymbols(const std::vector<symbol*>* _regs)
{
  regs = _regs;
  // $at is treated as reg0, since MIPS register names begin at r1
  reg0Id = Assembler::HasSymbol("$at", *regs);
}

void Debugger::setLocalStore(LocalStore* _ls_unit)
{
  ls_unit = _ls_unit;
}

void Debugger::enable()
{
  enabled = true;
}

void Debugger::disable()
{
  enabled = false;
  ls_unit->watchpoints.clear();
}

bool Debugger::isEnabled()
{
  return enabled;
}

// Handles a single control takeover of the debugger
// Either reads a command from the keyboard or processes the current execution/debug state
// To be invoked every time the IssueUnit issues an instruction
void Debugger::run(ThreadState* thread, Instruction* ins)
{
  
  if(!enabled)
    return;

  currentFrame = dwarfReader->UpdateRuntime(ins, currentFrame);

  // loop until a "continue-type" command is entered (s, n, c, r). 
  do
    {
      switch(lastCommand)
	{
	case START:
	  printf( "TRaX debugger ready...\nType 'help' for a list of commands.\n" );
	  readCommand( thread, ins );
	  break;

	case NONE:
	  readCommand( thread, ins );
	  break;

	case RUN: // Nothing to do here, carry on
	  break;
	  
	case NEXT:
	  if(ins->srcInfo != currentSourceInfo)
	    {
	      currentFrame->executionPoint = ins->srcInfo;
	      if(ins->srcInfo.fileNum < 0 || ins->srcInfo.fileNum >= source_names.size() ||
		   ins->srcInfo.lineNum < 0 || ins->srcInfo.lineNum > source_lines[ins->srcInfo.fileNum].size())
		{
		  printf("(tdb) (Source line unknown) %s\n", ins->asmLine.c_str());
		}
	      else
		{  
		  printSourceInfo(ins->srcInfo);
		}
	      readCommand(thread, ins);
	    }
	  break;
	}
    }
  while(!isExecutingCommand( lastCommand ));

  // Update state
  if(thread == NULL || ins == NULL) // handle first invocation
    currentSourceInfo = SourceInfo();
  else
    { 
      if(ins->srcInfo != currentSourceInfo)
	{
	  currentSourceInfo = ins->srcInfo;
	  currentFrame->UpdateExecutionPoint(currentSourceInfo);
	  // Check for breakpoints
	  if( isBreakPoint(currentSourceInfo) )
	    {
	      printf( "Breakpoint reached\n" );
	      printSourceInfo(currentSourceInfo);
	      readCommand(thread, ins);
	    }
	}
      if(ls_unit->watchPointHit){
	ls_unit->watchPointHit = false;
	printSourceInfo(currentSourceInfo);
	readCommand(thread, ins);
      }
    }
}

void Debugger::readCommand(ThreadState* thread, Instruction* ins)
{
  std::string command; 
  printf("(tdb) ");
  getline(std::cin, command);

  if(command.length() == 0)
    if(lastEntry.length() != 0)
      command = lastEntry;

  lastEntry = command;

  boost::smatch m;
  if(boost::regex_match(command, m, boost::regex(expNext)) && m.position() == 0)
    {
      lastCommand = NEXT;
    }  
  else if(boost::regex_match(command, m, boost::regex(expRun)) && m.position() == 0)
    {
      lastCommand = RUN;
    }
  else if(boost::regex_match(command, m, boost::regex(expQuit)) && m.position() == 0)
    {      
      lastCommand = QUIT;
    }
  else if(boost::regex_search(command, m, boost::regex(expBacktrace)) && m.position() == 0)
    { 
      lastCommand = BACKTRACE;
    }
  else if(boost::regex_search(command, m, boost::regex(expBreak)) && m.position() == 0)
    {
      lastCommand = BREAK;
    }
  else if(boost::regex_search(command, m, boost::regex(expPrint)) && m.position() == 0)
    { 
      lastCommand = PRINT;
    }
  else if(boost::regex_search(command, m, boost::regex(expInfo)) && m.position() == 0)
    {
      lastCommand = INFO;
    }
  else if(boost::regex_search(command, m, boost::regex(expWatch)) && m.position() == 0)
    {
      lastCommand = WATCH;
    }
  else if(command.size() != 0) // If no other commands were recognized
    {
      lastCommand = NONE;
    }

  switch(lastCommand)
    {
    case NEXT:
      break;
    case RUN:
      break;
    case QUIT:
      printf("(tdb) Debugger quitting. Resuming simulation...\n");
      disable();
      break;
    case BREAK:
      {
	// Read the rest of the break command
	std::string stripped = command.substr(command.find(m.str()) + m.str().length());
	if( boost::regex_search(stripped, m, boost::regex(expBreakFile)) )
	  {
	    boost::regex_search( stripped, m, boost::regex(expFilename) );
	    std::string filename = m.str();
	    stripped = stripped.substr(stripped.find(m.str()) + m.str().length() + 1); // +1 to get rid of the ':'
	    boost::regex_search( stripped, m, boost::regex(expInteger) );
	    int linenum = atoi( m.str().c_str() );
	    // find the filename
	    int fileIndex = findSourceFile(filename);
	    
	    if( fileIndex == -1 )
	      {
		printf("Can't find symbols for file \"%s\", breakpoint not set\n", filename.c_str());
	      }
	    else
	      {
		SourceInfo break_loc;
		break_loc.fileNum = fileIndex;
		break_loc.lineNum = linenum;
		BreakPoint b;
		b.stop_at = break_loc;
		breakPoints.push_back( b );
	      }
	  }
	lastCommand = NONE;
      }
      break;
    case WATCH:
      {
	bool foundAddress = false;
	// Read the rest of the watch command
	std::string stripped = command.substr(command.find(m.str()) + m.str().length());
	// Check for a hard-coded address watch point
	if( boost::regex_search(stripped, m, boost::regex(expHexLiteral)) ){
	    long address = strtol(m.str().c_str() + 2, NULL, 16);
	    ls_unit->AddWatchPoint(thread, address);
	    printf("added watch to address %d\n", address);
	    foundAddress = true;
	  }
	else{
	  std::string typeName;
	  Location varLoc = findVariable(currentFrame, stripped, thread, typeName);
	  if(varLoc.value >= 0){
	    ls_unit->AddWatchPoint(thread, varLoc.value);
	    // TODO: This just uses the raw address. 
	    //       Add variable information to watchpoints so the location of the watchpoint can be dynamic with respect to the variable.
	    //       This would require tracking which frame the variable is in, and its relative location in that frame.
	    printf("added watch to address %d\n", varLoc.value);
	    foundAddress = true;
	  }
	}
	if(!foundAddress)
	  printf("Unknown watch address or symbol: %s\n", stripped.c_str());
	lastCommand = NONE;
      }
      break;
    case PRINT:
      {
	if( !isInitialized() )
	  {
	    printf("Debugger unable to print symbols before any execution has occured\n");
	    readCommand(thread, ins);
	    break;
	  }
	// Read the rest of the print command
	std::string stripped = command.substr(command.find(m.str()) + m.str().length());

	if( boost::regex_search(stripped, m, boost::regex(expHexLiteral))){
	  long address = strtol(m.str().c_str() + 2, NULL, 16);
	  if(address < 0 || address >= LOCAL_SIZE)
	    printf("Can't print invalid address (not in [0 .. %d)\n", LOCAL_SIZE);
	  else{
	    int      ivalToPrint = ((FourByte *)((char *)(ls_unit->storage[thread->thread_id]) + address))->ivalue;
	    unsigned uvalToPrint = ((FourByte *)((char *)(ls_unit->storage[thread->thread_id]) + address))->uvalue;
	    float    fvalToPrint = ((FourByte *)((char *)(ls_unit->storage[thread->thread_id]) + address))->fvalue;
	    printf("0x%x: = %di %uu %ff\n", address, ivalToPrint, uvalToPrint, fvalToPrint);
	  }
	}

	else if( boost::regex_search(stripped, m, boost::regex(expSymbol)) )
	  {
	    int symbId = Assembler::HasSymbol(m.str(), *regs);
	    if(symbId >= 0) // Look for a register name
	      {
		reg_value arg;
		Instruction::Opcode failop = Instruction::NOP;
		int regAddr = regs->at(symbId)->address;
		// We don't care about ready_cycle, we just want to read whatever is in the register, 
		// so just pass in a huge number so it always reads
		if(!thread->ReadRegister(regAddr, 90000000000, arg, failop))
		  {
		    printf("Unable to read register %s\n", m.str().c_str());
		  }
		else
		  {
		    printf("<Thread: %p> Register %u (\"%s\") has value %d, %u, %f\n",
			   thread, regAddr, m.str().c_str(), arg.idata, arg.udata, arg.fdata);
		  }
	      }
	    else // Look for a source variable name
	      {
		std::string typeName;
		Location varLoc = findVariable(currentFrame, m.str(), thread, typeName);

		if(varLoc.value >= 0)
		  {
		      int location = varLoc.value;
		      FourByte* value = ((FourByte *)((char *)(ls_unit->storage[thread->thread_id]) + location));
		      if(typeName == "int")
			{			    
			  int ivalToPrint = value->ivalue;
			  printf("%s (0x%x) = %d\n", m.str().c_str(), location, ivalToPrint);
			}
		      else if(typeName == "char")
			{			    
			  int ivalToPrint = value->ivalue;
			  printf("%s (0x%x) = %d(\'%c\')\n", m.str().c_str(), location, (char)(ivalToPrint & 0xff), (char)(ivalToPrint & 0xff));
			}
		      else if(typeName == "float")
			{
			  float fvalToPrint = value->fvalue;
			  printf("%s (0x%x) = %f\n", m.str().c_str(), location, fvalToPrint);
			}
		      else if(typeName == "bool")
			{
			  int ivalToPrint = value->ivalue;
			  printf("%s (0x%x) = ", m.str().c_str(), location);
			  if(ivalToPrint == 0)
			    printf("false\n");
			  else
			    printf("true\n");
			}
		      else
			printf("%s (0x%x) (unknown type %s)\n", m.str().c_str(), location, typeName.c_str());
		  }
		else 
		  printf("Can't find symbol %s\n", m.str().c_str());
	      }
	  }
	lastCommand = NONE;
      }
      break;
    case INFO:
      //TODO: Handle extra args to info, like "break"
      printf("PC: %d\n", thread->program_counter); 
      lastCommand = NONE;
      break;
    case BACKTRACE:
      currentFrame->PrintBacktrace();
      lastCommand = NONE;
      break;
    case NONE:
      printf("(tdb) Unknown command: %s\n", command.c_str());
      break;
    }
}


Location
Debugger::findVariable(RuntimeNode* currentFrame, std::string name, ThreadState* thread, std::string& typeName)
{

  Location retval;
  if(!currentFrame)
    return retval;

  CompilationUnit* containingFrame = currentFrame->source_node;
  Location baseLoc = containingFrame->loc;
  if(baseLoc.type == Location::UNDEFINED)
    baseLoc = containingFrame->FindContainingFunction()->loc;

  CompilationUnit* instanceVar;
  CompilationUnit* typeUnit;
  retval = findVariableLocation(containingFrame, name, thread, baseLoc, typeName);

  return retval;
}

bool Debugger::isExecutingCommand(int command )
{
  switch( command )
    {
    case NEXT:
    case CONTINUE:
    case STEP:
    case QUIT:
    case RUN:
      return true;
    default:
      return false;
    }
}

bool Debugger::isBreakPoint(SourceInfo si)
{
  for(size_t i = 0; i < breakPoints.size(); i++)
    if( si == breakPoints[i].stop_at )
      return true;
  return false;
}


void Debugger::printSourceInfo(SourceInfo si)
{
  printf("(tdb) %s:%d\n", source_names[si.fileNum].c_str(), 
	 si.lineNum);
  if(si.lineNum > 0)
    printf("      %d\t%s\n", si.lineNum, source_lines[si.fileNum][si.lineNum - 1].c_str());
}


int Debugger::findSourceFile(std::string name)
{
  size_t i = 0;
  for( ; i < source_names.size(); i++ )
    {
      int lastSlash = source_names[i].rfind("/");
      if(lastSlash == std::string::npos)
	lastSlash = 0;
      else
	lastSlash++; // get past the '/' character
      std::string stripped = source_names[i].substr(lastSlash);
      if( stripped == name )
	return i;
    }
  return -1;
}

bool Debugger::isInitialized()
{
  return currentSourceInfo.fileNum != -1;
}

Location Debugger::evaluateLocation(ThreadState* thread, Location varLoc, Location base)
{
  Location retVal;
  retVal.value = -1;
  reg_value arg;
  Instruction::Opcode failop = Instruction::NOP;
  if(varLoc.type == Location::REGISTER)
    {
      // TODO: Move dwOpToRegId in to the dwarf reader, and save the register number as the value, instead of the DW opcode for it.
      if(!thread->ReadRegister(dwOpToRegId(varLoc.value), 90000000000, arg, failop))
	{
	  printf("Unable to read register %d\n", dwOpToRegId(varLoc.value));
	}
      else
	retVal.value = arg.idata;
    }
  else if(varLoc.type == Location::FRAME_OFFSET)
    {
      if(base.type == Location::REGISTER)
	{ 
	  if(!thread->ReadRegister(dwOpToRegId(base.value), 90000000000, arg, failop))
	    {
	      printf("Unable to read register %d\n", dwOpToRegId(varLoc.value));
	    }
	  else
	    {
	      retVal.value =  arg.idata + varLoc.value;
	    }
	}
      else
	{
	  // TODO: We need to handle more types here. I'm assuming all other types are just a uconst stored in the base location.
	  //       Will need to define a Location::type that encodes uconst numbers
	  if(base.type != Location::UNDEFINED){
	    printf("error: Unhandled base location type in FRAME_OFFSET\n");
	    exit(1);
	  }
	  retVal.value =  varLoc.value + base.value; 
	}
    }
  else if(varLoc.type == Location::MEMBER_OFFSET)
    {
      retVal.value =  varLoc.value + base.value;
    }
  else
    printf("Unknown location\n");

  return retVal;
}



Location Debugger::findVariableLocation(CompilationUnit* containingFrame, std::string name, ThreadState* thread, Location baseLoc, std::string& type)
{
  std::string firstName;
  std::string remainingName = name;
  Location notFound;
  
  // Strip out each object's name
  int separatorIndex = remainingName.find(".");
  if(separatorIndex < 0)
    {
      firstName = remainingName;
      remainingName = "";
    }
  else
    {
      firstName = remainingName.substr(0, separatorIndex);
      remainingName = remainingName.substr(separatorIndex + 1);	
    } 

  CompilationUnit* instanceUnit = containingFrame->GetVariable(firstName);
  if(instanceUnit){
    CompilationUnit* typeUnit = instanceUnit->typeUnit;
    int objectAddr = 0;
    if(typeUnit){
      if(typeUnit->tag == DW_TAG_reference_type || typeUnit->tag == DW_TAG_pointer_type){
	// Find the location of the pointer
	Location ref_loc = evaluateLocation(thread, instanceUnit->loc, baseLoc);
	// Then get the pointer itself
	if(ref_loc.type == Location::REGISTER)
	  objectAddr = readRegister(ref_loc, thread);
	else
	  objectAddr = readStack(ref_loc, thread);
	baseLoc.value = objectAddr;
	baseLoc.type = Location::UNDEFINED;

	// TODO: This may only work for "this" pointers
	if(typeUnit->tag == DW_TAG_pointer_type){
	  //typeUnit = typeUnit->typeUnit;//->GetVariable(firstName);
	  remainingName = name;
	}

	if(remainingName.length() == 0)
	  return baseLoc;
	//return evaluateLocation(thread, instanceUnit->loc, baseLoc);
	//return findVariableLocation(typeUnit, remainingName, thread, baseLoc, type);
      }
    }
    if(!typeUnit)
      return notFound;
    
    if( remainingName.length() != 0 ){
      if(typeUnit->tag != DW_TAG_reference_type && typeUnit->tag != DW_TAG_pointer_type){
	baseLoc = evaluateLocation(thread, instanceUnit->loc, baseLoc);
      }
      
      return findVariableLocation(typeUnit, remainingName, thread, baseLoc, type);
    }
    
    type = typeUnit->name;
    return evaluateLocation(thread, instanceUnit->loc, baseLoc);
  }
  printf("Unable to find instance unit for variable %s\n", firstName.c_str());
  return notFound;
}


int Debugger::readRegister(const Location& loc, ThreadState* thread)
{
  reg_value arg;
  Instruction::Opcode failop = Instruction::NOP;
  // We don't care about ready_cycle, we just want to read whatever is in the register, 
  // so just pass in a huge number so it always reads
  if(!thread->ReadRegister(dwOpToRegId(loc.value), 90000000000, arg, failop)){
    printf("failed to read register from dwarf location\n");
    exit(1);
  }
  return arg.idata;
}

int Debugger::readStack(const Location& loc, ThreadState* thread)
{
  return ((FourByte *)((char *)(ls_unit->storage[thread->thread_id]) + loc.value))->ivalue;	
}
