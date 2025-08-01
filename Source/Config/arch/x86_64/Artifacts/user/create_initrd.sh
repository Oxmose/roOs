#!/bin/bash

echo -e "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
echo -e "\e[1m\e[34m| Creating init ram disk for architecture x86_64\e[22m\e[39m"
echo -e "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

pwd
# Create tar file
echo "Creating TAR with files:"
cd $1
tar --sort=name -pcvf ../$2/initrd.tar `ls -A`
filesize=$(wc -c < "../$2/initrd.tar")

echo -e "\nInitrd size: $filesize"

cd ../$2
rm -f utk.initrd
cat initrd.tar >> utk.initrd
rm initrd.tar

echo "section .roos_ramdisk" >> ramdisk.s
echo "incbin \"utk.initrd\"" >> ramdisk.s

nasm -g -f elf64 -w+gnu-elf-extensions -F dwarf ramdisk.s -o ramdisk.o -I .
ar r libramdisk.a ramdisk.o
rm ramdisk.o ramdisk.s utk.initrd