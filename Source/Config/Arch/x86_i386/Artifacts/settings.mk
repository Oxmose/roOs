################################################################################
# UTK Makefile
#
# Created: 29/03/2023
#
# Author: Alexy Torres Aurora Dugo
#
# Settings makefile, contains the shared variables used by the different
# makefiles.
################################################################################

CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

LINKER_FILE = ../../Config/Arch/x86_i386/linker.ld

DEBUG_FLAGS = -O0 -g
EXTRA_FLAGS = -O3 -g

CFLAGS = -m32 -std=c11 -nostdinc -fno-builtin -nostdlib -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c -fno-pie \
		 -no-pie -MD -ffreestanding -Wno-address-of-packed-member \
		 -fno-omit-frame-pointer -Wmissing-prototypes

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

ASFLAGS = -g -f elf -w+gnu-elf-extensions -F dwarf
LDFLAGS = -T $(LINKER_FILE) -melf_i386 -no-pie