################################################################################
# UTK Makefile
#
# Created: 29/03/2023
#
# Author: Alexy Torres Aurora Dugo
#
# Run make file, used to build the final kernel image and run it on QEMU or
# virtualbox.
################################################################################

QEMUOPTS = -cpu Nehalem -d guest_errors -rtc base=localtime -m 256M \
           -gdb tcp::1234 -smp 4 -serial stdio

QEMU = qemu-system-x86_64

######################### Qemu options
pre-run:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Preparing run for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"
	rm -rf ./$(BUILD_DIR)/GRUB
	$(RM) -f ./$(BUILD_DIR)/utk_boot.iso
	cp -R Config/Arch/x86_64/GRUB ./$(BUILD_DIR)/
	cp ./$(BUILD_DIR)/$(KERNEL).elf ./$(BUILD_DIR)/GRUB/boot/
#cp ./$(BUILD_DIR)/utk.initrd ./$(BUILD_DIR)/GRUB/boot/
	grub-mkrescue -o ./$(BUILD_DIR)/utk_boot.iso ./$(BUILD_DIR)/GRUB

run: pre-run
	@echo "\e[1m\e[94m=== Running on Qemu\e[22m\e[39m"
	@$(QEMU) $(QEMUOPTS) -boot d -cdrom ./$(BUILD_DIR)/utk_boot.iso

qemu-test-mode: pre-run
	@echo "\e[1m\e[94m=== Running on Qemu TEST MODE\e[22m\e[39m"
	@$(QEMU) $(QEMUOPTS) -boot d -cdrom ./$(BUILD_DIR)/utk_boot.iso -nographic -monitor none

debug: pre-run
	@echo "\e[1m\e[94m=== Running on Qemu DEBUG MODE\e[22m\e[39m"
	@$(QEMU) $(QEMUOPTS) -boot d -cdrom ./$(BUILD_DIR)/utk_boot.iso -S
