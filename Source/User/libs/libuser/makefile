################################################################################
# roOs Makefile
#
# Created: 29/03/2023
#
# Author: Alexy Torres Aurora Dugo
#
# User library module makefile. This makefile is used to compile the user
# library module for the desired target.
################################################################################

CPU_ARCH_LIST=i386,x86_64
BOARD_ARCH_LIST=x86

BIN_DIR    = ./bin

.PHONY: all
all: init build_modules

init:
	@mkdir -p $(BIN_DIR)

build_cpu:
################################################################################
# Build the CPU target user lib module
ifeq ($(target_cpu), i386)
	@$(MAKE) -C ./i386
else ifeq ($(target_cpu), x86_64)
	@$(MAKE) -C ./x86_64
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown CPU architecture $(target_cpu)\e[22m\e[39m"
	@echo "Available architectures: $(CPU_ARCH_LIST)\n"
	@exit 1
endif

build_modules: build_cpu
ifeq ($(target_cpu), i386)
	@cp ./i386/$(BIN_DIR)/* $(BIN_DIR)
else ifeq ($(target_cpu), x86_64)
	@cp ./x86_64/$(BIN_DIR)/* $(BIN_DIR)
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown CPU architecture $(target_cpu)\e[22m\e[39m"
	@echo "Available architectures: $(CPU_ARCH_LIST)\n"
	@exit 1
endif

	@echo "\e[1m\e[92m\n=> Generated user lib module\e[22m\e[39m"
	@echo "\e[1m\e[92m--------------------------------------------------------------------------------\n\e[22m\e[39m"

clean:
################################################################################
# Clean the CPU target user lib module
ifeq ($(target_cpu), i386)
	@$(MAKE) -C ./i386 clean
else ifeq ($(target_cpu), x86_64)
	@$(MAKE) -C ./x86_64 clean
else
	@$(MAKE) -C ./i386 clean
	@$(MAKE) -C ./x86_64 clean
endif

	$(RM) -rf $(BIN_DIR)

	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned user lib module\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"