#!/bin/bash

echo -e "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
echo -e "\e[1m\e[34m| Init build for target x86_64\e[22m\e[39m"
echo -e "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

cd "$(dirname "$0")"

mkdir -p ../../../Kernel/ARTIFACTS


# Generate DTB
echo -e "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
echo -e "\e[1m\e[34m| Generating device tree for x86_i386\e[22m\e[39m"
echo -e "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"


dtc -O dtb -o x86_64_fdt.dtb x86_64_fdt.dts

echo "section .dtb_raw" >> dtb_obj.s
echo "incbin \"x86_64_fdt.dtb\"" >> dtb_obj.s

nasm -g -f elf64 -w+gnu-elf-extensions -F dwarf dtb_obj.s -o dtb_obj.o -I .
ar r librawdtb.a dtb_obj.o
mv librawdtb.a ../../../Kernel/ARTIFACTS/

cp Artifacts/* ../../../Kernel/ARTIFACTS/
rm x86_64_fdt.dtb dtb_obj.o dtb_obj.s

echo -e "\e[1m\e[92m\nUpdated ARTIFACTS\e[22m\e[39m"
