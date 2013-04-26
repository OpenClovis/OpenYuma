#!/usr/bin/env python
import sys
import os
import commands
import re

# ----------------------------------------------------------------------------|
YumaRootDir = "../../../netconf"
YumaModuleDir = YumaRootDir + "/modules"
YumaModPath = "YUMA_MODPATH=" + YumaModuleDir
YangdumpExe = YumaRootDir + "/target/bin/yangdump"
LdLibraryPath = "LD_LIBRARY_PATH=" + YumaRootDir + "/target/lib"
YangEnvSettings = LdLibraryPath + " " + YumaModPath + " " 
TestOutputDir = "./yangdump-op"

# ----------------------------------------------------------------------------|
def InitialiseOutputDir():
    """Create / Clean the test output directory"""
    commands.getoutput( "mkdir -p " + TestOutputDir )
    commands.getoutput( "rm -rf " + TestOutputDir+"/*" )

# ----------------------------------------------------------------------------|
def RunYangDump( fmt ):
    """Run Yangdump over the yang files generating output in the
    requested format"""
    command = ( YangEnvSettings + YangdumpExe + " "
                + "subtree=" + YumaModuleDir+ "/ietf "
                + "subtree=" + YumaModuleDir+ "/netconfcentral "
                + "subtree=" + YumaModuleDir+ "/yang "
                + "subtree=" + YumaModuleDir+ "/test/pass "
                + "output=" + TestOutputDir + " "
                + "format="+fmt + " "
                + "defnames=true "
                + "log=" + TestOutputDir+"/test-"+fmt+".log "
                + "log-level=debug" )
    print "Running command: %s" % command
    commands.getoutput( command )

# ----------------------------------------------------------------------------|
def CountOccurrences ( searchText, data, ignoreCase = True ):
    """Crude count of the number of occurrences of searchText in data"""
    if ignoreCase:
        res = [ m.start() for m in re.finditer( searchText, data, re.IGNORECASE) ]
    else:
        res = [ m.start() for m in re.finditer( searchText, data ) ]
    return len( res )

# ----------------------------------------------------------------------------|
def SummariseErrorsAndWarnings( data ):
    """Search for the line '*** # Errors, # Warnings' in the output file and
    extract the count of errors and warnings reported"""
    regex = re.compile( "\*\*\* (\\d+) Errors, (\\d+) Warnings" )
    errors = 0
    warnings = 0
    for m in re.findall( regex, data ):
        errors += int( m[0] )
        warnings += int( m[1] )
    return (errors, warnings)

# ----------------------------------------------------------------------------|
def AnalyseOutput( fmt ):
    """Analyse the output log file for the specified yangdump format"""
    filename = TestOutputDir + "/test-" + fmt + ".log"
    print "Analysing Results From %s" % filename
    f = open( filename, "r" )
    data = f.read();

    # get the number of errors and warnings
    errors, warnings = SummariseErrorsAndWarnings( data )

    # get the number of Segmentation Violations
    # Note this is a based on the original makefile script, which
    # simply greps for 'Segmentation' - it is assumed that any
    # occurrences indicates a Segmentation Fault
    segmentCount = CountOccurrences( "Segmentation", data )

    # get the number of occurrences of 'internal'
    internalCount = CountOccurrences( "internal", data )

    return ( errors, warnings, segmentCount, internalCount ) 

# ----------------------------------------------------------------------------|
def TestFormat( fmt ):
    """Test the specified format and collate the results"""
    RunYangDump( fmt )
    return AnalyseOutput( fmt )

# ----------------------------------------------------------------------------|
def TestYang():
    results = {}
    for fmt in [ "yin", "yang", "xsd", "sqldb", "html", "h", "c" ]:
        result = TestFormat( fmt )
        results[ fmt ] = result
    return results

# ----------------------------------------------------------------------------|
def DisplayResults( results ):
    colWid = 15
    tabWid = 80
    print "\n"
    print "="*tabWid
    print " The Results of running yangdump are summarised in the table below."
    print "-"*tabWid
    print ( " %s %s %s %s %s" % ( "Format".ljust(colWid), 
                                 "Errors".center(colWid),
                                 "Warnings".center(colWid),
                                 "Seg-Faults".center(colWid),
                                 "Internal".center(colWid) ) )
    totalErrors = 0
    totalInternal = 0
    totalSegFaults = 0
    warningHighWaterMark = 89
    warningHighWaterMarkExceeded = False
    print "-"*tabWid
    for k in sorted( results.keys() ):
        res = results[k]
        print ( " %s %s %s %s %s" % ( repr(k).ljust(colWid), 
                                     repr(res[0]).center(colWid), 
                                     repr(res[1]).center(colWid), 
                                     repr(res[2]).center(colWid), 
                                     repr(res[3]).center(colWid) ) )
        totalErrors += res[0]
        totalSegFaults += res[2]
        totalInternal += res[3]
        if res[1] > warningHighWaterMark:
            warningHighWaterMarkExceeded = True

    print "-"*tabWid
    print " Note: Many yang files currently emit warnings."
    print "-"*tabWid
    errorOp = False
    if totalErrors>0:
        print "\033[31;1m Test Failed: There were %d errors \033[39;0m" % totalErrors
        errorOp = True
    if totalInternal>0:
        print "\033[31;1m Test Failed: There were %d Segment Faults \033[39;0m" % totalErrors
        errorOp = True
    if totalInternal>0:
        print "\033[31;1m Test Failed: There were %d internal errors \033[39;0m" % totalErrors
        errorOp = True
    if warningHighWaterMarkExceeded:
        print "\033[31;1m Test Failed: Warning High Water Mark of %d Exceeded, \033[39;0m" % warningHighWaterMark
        print "\033[31;1m New Warnings were introduced! \033[39;0m" 
        errorOp = True

    if errorOp == False:
        print "\033[39;1m Test Passed! \033[39;0m"

    print "-"*tabWid
    print "\n"

# ----------------------------------------------------------------------------|
if __name__ == '__main__':
    print "Testing Yangdump for various formats"

    InitialiseOutputDir()
    results = TestYang()
    DisplayResults( results )
