#!/bin/bash

pwd
cd Source

make clean
make target=x86_i386 DEBUG=TRUE TESTS=TRUE
make target=x86_i386 qemu-test-mode &
sleep 10
killall qemu*