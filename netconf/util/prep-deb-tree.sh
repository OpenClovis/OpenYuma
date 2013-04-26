#!/bin/sh
#
# prep-deb-tree.sh [major version number]
#
# make the yuma build code and tar it up for debian
# use the current tree, not the latest SVN sources
#
# the $1 parameter must be '1' or '2'

if [ $# != 1 ]; then
  echo "Usage: prep-deb-tree.sh <major-version>"
  echo "Example:   prep-deb-tree.sh 1"
  exit 1
fi

if [ $1 = 1 ]; then
VER="1.15"
DIR=branches/v1
elif [ $1 = 2 ]; then
VER="2.2"
DIR=branches/v2
elif [ $1 = 3 ]; then
VER="3.0"
DIR=trunk
else
  echo "Error: major version must be 1 to 3"
  echo "Usage: prep-deb-tree.sh <major-version>"
  echo "Example:   prep-deb-tree.sh 2"
  exit 1
fi

mkdir -p ~/build
mkdir -p ~/rpmprep
rm -rf ~/rpmprep/*
mkdir -p ~/rpmprep/yuma-$VER

cd $DIR
tar cvf ~/rpmprep/yuma-$VER/yprep.tar --exclude=.svn *

cd ~/rpmprep/yuma-$VER
tar xvf yprep.tar
rm -f yprep.tar
cd ..
tar cvf yuma_$VER.tar yuma-$VER
gzip yuma_$VER.tar
cp yuma_$VER.tar.gz ~/build

cd ~/build
if [ ! -f yuma_$VER.orig.tar.gz ]; then
  cp yuma_$VER.tar.gz yuma_$VER.orig.tar.gz
fi
rm -rf yuma_$VER
tar xvf yuma_$VER.tar.gz






