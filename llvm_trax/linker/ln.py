#!/usr/bin/python

# Read in a list of files and link them together.
# This linker as been modified to handle MIPS32 assembly.

import sys
import re
import os

#STACK_OFFSET = 1024
STACK_OFFSET = 8192

needsMemset = False
needsMemcpy = False
needsExtendsf = False

reg_list = ["$zero", "$at", "$gp", "$sp", "$fp", "$ra"]
opnames = []

mips_names = ["and",
              "or",
              "not",
              "sra", 
              "srl", 
              "xor",
              "c.ule.s"]

# All others will be commented out
keep_directives = [".byte",
                   ".2byte",
                   ".4byte",
                   ".space",
                   ".file",
                   ".loc",
                   ".section",
                   ".ascii",
                   ".text"]
                   #".globl",
                   #".data",
                   #".previous",
                   #".align",
                   #".type",
                   #".ent",
                   #".frame",
                   #".mask",
                   #".fmask",
                   #".set",
                   #".size"]
                   #".end"]

#------------------------------------------------------------------------------#

def main():
    global reg_list
    global opnames

    lnPath = "" + os.path.dirname(os.path.realpath(__file__))

    try:
        mipsNamesIn = open(lnPath + "/mipsnames.txt")
    except:
        sys.stderr.write('ln.py failed to open mipsnames.txt.\n')
        sys.exit()

    for line in mipsNamesIn:
        opnames.append("" + line.strip())

    mipsNamesIn.close()
        
    # print preamble
    sys.stdout.write('#BEGIN Preamble\n')

    # Append a list of the registers to the top of the assembly file.
    sys.stdout.write("\tREG\t$HI\n")   # <-- special register (not directly addressed by assembly)
    sys.stdout.write("\tREG\t$LO\n")   # <-- special register (not directly addressed by assembly)
    sys.stdout.write("\tREG\t$zero\n") # <-- always zero
    sys.stdout.write("\tREG\t$at\n")   # <-- assembler temporary

    # General purpose registers. The calling convention specifies roles
    # for different register groups (saved, temp, arguments, etc.), but
    # I'm not yet sure if LLVM emits assembly with the special names, or
    # if it always uses generic names for these registers.
    for r in xrange(1, 28):
        sys.stdout.write('\tREG\t$%d\n'%r)
        reg_list.append("${0}".format(r))

    sys.stdout.write("\tREG\t$gp\n")
    sys.stdout.write("\tREG\t$sp\n")
    sys.stdout.write("\tREG\t$fp\n")
    sys.stdout.write("\tREG\t$ra\n")

    # MIPS has 32 floating-point registers.
    for f in xrange(0, 32):
        sys.stdout.write('\tREG\t$f%d\n'%f)
        reg_list.append("$f{0}".format(f))

    sys.stdout.write('\tREG\t$fcc0\n')
    reg_list.append("$fcc0")

    
    # Set up the stack pointer. In MIPS, the value of $zero is always 0. That
    # is also the case in MicroBlaze.
    sys.stdout.write('.TRaX_START_PREAMBLE:\n')
    sys.stdout.write("\txor_m\t$zero, $zero, $zero\n")
    sys.stdout.write("\txor_m\t$gp, $gp, $gp\n")
    sys.stdout.write("\tLOADIMM\t$sp, {0}\n".format(STACK_OFFSET))

    # Add a jump to TRaX initialization 
    sys.stdout.write('\tbal\t$ra, .TRaX_INIT\n')
    sys.stdout.write('\tnop\n')
    
    # Set up a jump and return to main to start the program.
    sys.stdout.write('.start:\n')
    sys.stdout.write('\tbal\t$ra, main\n')
    sys.stdout.write('\tnop\n')
    sys.stdout.write('\tHALT\n')

    # the abort library call
    sys.stdout.write('abort:\n')
    sys.stdout.write('\taddi\t$5, $zero, -1\n')
    sys.stdout.write('\tPRINT\t$5\n')
    sys.stdout.write('\tHALT\n')
    sys.stdout.write('#END Preamble\n')
    # Need a label to separate the preamble text section
    sys.stdout.write('.TRaX_END_PREAMBLE:\n')

    fileid = 0

    LinkFile(sys.argv[1], fileid)
    fileid += 1

    if needsExtendsf:
        sys.stdout.write('#promote a float to a double\n')
        sys.stdout.write('#since TRaX does not support double, this does nothing\n')
        sys.stdout.write('#other 32 bits of the double will be undefined\n')
        sys.stdout.write('__extendsfdf2:\n')
        sys.stdout.write('\tFTOD $2 $3 $f12\n')
        sys.stdout.write('\tjr $ra\n')
        sys.stdout.write('\tnop\n')

    # Add a label for __stack_chk_guard, which contains 0
    # Simulator checks for stack safety on its own
    sys.stdout.write('__stack_chk_guard:\n')
    sys.stdout.write('.4byte 0\n')
    sys.stdout.write('__stack_chk_fail:\n')
    sys.stdout.write('jr $ra\n')
    sys.stdout.write('nop\n')

    # Add a label for TRaX initialization 
    sys.stdout.write('.TRaX_INIT:\n')
    sys.stdout.write('# .TRaX_INIT instructions are not emmited by compiler, but added by assembler\n')

    ## for filename in sys.argv[2:]:
    ##     if filename.find("memset.s") != -1:
    ##         if needsMemset == False:
    ##             continue
    ##     if filename.find("memcpy.s") != -1:
    ##         if needsMemcpy == False:
    ##             continue
    ##     if filename.find("__extendsfdf2.s") != -1:
    ##         if needsExtendsf == False:
    ##             continue
    ##     LinkFile(filename, fileid)
    ##     fileid += 1

