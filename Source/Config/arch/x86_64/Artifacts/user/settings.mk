################################################################################
# roOs Makefile
#
# Created: 23/05/2020
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

USER_LINKER_FILE = user_linker.ld

DEBUG_FLAGS = -O0 -g
EXTRA_FLAGS = -O3
ARCH_FLAGS = -DARCH_64_BITS

CFLAGS = -std=c11 -nostdinc -fno-builtin -nostdlib  \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c -fno-pie \
		 -no-pie -MD -ffreestanding -Wno-address-of-packed-member \
		 -fno-omit-frame-pointer -Wmissing-prototypes \
		 -Wunused-result $(ARCH_FLAGS)



ifeq ($(DEBUG), TRUE)
CFLAGS += $(DEBUG_FLAGS)
else
CFLAGS += $(EXTRA_FLAGS)
endif

ASFLAGS = -g -f elf64 -w+gnu-elf-extensions -F dwarf
LDFLAGS = -no-pie