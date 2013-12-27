#ifndef __SIMHWRT_ASSEMBLER_H_
#define __SIMHWRT_ASSEMBLER_H_
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Instruction.h"
struct symbol
{
  std::vector<char*> names;
  int address;
  bool isJumpTable;

symbol() : 
  isJumpTable(false)
  {}
};

class Assembler
{
 public:
  static int start_wq, start_framebuffer, start_camera, start_scene, start_light, start_bg_color, start_matls, start_permutation, num_instructions, num_regs;
  static int LoadAssem(char *filename, std::vector<Instruction*>& instructions, std::vector<symbol*>& regs, int num_system_regs, std::vector<int>& jump_table, int _start_wq, int _start_framebuffer, int _start_camera, int _start_scene, int _start_light, int _start_bg_color, int start_permutation, int start_matls, bool print_symbols);
 private:
  static int handleLine(FILE *input, int pass, std::vector<Instruction*>& instructions, std::vector<symbol*>& labels, std::vector<symbol*>& regs, std::vector<int>& jump_table);
  static int hasSymbol(char *name, std::vector<symbol*>& syms);  
  static int addInstruction(char *line, std::vector<Instruction*>& instructions, std::vector<symbol*>& labels, std::vector<symbol*>& regs);
  static int getArgs(char *line, int* args, std::vector<symbol*>& labels, std::vector<symbol*>& regs);
  static Instruction::Opcode getOpcode(char *line);
  static int isImmediateInt(char *token);
  static int isImmediateFloat(char *token);
  static int isKeyword(char *token, int& value);
  static int makeAlias(char *word1, char *word2, std::vector<symbol*>& labels, std::vector<symbol*>& regs);
  static int unlink(char *word, std::vector<symbol*>& regs);
};
#endif
