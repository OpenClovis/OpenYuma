#!/usr/bin/env python
import sys
import os
import commands

# ----------------------------------------------------------------------------|
def DisplayUsage():
    print ( """Usage: sort_undefs.py filename.txt""" )
    sys.exit(-1)

# ----------------------------------------------------------------------------|
if __name__ == '__main__':
    if len ( sys.argv ) < 2 or sys.argv[1] == "--help":
        DisplayUsage()

    inFile = open ( sys.argv[1], 'r' )
    rawtext = inFile.readlines()

    undefinedFuncSet = set()
    for line in rawtext:
        if line.rfind( 'undefined reference to' ) != -1:
            startIdx = line.find( "`" )
            endIdx = line.find( "'", startIdx+1 )
            if startIdx and endIdx:
                name = line[ startIdx+1:endIdx]
                undefinedFuncSet.add( name )

    undefinedFuncs = sorted( undefinedFuncSet )

    print( "="*80 )
    print( "There are %d undefined functions:" % len(undefinedFuncs) )
    print( "-"*80 )
    for func in undefinedFuncs:
        print "\t%s" % func
    print( "-"*80 )
    print( "There are %d undefined functions!" % len(undefinedFuncs) )
    print( "="*80 )
