#!/bin/bash
set -e

# Install dependencies
apt install g++ \
            nasm \
            xorriso \
            grub-common \
            mtools \
            libmount-dev \
            libselinux-dev \
            build-essential \
            libtool \
            pkg-config \
            intltool \
            libglib2.0-dev \
            libfdt-dev \
            libpixman-1-dev \
            zlib1g-dev \
            libaio-dev \
            libbluetooth-dev \
            libbrlapi-dev \
            libbz2-dev \
            libcap-dev \
            libcap-ng-dev \
            libcurl4-gnutls-dev \
            libgtk-3-dev \
            libibverbs-dev \
            libjpeg8-dev \
            libncurses5-dev \
            libnuma-dev \
            librbd-dev \
            librdmacm-dev \
            libsasl2-dev \
            libsdl1.2-dev \
            libseccomp-dev \
            libsnappy-dev \
            libssh2-1-dev \
            libvde-dev \
            libvdeplug-dev \
            libxen-dev \
            liblzo2-dev \
            valgrind \
            xfslibs-dev \
            libpulse-dev \
            libpulse0 \
            grub-pc-bin \
            ninja-build
# Setup build
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

# Downloading Qemu
test -f "qemu-$VERSION.tar.bz2" || wget "https://download.qemu.org/qemu-$VERSION.tar.bz2"
tar -xf "qemu-$VERSION.tar.bz2"
cd "qemu-$VERSION"

# Building Qemu
./configure \
  --prefix="$HOME/qemu" \
  --target-list="$TARGETS"

make -j8
make install

echo "$VERSION $TARGETS" > $HOME/qemu/.build
