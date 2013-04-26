#!/bin/sh
#
#
# This is obsolete!!! Do not use!!!
#
# make-tarball.sh [revision]
#
# make the yuma binary tar ball 
#
# the $1 parameter must be a revision number if present
# this will be used instead of HEAD in the svn export step

if [ -z $YUMA_ARCH ]; then
  echo "Error: Missing YUMA_ARCH environment variable.";
  exit 1
fi

if [ -z $YUMA_OS ]; then
  echo "Error: Missing YUMA_OS environment variable.";
  exit 1
fi

ARCH=$YUMA_ARCH
OS=$YUMA_OS

if [ $# != 2 ]; then
  echo "Usage: make-tarball <version-num> <release-num>"
  echo "make-tarball 1.14 3"
  exit 1
fi

VER=$1
REL=$2

mkdir -p ~/tarballprep
rm -rf ~/tarballprep/*

cd ~/tarballprep

mkdir -p yuma-$VER-$REL
cd yuma-$VER-$REL
mkdir bin
mkdir sbin
mkdir lib
mkdir doc
mkdir man
mkdir etc
mkdir util
mkdir modules
cd ..

echo "Making yuma tarball for yuma-$VER-$REL"

git clone https://github.com/YumaWorks/yuma
cd yuma
git checkout v2

TAGSTR=$VER-"$REL"."$OS"."$ARCH"

if [ "$OS" = "fc12"  ]; then
  EXTRA=""
elif [ "$OS" = "fc13" ]; then
  EXTRA=""
elif [ "$OS" = "fc14" ]; then
  EXTRA=""
elif [ "$OS" = "fc15" ]; then
  EXTRA=""
elif [ "$OS" = "fc16" ]; then
  EXTRA=""
elif [ "$OS" = "fc17" ]; then
  EXTRA=""
else
  EXTRA="DEBIAN=1"
fi

echo "extra=$EXTRA"

cd trunk
make STATIC=1 $EXTRA RELEASE=$REL
cd ..

cp -p trunk/netconf/target/bin/yangdump yuma-$VER-$REL/bin
cp -p trunk/netconf/target/bin/yangdiff yuma-$VER-$REL/bin
cp -p trunk/netconf/target/bin/yangcli yuma-$VER-$REL/bin
cp -p trunk/netconf/util/make_sil_dir yuma-$VER-$REL/bin
cp -p trunk/netconf/target/bin/netconfd yuma-$VER-$REL/sbin
cp -p trunk/netconf/target/bin/netconf-subsystem yuma-$VER-$REL/sbin
cp -p trunk/libtoaster/lib/libtoaster.so yuma-$VER-$REL/lib

cp -p trunk/netconf/doc/extra/AUTHORS yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/extra/README yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/extra/INSTALL.tarball yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/extra/INSTALL.tarball yuma-$VER-$REL/
cp -p trunk/netconf/doc/extra/yumatools-legal-notices.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-installation-guide.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-netconfd-manual.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-quickstart-guide.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-user-cmn-manual.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-yangcli-manual.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-yangdiff-manual.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-yangdump-manual.pdf yuma-$VER-$REL/doc
cp -p trunk/netconf/doc/yuma_docs/yuma-dev-manual.pdf yuma-$VER-$REL/doc

cp -p trunk/netconf/util/make_sil_dir yuma-$VER-$REL/util
cp -p trunk/netconf/util/makefile.sil yuma-$VER-$REL/util
cp -p trunk/netconf/util/makefile-top.sil yuma-$VER-$REL/util

cp -p trunk/netconf/man/netconfd.1 yuma-$VER-$REL/man
cp -p trunk/netconf/man/netconf-subsystem.1 yuma-$VER-$REL/man
cp -p trunk/netconf/man/yangcli.1 yuma-$VER-$REL/man
cp -p trunk/netconf/man/yangdiff.1 yuma-$VER-$REL/man
cp -p trunk/netconf/man/yangdump.1 yuma-$VER-$REL/man
cp -p trunk/netconf/man/make_sil_dir.1 yuma-$VER-$REL/man

cp -p trunk/netconf/etc/netconfd-sample.conf yuma-$VER-$REL/etc
cp -p trunk/netconf/etc/yangcli-sample.conf yuma-$VER-$REL/etc
cp -p trunk/netconf/etc/yangdiff-sample.conf yuma-$VER-$REL/etc
cp -p trunk/netconf/etc/yangdump-sample.conf yuma-$VER-$REL/etc

cp -p -R trunk/netconf/modules/ietf yuma-$VER-$REL/modules
cp -p -R trunk/netconf/modules/netconfcentral yuma-$VER-$REL/modules
cp -p -R trunk/netconf/modules/test yuma-$VER-$REL/modules
cp -p -R trunk/netconf/modules/yang yuma-$VER-$REL/modules

tar cvf yuma-$TAGSTR.tar yuma-$VER-$REL
gzip yuma-$TAGSTR.tar


