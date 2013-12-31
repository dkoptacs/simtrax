#!/usr/bin/python

#Read in a list of files and link them together.

import sys

#STACK_OFFSET = 1024
STACK_OFFSET = 8192

needsMemset = False
needsExtendsf = False

def main():
    #print preamble
    sys.stdout.write('#BEGIN Preamble\n')
    for r in xrange(32):
        sys.stdout.write('\tREG\tr%d\n'%r)
    sys.stdout.write('\tLOADIMM\tr0, 0\n')
    # change this value if the stack needs to be bigger
    sys.stdout.write('\tADDI\tr1, r0, %d\n'%STACK_OFFSET)
    # Set up a jump and return to it to start the program
    sys.stdout.write('start:\tbrlid\tr15, main\n')
    sys.stdout.write('\tNOP\n')
    sys.stdout.write('\tHALT\n')
    # the abort library call
    sys.stdout.write('abort:\tADDI\tr5, r0, -1\n')
    sys.stdout.write('\tPRINT\tr5\n')
    sys.stdout.write('\tHALT\n')
    sys.stdout.write('#END Preamble\n')
    

    fileid = 0

    LinkFile(sys.argv[1], fileid)
    fileid += 1
    
    for filename in sys.argv[2:]:
        if filename.find("memset.s") != -1:
            if needsMemset == False:
                continue
        if filename.find("__extendsfdf2.s") != -1:
            if needsExtendsf == False:
                continue
        LinkFile(filename, fileid)
        fileid += 1

def LinkFile(filename, fileid):

    global needsMemset
    global needsExtendsf

    #for filename in sys.argv[1:]:
    sys.stderr.write('Reading %s.\n'%filename)
    try:
        f = open(filename)
    except:
        sys.stderr.write('ln.py failed to open %s.\n'%filename)
        sys.exit()
    for line in f.readlines():
        temp = line.strip()
        if len(temp) > 0 and temp[0] == '#':
                #remove comments
            continue
        
        
        # keep .asciz entries for string literals 
        if len(temp) > 0 and (temp[0] == '.' and temp[0:5] == '.asci'):
            # remove excess whitespace
            endDirective = temp.find("z")
            startString = temp.find("\"")
            newline = temp[0:endDirective + 1] + "\t" + temp[startString:] + "\n"

        # comment out directives but leave them in:
        #    comment out assignment directives "="
        #    keep .long entries for jtable
        #    keep labels ":"
        elif len(temp) > 0 and ((temp[0] == '.' and temp[0:5] != '.long') or ((":" not in temp) and ("=" in temp))):
            newline = '#' + line
        else:
            tokens = line.strip().split()
            
            newline = ''
            
            for tok in tokens:
                # Check if we need to link external library calls
                if tok == "memset":
                    needsMemset = True
                if tok == "__extendsfdf2":
                    needsExtendsf = True

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
    #pass

if __name__ == "__main__":
    main()
