#!/usr/bin/python

#cleans \01 from a .ll file

import sys

def main():
    filename = sys.argv[1]
    try:
        f = open(filename)
    except:
        sys.stderr.write('Failed to open %s.\n'%filename)
        sys.exit()
    for line in f.readlines():
        print line.replace('\\01','').strip('\n')

if __name__ == "__main__":
    main()
