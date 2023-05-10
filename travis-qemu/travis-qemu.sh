#!/bin/bash
set -e

VERSION=${QEMU_VERSION:=8.0.0}
ARCHES=${QEMU_ARCHES:=i386 x86_64}
TARGETS=${QEMU_TARGETS:=$(echo $ARCHES | sed 's#$# #;s#\([^ ]*\) #\1-softmmu \1-linux-user #g')}

if echo "$VERSION $TARGETS" | cmp --silent $HOME/qemu/.build -; then
  echo "qemu $VERSION up to date!"
  exit 0
fi

echo "VERSION: $VERSION"
echo "TARGETS: $TARGETS"

cd $HOME
rm -rf qemu

# Checking for a tarball before downloading makes testing easier :-)
test -f "qemu-$VERSION.tar.bz2" || wget "https://download.qemu.org/qemu-$VERSION.tar.bz2"
tar -xf "qemu-$VERSION.tar.bz2"
cd "qemu-$VERSION"

./configure \
  --prefix="$HOME/qemu" \
  --target-list="$TARGETS"

make -j8
make install

echo "$VERSION $TARGETS" > $HOME/qemu/.build
