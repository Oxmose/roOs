#!/bin/bash

echo -e "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
echo -e "\e[1m\e[34m| Creating init ram disk for architecture x86_64\e[22m\e[39m"
echo -e "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

# Create tar file
echo "Creating TAR with files:"
cd initrd
tar --sort=name -cvf ../initrd.tar *
cd ..

filesize=$(wc -c < "initrd.tar")

echo -e "\nInitrd size: $filesize"

rm -f utk.initrd
cat initrd.tar >> utk.initrd
rm initrd.tar