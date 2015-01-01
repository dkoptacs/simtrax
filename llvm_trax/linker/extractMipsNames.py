#!/usr/bin/python

# Use this to generate "mipsnames.txt", required by ln.py.
# Usage: pass llvm_trax/-3.5.xx/lib/Target/Mips/MipsGenAsmMatcher.inc to this program, after compiling llvm_trax

import sys
import re

filename = sys.argv[1]
newfile = open( filename, "rt" )

ops = []

for line in newfile:
    start = line.find(" /* ")
    end = line.find(" */, Mips::")
    if (start >= 0) and (end >= 0) and (end > start):
        if line[start+4:end] not in ops:
            ops.append(line[start+4:end])

for op in ops:
    print op

newfile.close()



