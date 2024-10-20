################################################################################
# roOs Makefile
#
# Created: 29/03/2023
#
# Author: Alexy Torres Aurora Dugo
#
# Settings makefile, contains the shared variables used by the different
# makefiles.
################################################################################

ifeq ($(shell uname -s),Darwin)
	CC = x86_64-elf-gcc
	AS = nasm
	LD = x86_64-elf-ld
	OBJCOPY = x86_64-elf-objcopy
	AR = x86_64-elf-ar
else
	CC = gcc
	AS = nasm
	LD = ld
	OBJCOPY = objcopy
	AR = ar
endif

LINKER_FILE = ../../Config/arch/x86_i386/linker.ld

DEBUG_FLAGS = -O0 -g
EXTRA_FLAGS = -O3 -g

CFLAGS = -m32 -std=c11 -nostdinc -fno-builtin -nostdlib \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c -fno-pie \
		 -no-pie -MD -ffreestanding -Wno-address-of-packed-member \
		 -fno-omit-frame-pointer -Wmissing-prototypes \
		 -Wunused-result

TESTS_FLAGS = -D_TESTING_FRAMEWORK_ENABLED

ifeq ($(TESTS), TRUE)
CFLAGS += $(TESTS_FLAGS)
endif

ifeq ($(STK_PROT), TRUE)
CFLAGS += -D_STACK_PROT -fstack-protector-all -mstack-protector-guard=global
else
CFLAGS += -fno-stack-protector
endif

ifeq ($(DEBUG), TRUE)
CFLAGS += $(DEBUG_FLAGS)
else
CFLAGS += $(EXTRA_FLAGS)
endif

ifeq ($(TRACE), TRUE)
CFLAGS += -D_TRACING_ENABLED
endif

ASFLAGS = -g -f elf -w+gnu-elf-extensions -F dwarf
LDFLAGS = -T $(LINKER_FILE) -no-pie -melf_i386