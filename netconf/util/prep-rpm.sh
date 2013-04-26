#!/bin/sh
#
# prep-rpm.sh [major version number]
#
# make the yuma build code and tar it up for rpmbuild
#
# 1 package:
#   yuma-version.rpm
#
# the $1 parameter must be '1', '2', or '3'

if [ $# != 1 ]; then
  echo "Usage: prep-rpm.sh <major-version>"
  echo "Example:   prep-rpm.sh 2"
  exit 1
fi

if [ $1 = 1 ]; then
VER="1.15"
BRANCH="v1"
SPEC="yuma.spec"
elif [ $1 = 2 ]; then
VER="2.2"
BRANCH="v2"
SPEC="yuma2.spec"
ALLSPEC="yuma2all.spec"
elif [ $1 = 3 ]; then
BRANCH="master"
SPEC="yuma2.spec"
ALLSPEC="yuma2all.spec"
else
  echo "Error: major version must be 1 or 2"
  echo "Usage: prep-rpm.sh <major-version>"
  echo "Example:   prep-rpm.sh 2"
  exit 1
fi

if [ ! -d $HOME/swdev/yuma-$VER ]; then
  echo "Error: $HOME/swdev/yuma-$VER not found"
  echo "  1: git clone https://github.com/YumaWorks/yuma yuma-2.2"
  echo "  2: cd yuma-2.2; git checkout v2; git pull"
  exit 1
fi

mkdir -p ~/rpmprep
rm -rf ~/rpmprep/*

cd ~/rpmprep

cd $HOME/swdev
touch yuma-$VER/configure
chmod 775 yuma-$VER/configure
tar --exclude=.git* --exclude=.svn* -cvf yuma-$VER.tar  yuma-$VER/
gzip yuma-$VER.tar
rm yuma-$VER/configure

mkdir -p ~/rpmbuild/SPECS
mkdir -p ~/rpmbuild/SOURCES
mkdir -p ~/rpmbuild/SRPMS
mkdir -p ~/rpmbuild/RPMS

cp yuma-$VER.tar.gz ~/rpmbuild/SOURCES
cp yuma-$VER/SPECS/$SPEC ~/rpmbuild/SPECS

if [ $1 != 1 ]; then
  cp yuma-$VER/SPECS/$ALLSPEC ~/rpmbuild/SPECS
fi

