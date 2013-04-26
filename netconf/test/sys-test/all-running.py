#!/usr/bin/env python
import sys
import os
import commands

# ----------------------------------------------------------------------------|
def GetTestProgramList():
    fileList=[]
    for dirname, dirnames, filenames in os.walk('.'):
        for filename in filenames:
            if filename.startswith( "test-" ) and os.access(filename, os.X_OK) and "running" in filename:
                fileList.append( "./" + filename )
    return fileList

# ----------------------------------------------------------------------------|
def RunTests( tests, argv  ):
    testArgs=""

    for arg in argv[1:]:
        testArgs += arg + " " 

    for test in tests:
        testCommand = test + " " + testArgs
        print "running test %s" % testCommand

        # commands.getoutput( test )
        os.system( testCommand )

# ----------------------------------------------------------------------------|
if __name__ == '__main__':
    # find the executables
    tests = GetTestProgramList()

    # run all executables
    RunTests( tests, sys.argv )
