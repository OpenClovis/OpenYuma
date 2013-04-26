#!/bin/sh
#
# Rename the yuma package files
#
# P1 == the Yuma version number to use

if [ $# != 1 ]; then
  echo "Usage: rename_yumadebs.sh yuma-version"
  echo "   rename_yumadebs.sh 1.12-2"
  exit 1
fi

if [ -z $YUMA_ARCH ]; then
  echo "Error: Missing YUMA_ARCH environment variable.";
  exit 1
fi

if [ -z $YUMA_OS ]; then
  echo "Error: Missing YUMA_OS environment variable.";
  exit 1
fi

if [ ! -d ~/build ]; then
  echo "Error: ~/build directory not found"
  exit 1
fi

UV="$YUMA_OS"
ARCH="$YUMA_ARCH"

mkdir -p ~/YUMA_PACKAGES

if [ ! -d ~/YUMA_PACKAGES ]; then
  echo "Error: ~/YUMA_PACKAGES directory could not be created"
  exit 1
fi

cd ~/build

if [ ! -f ./yuma_$1_$ARCH.deb ]; then
  echo "Error: yuma_$1_$ARCH.deb not found"
  exit 1
fi

if [ ! -f ./yuma-dev_$1_$ARCH.deb ]; then
  echo "Error: yuma-dev_$1_$ARCH.deb not found"
  exit 1
fi

cp -f ./yuma_$1_$ARCH.deb ~/YUMA_PACKAGES/yuma-$1."$UV".$ARCH.deb
cp -f ./yuma-dev_$1_$ARCH.deb ~/YUMA_PACKAGES/yuma-dev-$1."$UV".$ARCH.deb
cp -f ./yuma-doc_$1_$ARCH.deb ~/YUMA_PACKAGES/yuma-dev-$1."$UV".$ARCH.deb



  
    
    



