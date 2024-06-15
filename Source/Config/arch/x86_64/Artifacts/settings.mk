################################################################################
# UTK Makefile
#
# Created: 23/05/2020
#
# Author: Alexy Torres Aurora Dugo
#
# Settings makefile, contains the shared variables used by the different
# makefiles.
################################################################################

ifeq ($(UNAME_S),Darwin)
	CC = x86_64-elf-gcc
	AS = nasm
	LD = x86_64-elf-ld
	OBJCOPY = x86_64-elf-objcopy
else
	CC = gcc
	AS = nasm
	LD = ld
	OBJCOPY = objcopy
endif

LINKER_FILE = ../../Config/arch/x86_64/linker.ld

DEBUG_FLAGS = -O0 -g
EXTRA_FLAGS = -O3 -g

CFLAGS = -std=c11 -nostdinc -fno-builtin -nostdlib -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c -fno-pie \
		 -no-pie -MD -ffreestanding -Wno-address-of-packed-member \
		 -fno-omit-frame-pointer -Wmissing-prototypes -mcmodel=kernel \
		 -Wunused-result

TESTS_FLAGS = -D_TESTING_FRAMEWORK_ENABLED

ifeq ($(TESTS), TRUE)
CFLAGS += $(TESTS_FLAGS)
endif

ifeq ($(DEBUG), TRUE)
CFLAGS += $(DEBUG_FLAGS)
else
CFLAGS += $(EXTRA_FLAGS)
endif

ifeq ($(TRACE), TRUE)
CFLAGS += -D_TRACING_ENABLED
endif

ASFLAGS = -g -f elf64 -w+gnu-elf-extensions -F dwarf
LDFLAGS = -T $(LINKER_FILE) -no-pie