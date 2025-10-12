################################################################################
# roOs Makefile
#
# Created: 23/05/2020
#
# Author: Alexy Torres Aurora Dugo
#
# Initrd module makefile. This makefile is used to compile the initrd module.
################################################################################

# Variables definitions
BASE_DIR       = basefiles
PROGRAM_DIR    = programs/bin
BUILD_DIR      = build
INITRD_OUTPUT  = initrdout
ARTIFACTS_DIR  = ../ARTIFACTS
INITRD_DIR     = $(ARTIFACTS_DIR)/initrd

.PHONY: all
all: init build_initrd

init:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generating user initrd module\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(INITRD_DIR)

build_programs:
	@make -C programs

build_initrd: build_programs
	@cp -r $(BASE_DIR)/. $(PROGRAM_DIR)/. $(BUILD_DIR)
	@chmod +x $(ARTIFACTS_DIR)/create_initrd.sh
	$(ARTIFACTS_DIR)/create_initrd.sh $(BUILD_DIR) $(INITRD_DIR)

	@echo "\e[1m\e[92m=> Generated user initrd module\e[22m\e[39m"
	@echo "\e[1m\e[92m--------------------------------------------------------------------------------\n\e[22m\e[39m"

# Clean
clean:
	@$(RM) -rf $(BUILD_DIR) $(INITRD_DIR)
	@make -C programs clean

	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned user initrd module\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"