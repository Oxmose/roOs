################################################################################
# roOs Makefile
#
# Created: 23/05/2020
#
# Author: Alexy Torres Aurora Dugo
#
# Libraries module makefile. This makefile is used to compile the library module
# for the desired target.
################################################################################

BIN_DIR    = ./bin

.PHONY: all
all: init build_modules

init:
	@mkdir -p $(BIN_DIR)

build_modules:
	@$(MAKE) -C ./libc
	@$(MAKE) -C ./libuser

# Copy the generated libs
	@cp ./libc/$(BIN_DIR)/* $(BIN_DIR)/
	@cp ./libuser/$(BIN_DIR)/* $(BIN_DIR)/

clean:
	@$(MAKE) -C ./libc clean
	@$(MAKE) -C ./libuser clean

	$(RM) -rf $(BIN_DIR)

	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned LIBS module\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"