#------------------------------------------------------------------------------#

def LinkFile(filename, fileid):


    #global needsMemset
    #global needsMemcpy
    global needsExtendsf
    global reg_list
    global keep_directives

    textEnd = False
    dataEnd = False

    reg = re.compile('[ |,|\t|\n|\r]')

    # for filename in sys.argv[1:]:
    sys.stderr.write('Reading %s.\n'%filename)
    try:
        f = open(filename)
    except:
        sys.stderr.write('ln.py failed to open %s.\n'%filename)
        sys.exit()

    # make a list of all the labels, and create a list with all of the
    # lines from the input file
    labels = []
    lines  = []
    for line in f.readlines():
        temp = line.strip()
        lines.append(line)
        if len(temp) < 2:
            continue
        if temp[0] != '#' and temp[-1] == ':':
            labels.append(temp[0:-1])

    f.close()

    for line in lines:

        temp = line.strip()

        if len(temp) > 0 and temp[0] == '#':
            continue

        # strip out unused debug info
        #if (textEnd == True) and (dataEnd == True):
         #   break

        if temp[0:10] == '$text_end:':
            textEnd = True
        if temp[0:10] == '$data_end:':
            dataEnd = True

        tokens = reg.split(line.strip())

        # keep .asciz entries for string literals
        if len(temp) > 0 and (temp[0] == '.' and temp[0:6] == '.asciz'):
            # remove excess whitespace
            endDirective = temp.find("z")
            startString = temp.find("\"")
            newline = temp[0:endDirective + 1] + "\t" + temp[startString:] + "\n"

        elif temp.find(".ctors") >= 0:
            newline = line
        
        # comment out directives but leave them in:
        #    comment out assignment directives "="
        #    keep .Nbyte entries for jtable
        #    keep debug source info (.file, .loc)
        #    keep labels ":"

        elif (len(temp) > 0) and (temp[0] == '.' and tokens[0] not in keep_directives):
            newline = '#' + line
        #elif (tokens[0] in keep_directives):
        #    newline = line
        
        #elif len(temp) > 0 and (((temp[0:6] not in keep_directives) or ((":" not in temp) and ("=" in temp)))):
        #elif (len(temp) > 0) and (temp[0:6] not in keep_directives):
         #   newline = '#' + line

        #elif temp[0:6] in keep_directives :
        #    newline = line
        
        else:

            newline = ''

            for tok in tokens:

                # ignore empty tokens
                if len(tok) < 1:
                    continue
                
                # check if we need to link external library calls
                ## if tok == "memset":
                ##     needsMemset = True
                ## if tok == "memcpy":
                ##     needsMemcpy = True
                if tok == "__extendsfdf2":
                    needsExtendsf = True

                # determine if the token is a label name
                is_label = False
                if tok[0] != '.' and tok[0] != '$':
                    for label in labels:
                        if label in tok:
                            is_label = True
                            break

                # replace periods with underscores, except in string literals
                if tok in opnames:
                    tok = tok.replace('.', '_')

                # append '_m' to conflicting MIPS instructions
                if tok in mips_names:
                    tok += "_m"



                

                # remove any comments
                #if tok[0] == '#':
                #    break

                # temp fix all jumps to be absolute
                #if tok[0] == '(' and tok[-1] == ')':
                #    tok = tok[1:-1]

                # uniquify labels
                ## if tok[0] == '$' and tok not in reg_list:
                ##     tok = tok.replace(tok[1:], "{0}{1}".format(
                ##         fileid, tok[1:]))
                    
                # uniquify interior labels
                ## for label in labels:
                ##     if label[0] == '$' and label in tok:
                ##         tok = tok.replace(label[1:], "{0}{1}".format(
                ##             fileid, label[1:]))

                # no tab before a label
                if tok[-1] == ':':
                    newline += tok
                else:
                    newline += '\t' + tok

            newline += '\n'

        # write the line
        sys.stdout.write(newline)

    # pass?
    

if __name__ == "__main__":
    main()
