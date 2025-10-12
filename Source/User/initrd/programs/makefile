################################################################################
# roOs Makefile
#
# Created: 23/05/2020
#
# Author: Alexy Torres Aurora Dugo
#
# Initrd module makefile. This makefile is used to compile the programs for the
# initrd module.
################################################################################

# Variables definitions
BIN_DIR = bin

PROG_DIRS := $(sort $(dir $(wildcard */)))
SUBDIRS     := $(shell for d in $(PROG_DIRS); \
    do ! [ "$$d" = "$(BIN_DIR)/" ] && echo "$$d"; done)
SUBDIRSCLEAN=$(addsuffix .clean,$(SUBDIRS))

.PHONY: all $(SUBDIRS)
all: prepare $(SUBDIRS)

prepare:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Compiling user initrd programs \e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@mkdir -p $(BIN_DIR)

$(SUBDIRS):
	@make -C $@
	@cp -r $@/$(BIN_DIR)/. $(BIN_DIR)

# Clean
clean: $(SUBDIRSCLEAN)
	@$(RM) -rf $(BIN_DIR)
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned user initrd programs\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

$(SUBDIRSCLEAN):
	$(MAKE) -C $(basename $@) clean