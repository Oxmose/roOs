################################################################################
# UTK Makefile
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
	@$(MAKE) -C ./libc target_cpu=$(target_cpu)
	@$(MAKE) -C ./libapi target_cpu=$(target_cpu)
	@$(MAKE) -C ./libtrace target_cpu=$(target_cpu)

# Copy the generated libs
	@cp ./libc/$(BIN_DIR)/liblibc.a $(BIN_DIR)/
	@cp ./libapi/$(BIN_DIR)/libapi.a $(BIN_DIR)/
	@cp ./libtrace/$(BIN_DIR)/libtrace.a $(BIN_DIR)/

clean:

	@$(MAKE) -C ./libc target_cpu=$(target_cpu) clean
	@$(MAKE) -C ./libapi target_cpu=$(target_cpu) clean
	@$(MAKE) -C ./libtrace target_cpu=$(target_cpu) clean

	$(RM) -rf $(BIN_DIR)

	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned LIBS module\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"