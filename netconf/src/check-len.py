#!/usr/bin/env python
import sys
import os
import commands

# ----------------------------------------------------------------------------|
class NullFilter:
    def apply( self, filename ):
        """Apply a 'null' filter to the filename supplied."""
        return True

# ----------------------------------------------------------------------------|
class EndswithFilenameFilter:
    def __init__( self, filters = [ '.c', '.h', '.cpp', '.inl' ] ):
        self.filters_ = filters

    def apply( self, filename ):
        """Apply an 'endswith' filter to the filename supplied."""
        for f in self.filters_:
            if filename.endswith( f ):
                return True
        return False

# ----------------------------------------------------------------------------|
class StartswithFilenameFilter:
    def __init__( self, filters = [ 'M ', 'A ' ] ):
        self.filters_ = filters

    def apply( self, filename ):
        """Apply an 'startswith' filter to the filename supplied."""
        for f in self.filters_:
            if filename.startswith( f ):
                return True
        return False

# ----------------------------------------------------------------------------|
class SVNModifiedFilenameFilter:
    def __init__( self ):
        self.startFilter = StartswithFilenameFilter()
        self.endFilter = EndswithFilenameFilter()

    def apply( self, filename ):
        """Apply the filter"""
        if self.startFilter.apply( filename ) and self.endFilter.apply( filename ):
            return True
        return False

# ----------------------------------------------------------------------------|
def DisplayUsage():
    print ( """Usage: check-len.py [-a]
               Check the line length of modified netconf source files.

               [-a] : Check all source files.""" )
    sys.exit(-1)

# ----------------------------------------------------------------------------|
def GetAllFilenames( rootDir = "./", filenameFilter = NullFilter() ):
    """Get the list of all filenames matching the supplied filter"""
    filenames = []
    for root, dirs, files in os.walk( rootDir ):
        filtered = [ n for n in files 
                     if filenameFilter.apply( n ) ]
        filenames += [ root + "/" + n for n in filtered ]
    return filenames

# ----------------------------------------------------------------------------|
def GetModifiedFiles():
    svnOp = commands.getoutput( "svn status" )
    svnOp = svnOp.split( '\n' )
    filenameFilter = SVNModifiedFilenameFilter()
    filenames = []
    for entry in svnOp:
        if filenameFilter.apply( entry ):
            filenames.append( entry.lstrip( "MA ") )
    return filenames

# ----------------------------------------------------------------------------|
def CheckFile( filename ):
    """Check the length of every line in the file"""
    print "Checking %s...." %filename
    f = open( filename, 'r' )
    lines = f.readlines()
    lineNo = 1
    for line in lines:
        # Note allow 1 character for CR
        if len( line ) > 81:
            print ( "\tLine %d exceeds 80 characters:  %d" % 
                    ( lineNo, len( line ) ) )
        lineNo += 1

# ----------------------------------------------------------------------------|
def CheckFiles( filenames ):
    """Check each of the files"""
    for filename in filenames:
        CheckFile( filename )
        

# ----------------------------------------------------------------------------|
if __name__ == '__main__':
    if len ( sys.argv ) >1 :
        if sys.argv[1] == "-a":
            filenames = GetAllFilenames( filenameFilter = EndswithFilenameFilter() )
        else:
            DisplayUsage()
    else:
        filenames = GetModifiedFiles()

    CheckFiles( filenames )


