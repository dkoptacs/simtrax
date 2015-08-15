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

void Debugger::setLocalStore(const LocalStore* _ls_unit)
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
	  currentFrame->executionPoint = currentSourceInfo;
	  // Check for breakpoints
	  if( isBreakPoint(currentSourceInfo) )
	    {
	      printf( "Breakpoint reached\n" );
	      printSourceInfo(currentSourceInfo);
	      readCommand(thread, ins);
	    }
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
		printf("Can't find file \"%s\", breakpoint not set\n", filename.c_str());
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
	if( boost::regex_search(stripped, m, boost::regex(expSymbol)) )
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
		RuntimeNode* containingFrame = currentFrame->FindContainingFunction();

		if(containingFrame)
		  {
		    //CompilationUnit* instanceVar = containingFrame->GetVariable(m.str());
		    CompilationUnit* instanceVar;
		    CompilationUnit* typeUnit;
		    Location varLoc = findVariableUnits(instanceVar, typeUnit, containingFrame, m.str(), thread);
		    
		    if(instanceVar)
		      {
			int location = varLoc.value;
			std::string typeName;
			if(typeUnit)
			  {
			    typeName = typeUnit->name;
			    if(typeName == "int")
			      {			    
				int ivalToPrint = ((FourByte *)((char *)(ls_unit->storage[thread->thread_id]) + location))->ivalue;
				printf("%s (0x%x) = %d\n", m.str().c_str(), location, ivalToPrint);
			      }
			    else if(typeName == "float")
			      {
				float fvalToPrint = ((FourByte *)((char *)(ls_unit->storage[thread->thread_id]) + location))->fvalue;
				printf("%s (0x%x) = %f\n", m.str().c_str(), location, fvalToPrint);
			      }
			    else
			      printf("%s (0x%x) (unknown type)\n", m.str().c_str(), location);
			  }
			else
			  {
			    printf("%s (0x%x) (unknown type)\n", m.str().c_str(), location);
			  }
		      }
		    else
		      printf("can't find symbol: %s\n", m.str().c_str());
		  }
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
      if(!thread->ReadRegister(dwOpToRegId(varLoc.value), 90000000000, arg, failop))
	{
	  printf("Unable to read register %d\n", dwOpToRegId(varLoc.value));
	}
      else
	retVal.value = arg.idata;
    }
  else if(varLoc.type == Location::FRAME_OFFSET)
    {
      if(base.type != Location::REGISTER)
	{
	  printf("Error: Unsupported frame base type %d\n", base.type);
	  exit(1);
	}

      if(!thread->ReadRegister(dwOpToRegId(base.value), 90000000000, arg, failop))
	{
	  printf("Unable to read register %d\n", dwOpToRegId(varLoc.value));
	}
      else
	{
	  retVal.value =  arg.idata + varLoc.value;
	}
    }
  else if(varLoc.type == Location::MEMBER_OFFSET)
    {
      retVal.value =  varLoc.value + base.value;
    }

  return retVal;
}


Location Debugger::findVariableUnits(CompilationUnit* &instanceUnit, CompilationUnit* &typeUnit, RuntimeNode* frame, std::string name, ThreadState* thread)
{
  std::string firstName;
  std::string remainingName = name;
  std::string prefix;
  instanceUnit = NULL;
  typeUnit = NULL;
  Location retVal;
  retVal.value = -1;

  do {
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
    if(!instanceUnit)
      {
	instanceUnit = frame->GetVariable(firstName);
	if(instanceUnit->typeUnit)
	  typeUnit = instanceUnit->typeUnit;
	else
	  {
	    printf("WARNING: Couldn't find type unit for variable %s\n", name.c_str());
	    break;
	  }
      }
    else
      {
	if(typeUnit)
	  {
	    instanceUnit = typeUnit->SearchVariableDown(prefix + firstName);
	    if(instanceUnit && instanceUnit->typeUnit) // really bad names here. clean this up
	      typeUnit = instanceUnit->typeUnit;
	  }
	if(!typeUnit)
	  {
	    printf("WARNING: Couldn't find type unit for variable %s\n", name.c_str());
	    break;
	  }
      }
    if(typeUnit)
      {
	if(retVal.value == -1)
	  retVal = evaluateLocation(thread, instanceUnit->loc, frame->source_node->loc);
	else
	  retVal = evaluateLocation(thread, instanceUnit->loc, retVal);
	prefix = typeUnit->name + "::";
      }
  }
  while( remainingName.length() > 0 );
  
  return retVal;

}
