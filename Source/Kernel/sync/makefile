################################################################################
# roOs Makefile
#
# Created: 20/06/2024
#
# Author: Alexy Torres Aurora Dugo
#
# Syncronization module makefile. This makefile is used to compile the
# synchronization module.
################################################################################

# Dependencies
include dependencies.mk
include ../ARTIFACTS/settings.mk

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# Variables definitions
SRC_DIR       = src
BUILD_DIR     = build
INC_DIR       = includes
BIN_DIR       = bin
ARTIFACTS_DIR = ../ARTIFACTS

C_SRCS = $(call rwildcard,$(SRC_DIR),*.c)
A_SRCS = $(call rwildcard,$(SRC_DIR),*.s)
C_OBJS = $(C_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
A_OBJS = $(A_SRCS:$(SRC_DIR)/%.s=$(BUILD_DIR)/%.o)

.PHONY: all
all: init module

init:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Compiling Sync module\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

module: compile_asm compile_cc
	@$(AR) r $(BIN_DIR)/libsync.a $(BUILD_DIR)/*.o
	@echo "\e[1m\e[92m=> Generated Sync module\e[22m\e[39m"
	@echo "\e[1m\e[92m--------------------------------------------------------------------------------\n\e[22m\e[39m"

# Assembly sources compilation
compile_asm: $(A_OBJS)
	@echo "\e[1m\e[94m=> Compiled ASM sources\e[22m\e[39m"
	@echo

# C sources compilation
compile_cc: $(C_OBJS)
	@echo "\e[1m\e[94m=> Compiled C sources\e[22m\e[39m"
	@echo

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
ifeq ($(DEBUG), TRUE)
	@echo -n "  DEBUG "
endif
	@echo  "\e[32m  $< \e[22m\e[39m=> \e[1m\e[94m$@\e[22m\e[39m"
	$(CC) $(CFLAGS) $< -o $@ -I $(INC_DIR) $(DEP_INCLUDES) -I $(ARTIFACTS_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@echo  "\e[32m  $< \e[22m\e[39m=> \e[1m\e[94m$@\e[22m\e[39m"
	$(AS) $(ASFLAGS) $< -o $@ -I $(INC_DIR) $(DEP_INCLUDES) -I $(ARTIFACTS_DIR)

# Clean
clean:
	$(RM) -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned Sync module\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

# Check header files modifications
-include $(C_OBJS:.o=.d)