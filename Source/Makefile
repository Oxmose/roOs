################################################################################
# UTK Makefile
#
# Created: 29/03/2023
#
# Author: Alexy Torres Aurora Dugo
#
# Main makefile. This makefile will proceed to compile all required modules
# and configurations.
# Once compiled it will produce the final binary file for the desired
# architecture
################################################################################

CPU_ARCH_LIST= i386,x86_64
TARGET_LIST= x86_i386,x86_64
SOURCE_DIR= ./Kernel
CONFIG_DIR= ./Config

BUILD_DIR = build

KERNEL = UTK

.PHONY: all
all: init build_modules build_kernel

init:
	@mkdir -p $(BUILD_DIR)

ifeq ($(target), x86_i386)
	$(CONFIG_DIR)/Arch/x86_i386/init_build.sh
else ifeq ($(target), x86_64)
	$(CONFIG_DIR)/Arch/x86_64/init_build.sh
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown target $(target)\e[22m\e[39m"
	@echo "Available targets: $(TARGET_LIST)\n"
	@exit 1
endif

build_modules:
ifeq ($(target), x86_i386)
# Build the architecture module
	@make -C $(SOURCE_DIR)/Arch target_board=x86 target_cpu=i386
	@make -C $(SOURCE_DIR)/Core target_board=x86 target_cpu=i386
	@make -C $(SOURCE_DIR)/IO target_board=x86 target_cpu=i386
	@make -C $(SOURCE_DIR)/Time target_board=x86 target_cpu=i386
	@make -C $(SOURCE_DIR)/Libs target_board=x86 target_cpu=i386
else ifeq ($(target), x86_64)
# Build the architecture module
	@make -C $(SOURCE_DIR)/Arch target_board=x86 target_cpu=x86_64
	@make -C $(SOURCE_DIR)/Core target_board=x86 target_cpu=x86_64
	@make -C $(SOURCE_DIR)/IO target_board=x86 target_cpu=x86_64
	@make -C $(SOURCE_DIR)/Time target_board=x86 target_cpu=x86_64
	@make -C $(SOURCE_DIR)/Libs target_board=x86 target_cpu=x86_64
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown target $(target)\e[22m\e[39m"
	@echo "Available targets: $(TARGET_LIST)\n"
	@exit 1
endif

ifeq ($(TESTS), TRUE)
	# Build the tests module
	@make -C $(SOURCE_DIR)/TestFramework target=$(target)
endif

# Build general modules



build_kernel:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Building kernel for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@make -C $(SOURCE_DIR)/ARTIFACTS KERNEL_NAME=$(KERNEL)

	@cp -r $(SOURCE_DIR)/ARTIFACTS/build/* $(BUILD_DIR)

	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generated kernel for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

# TODO:
#	@chmod +x $(CONFIG_DIR)/Arch/x86_i386/create_initrd.sh
#	$(CONFIG_DIR)/Arch/x86_i386/create_initrd.sh $(BUILD_DIR)

clean:
# Dummy settings
	@mkdir -p Kernel/ARTIFACTS
	@touch Kernel/ARTIFACTS/settings.mk

# Clean the architecture module
ifeq ($(target), x86_i386)
	@make -C $(SOURCE_DIR)/Arch target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/Core target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/IO target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/Time target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/Libs target_board=x86 target_cpu=i386 clean
else ifeq ($(target), x86_64)
	@make -C $(SOURCE_DIR)/Arch target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/Core target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/IO target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/Time target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/Libs target_board=x86 target_cpu=x86_64 clean
else
	@make -C $(SOURCE_DIR)/Arch target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/Arch target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/Core target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/Core target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/IO target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/IO target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/Time target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/Time target_board=x86 target_cpu=x86_64 clean
	@make -C $(SOURCE_DIR)/Libs target_board=x86 target_cpu=i386 clean
	@make -C $(SOURCE_DIR)/Libs target_board=x86 target_cpu=x86_64 clean
endif

# Clean modules
	@make clean -C $(SOURCE_DIR)/TestFramework

# Clean kernel build directory
	rm -rf  $(BUILD_DIR) $(BIN_DIR)


	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned kernel \e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@rm -rf Kernel/ARTIFACTS

run:
	@make run -f $(CONFIG_DIR)/Arch/$(target)/makerun.mk BUILD_DIR=$(BUILD_DIR) KERNEL=$(KERNEL)

debug:
	@make debug -f $(CONFIG_DIR)/Arch/$(target)/makerun.mk BUILD_DIR=$(BUILD_DIR) KERNEL=$(KERNEL)

qemu-test-mode:
	@make qemu-test-mode -f $(CONFIG_DIR)/Arch/$(target)/makerun.mk BUILD_DIR=$(BUILD_DIR) KERNEL=$(KERNEL)