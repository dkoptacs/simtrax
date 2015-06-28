#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <queue>
#include <string>
#include <boost/regex.hpp>
#include "Assembler.h"
#include "Debugger.h"
#include "Profiler.h"

// Command regex matchers

extern std::vector<std::string> source_names;
extern std::vector< std::vector< std::string > > source_lines;

std::string expNext( "(next)|(n)" );
std::string expQuit( "(quit)|(q)" );
std::string expRun( "(run)|(r)" );

Debugger::Debugger()
{
  currentSourceLine = SourceInfo();
  currentAsmLine = 0;
  lastCommand = START;
  enabled = false;
}

void Debugger::setDwarfReader(const DwarfReader* _dwarfReader)
{
  dwarfReader = _dwarfReader;
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

  // loop until a "continue-type" command is entered (s, n, c, r). 
  do
    {
      switch(lastCommand)
	{
	case START:
	  printf( "TRaX debugger ready...\nType 'help' for a list of commands.\n" );
	  readCommand( thread, ins );
	  break;

	case RUN: // Nothing to do here, carry on
	  break;
	  
	case NEXT:
	  if(ins->srcInfo != currentSourceLine)
	    {
	      if(ins->srcInfo.fileNum < 0 || ins->srcInfo.fileNum >= source_names.size() ||
		   ins->srcInfo.lineNum < 0 || ins->srcInfo.lineNum > source_lines[ins->srcInfo.fileNum].size())
		{
		  printf("(tdb) (Source line unknown) %s\n", ins->asmLine.c_str());
		}
	      else
		{  
		  printf("(tdb) %s:%d\n", source_names[ins->srcInfo.fileNum].c_str(), 
			 ins->srcInfo.lineNum);
		  printf("      %d\t%s\n", ins->srcInfo.lineNum, source_lines[ins->srcInfo.fileNum][ins->srcInfo.lineNum - 1].c_str());
		}
	      readCommand(thread, ins);
	    }
	  break;
	}
    }
  while(!isExecutingCommand( lastCommand ));

  // Update state
  if(thread == NULL || ins == NULL) // handle first invocation
    currentSourceLine = SourceInfo();
  else if(ins->srcInfo != currentSourceLine)
    currentSourceLine = ins->srcInfo;
  
}

void Debugger::readCommand(ThreadState* thread, Instruction* ins)
{
  std::string command; 
  printf("(tdb) ");
  getline(std::cin, command);


  boost::smatch m;
  if(boost::regex_search(command, m, boost::regex(expNext)))
    {
      lastCommand = NEXT;
    }  
  else if(boost::regex_search(command, m, boost::regex(expRun)))
    {
      lastCommand = RUN;
    }
  else if(boost::regex_search(command, m, boost::regex(expQuit)))
    {
      lastCommand = QUIT;
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
