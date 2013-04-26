#!/bin/sh
#
# prep-deb.sh
#
# make the yuma build code and tar it up for debian
# Only supports packaging of v2 branch!!!

VER="2.2"

if [ ! -d $HOME/swdev/yuma-$VER ]; then
  echo "Error: $HOME/swdev/yuma-$VER not found"
  echo "  1: git clone https://github.com/YumaWorks/yuma yuma-2.2"
  echo "  2: cd yuma-2.2; git checkout v2; git pull"
  exit 1
fi

mkdir -p $HOME/build

cd $HOME/swdev
tar --exclude=.git* --exclude=.svn* -cvf yuma_$VER.tar  yuma-$VER/
gzip yuma_$VER.tar
mv yuma_$VER.tar.gz $HOME/build

cd $HOME/build
if [ ! -f yuma_$VER.orig.tar.gz ]; then
  cp yuma_$VER.tar.gz yuma_$VER.orig.tar.gz
fi

rm -rf yuma-$VER
tar xvf yuma_$VER.tar.gz





