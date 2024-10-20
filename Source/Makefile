################################################################################
# roOs Makefile
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
KERNEL_SRC_DIR= ./Kernel
USER_SRC_DIR = ./User
CONFIG_DIR= ./Config

KERNEL_ARTIFACTS_DIR = Kernel/ARTIFACTS
KERNEL_BUILD_DIR = build/kernel
USER_BUILD_DIR = build/user

KERNEL = roOs

.PHONY: all
all: init build_kernel

init:
ifeq ($(target), x86_i386)
	$(CONFIG_DIR)/arch/x86_i386/init_build.sh
else ifeq ($(target), x86_64)
	$(CONFIG_DIR)/arch/x86_64/init_build.sh
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown target $(target)\e[22m\e[39m"
	@echo "Available targets: $(TARGET_LIST)\n"
	@exit 1
endif

build_modules:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Building kernel modules for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

ifeq ($(target), x86_i386)
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=i386
else ifeq ($(target), x86_64)
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=x86_64
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown target $(target)\e[22m\e[39m"
	@echo "Available targets: $(TARGET_LIST)\n"
	@exit 1
endif

	@make -C $(KERNEL_SRC_DIR)/core
	@make -C $(KERNEL_SRC_DIR)/io
	@make -C $(KERNEL_SRC_DIR)/time
	@make -C $(KERNEL_SRC_DIR)/libs
	@make -C $(KERNEL_SRC_DIR)/user
	@make -C $(KERNEL_SRC_DIR)/sync
	@make -C $(KERNEL_SRC_DIR)/fs

ifeq ($(TESTS), TRUE)
	# Build the tests module
	@make -C $(KERNEL_SRC_DIR)/test_framework target=$(target)
endif

	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generated kernel modules for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

####################################
# Build Kernel and integrated initrd
####################################

build_user_modules:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Building user modules for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@make -C $(USER_SRC_DIR)/libs target_cpu=x86_64

	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generated user modules for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

build_initrd: build_user_modules
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Building kernel integrated initrd for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@make -C $(USER_SRC_DIR)/initrd target_cpu=x86_64

	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generated kernel integrated initrd for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

build_kernel: build_initrd build_modules
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Building kernel for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@cp -r $(USER_SRC_DIR)/ARTIFACTS/initrd/* $(KERNEL_SRC_DIR)/ARTIFACTS
	@make -C $(KERNEL_SRC_DIR)/ARTIFACTS KERNEL_NAME=$(KERNEL) BUILD_DIR=$(KERNEL_BUILD_DIR)

	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generated kernel for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

####################################
# Build rootfs and user apps
####################################

clean:

# Dummy settings
	@mkdir -p Kernel/ARTIFACTS
	@mkdir -p User/ARTIFACTS
	@touch Kernel/ARTIFACTS/settings.mk
	@touch User/ARTIFACTS/settings.mk

# Clean modules
ifeq ($(target), x86_i386)
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=i386 clean
else ifeq ($(target), x86_64)
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=x86_64 clean
else
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=i386 clean
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=x86_64 clean
endif

	@make -C $(KERNEL_SRC_DIR)/core clean
	@make -C $(KERNEL_SRC_DIR)/io clean
	@make -C $(KERNEL_SRC_DIR)/time clean
	@make -C $(KERNEL_SRC_DIR)/libs clean
	@make -C $(KERNEL_SRC_DIR)/user clean
	@make -C $(KERNEL_SRC_DIR)/sync clean
	@make -C $(KERNEL_SRC_DIR)/fs clean
	@make -C $(KERNEL_SRC_DIR)/test_framework clean


	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned kernel \e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@make -C $(USER_SRC_DIR)/libs clean
	@make -C $(USER_SRC_DIR)/initrd clean

	@rm -rf Kernel/ARTIFACTS
	@rm -rf User/ARTIFACTS

run:
	@make run -f $(CONFIG_DIR)/arch/$(target)/makerun.mk KERNEL_BUILD_DIR=$(KERNEL_ARTIFACTS_DIR)/$(KERNEL_BUILD_DIR) KERNEL=$(KERNEL)

debug:
	@make debug -f $(CONFIG_DIR)/arch/$(target)/makerun.mk KERNEL_BUILD_DIR=$(KERNEL_ARTIFACTS_DIR)/$(KERNEL_BUILD_DIR) KERNEL=$(KERNEL)

qemu-test-mode:
	@make qemu-test-mode -f $(CONFIG_DIR)/arch/$(target)/makerun.mk KERNEL_BUILD_DIR=$(KERNEL_ARTIFACTS_DIR)/$(KERNEL_BUILD_DIR) KERNEL=$(KERNEL)