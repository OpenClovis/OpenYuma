#!/bin/sh
#
# make-svn-src-tarball.sh
#
# make the debug yuma source tarball 
#
if [ $# -gt 1 ]; then
  echo "Usage: make-svn-src-tarball [trunk]"
  echo "Creates a debug yuma source tarball"
  exit 1
elif [ $# -eq 0 ]; then
  echo ""
elif [ $1 -ne "trunk" ]; then
  echo "Usage: make-svn-src-tarball [trunk]"
  echo "Creates a debug yuma source tarball"
  exit 1
fi

SVNVER=`svnversion`

echo "Making debug yuma source tarball for SVN revision $SVNVER"
mkdir -p ~/svntarballprep
rm -rf ~/svntarballprep/*
cd ~/svntarballprep

if [ $# -eq 1 ]; then
echo " from trunk"
svn export https://yuma.svn.sourceforge.net/svnroot/yuma/trunk yuma$SVNVER
else
echo " from V1 branch"
svn export https://yuma.svn.sourceforge.net/svnroot/yuma/branches/v1 yuma$SVNVER
fi

echo "echo \"#define SVNVERSION \\\"$SVNVER\\\"\" > platform/curversion.h" > \
    yuma$SVNVER/netconf/src/platform/setversion.sh
tar cvf yuma$SVNVER.tar yuma$SVNVER
gzip yuma$SVNVER.tar



