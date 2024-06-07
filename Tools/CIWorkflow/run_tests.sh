#!/bin/bash

mkdir GeneratedFiles
rm -rf GeneratedFiles/*
cd Source

python3 ../Tools/CIWorkflow/TestValidator.py x86_64 Kernel/test_framework/includes/test_groups_x64.json Kernel/test_framework/includes/test_list.h ../GeneratedFiles/test_file_output.txt

if (( $? != 0 ))
then
    exit -1
fi

python3 ../Tools/CIWorkflow/TestValidator.py x86_i386 Kernel/test_framework/includes/test_groups_i386.json Kernel/test_framework/includes/test_list.h ../GeneratedFiles/test_file_output.txt

if (( $? != 0 ))
then
    exit -1
fi
