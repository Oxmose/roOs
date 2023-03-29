#!/bin/bash

echo -e "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
echo -e "\e[1m\e[34m| Init build for target x86_i386\e[22m\e[39m"
echo -e "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

cd "$(dirname "$0")"

mkdir -p ../../../Kernel/ARTIFACTS

cp Artifacts/* ../../../Kernel/ARTIFACTS/
echo -e "\e[1m\e[92m\nUpdated ARTIFACTS\e[22m\e[39m"
