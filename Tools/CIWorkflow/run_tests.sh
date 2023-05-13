#!/bin/bash

cd Source

python ../Tools/CIWorkflow/TestValidator.py x86_64 Kernel/TestFramework/includes/test_groups.json Kernel/TestFramework/includes/test_list.h ../test_file_output.txt
python ../Tools/CIWorkflow/TestValidator.py x86_i386 Kernel/TestFramework/includes/test_groups.json Kernel/TestFramework/includes/test_list.h ../test_file_output.txt
