#!/bin/bash

pwd
cd ../../Source

make clean
make target=x86_i386 DEBUG=TRUE TESTS=TRUE
make target=x86_i386 run &
sleep 10
killall qemu*