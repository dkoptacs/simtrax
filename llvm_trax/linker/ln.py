#!/usr/bin/python

#Read in a list of files and link them together.

import sys

#STACK_OFFSET = 1024
STACK_OFFSET = 8192

def main():
    #print preamble
    sys.stdout.write('#BEGIN Preamble\n')
    for r in xrange(32):
        sys.stdout.write('\tREG\tr%d\n'%r)
    sys.stdout.write('\tLOADIMM\tr0, 0\n')
    # change this value if the stack needs to be bigger
    sys.stdout.write('\tADDI\tr1, r0, %d\n'%STACK_OFFSET)
    # Compiler generates _Z9trax_mainv as main label. Set up a jump and return to it
    sys.stdout.write('start:\tbrlid\tr15, _Z9trax_mainv\n')
    sys.stdout.write('\tNOP\n')
    sys.stdout.write('\tHALT\n')
    # the abort library call
    sys.stdout.write('abort:\tADDI\tr5, r0, -1\n')
    sys.stdout.write('\tPRINT\tr5\n')
    sys.stdout.write('\tHALT\n')
    sys.stdout.write('#END Preamble\n')
    
    fileid = 0
    for filename in sys.argv[1:]:
        sys.stderr.write('Reading %s.\n'%filename)
        try:
            f = open(filename)
        except:
            sys.stderr.write('Failed to open %s.\n'%filename)
            sys.exit()
        for line in f.readlines():
            temp = line.strip()
            if len(temp) > 0 and temp[0] == '#':
                #remove comments
                continue
            if len(temp) > 0 and temp[0] == '.' and temp[0:5] != '.long':
                #comment out directives but leave them in
                newline = '#' + line
            else:
                tokens = line.strip().split()

                newline = ''

                for tok in tokens:
                    #remove any comments
                    if tok[0] == '#':
                        break
                    #temp fix all jumps to be absolute
                    if tok[0] == '(' and tok[-1] == ')':
                        tok = tok[1:-1]
                    #uniquify labels
                    if tok[0] == '$':
                        tok = tok[:1] + str(fileid) + tok[1:]
                    #no tab before a label
                    if tok[-1] == ':':
                        newline += tok
                    else:
                        newline += '\t' + tok

                newline += '\n'

            # write the line
            sys.stdout.write(newline)
        fileid += 1
    pass

if __name__ == "__main__":
    main()
