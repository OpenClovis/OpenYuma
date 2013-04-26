#!/bin/sh
#
# make-src-tarball.sh <major-version> <minor-version> <release-number>
#
# make the yuma source tarball
#
# the $1 parameter must be a major version number, such as 1
# the $2 parameter must be a minor version number, such as 15
# the $3 parameter must be the release number, such as 1

if [ $# != 3 ]; then
  echo "Usage: make-src-tarball <major-version> <minor-version> <release-number>"
  echo "Example:   make-src-tarball 1 15 1 for v1.15-1"
  exit 1
fi

if [ $1 = 1 ]; then
  echo "Making yuma source tarball for $1.$2-$3 from v1 branch"
  mkdir -p ~/srctarballprep
  cd ~/srctarballprep
  rm -rf yuma-$1.$2-$3 yuma-$1.$2-$3.tar.gz
  git clone https://github.com/YumaWorks/yuma yuma-$1.$2-$3  
  cd yuma-$1.$2-$3
  git checkout v1
elif [ $1 = 2 ]; then
  echo "Making yuma source tarball for $1.$2-$3 from v2 branch"
  mkdir -p ~/srctarballprep
  cd ~/srctarballprep
  rm -rf yuma-$1.$2-$3 yuma-$1.$2-$3.tar.gz
  git clone https://github.com/YumaWorks/yuma yuma-$1.$2-$3  
  cd yuma-$1.$2-$3
  git checkout v2
elif [ $1 = 3 ]; then
  echo "Making yuma source tarball for $1.$2-$3 from trunk"
  mkdir -p ~/srctarballprep
  cd ~/srctarballprep
  rm -rf yuma-$1.$2-$3 yuma-$1.$2-$3.tar.gz
  git clone https://github.com/YumaWorks/yuma yuma-$1.$2-$3  
  cd yuma-$1.$2-$3
else
  echo "Error: major version must be 1 or 2"
  echo "Usage: make-src-tarball <major-version> <minor-version> <release-number>"
  echo "Example:   make-src-tarball 1 15 1 for v1.15-1"
  exit 1
fi

echo "echo \"#define RELEASE $3\" > platform/curversion.h" > \
    netconf/src/platform/setversion.sh
cd ..
rm -f yuma-$1.$2-$3.tar yuma-$1.$2-$3.zip
tar --exclude=.git* --exclude=.svn* -cvf yuma-$1.$2-$3.tar yuma-$1.$2-$3
gzip yuma-$1.$2-$3.tar
zip -r yuma-$1.$2-$3.zip yuma-$1.$2-$3 -x "yuma-$1.$2-$3/.*"









