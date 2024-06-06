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

QEMUOPTS = -cpu coreduo,-syscall,-lm -d guest_errors -rtc base=localtime -m 256M \
           -smp 8 -serial stdio

QEMU = qemu-system-i386

######################### Qemu options
pre-run:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Preparing run for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"
	rm -rf ./$(BUILD_DIR)/GRUB
	$(RM) -f ./$(BUILD_DIR)/utk_boot.iso
	cp -R Config/Arch/x86_i386/GRUB ./$(BUILD_DIR)/
	cp ./$(BUILD_DIR)/$(KERNEL).elf ./$(BUILD_DIR)/GRUB/boot/
#cp ./$(BUILD_DIR)/utk.initrd ./$(BUILD_DIR)/GRUB/boot/
	grub-mkrescue -d ../Tools/i386-pc -o ./$(BUILD_DIR)/utk_boot.iso ./$(BUILD_DIR)/GRUB 2>&1 /dev/null

run: pre-run
	@echo "\e[1m\e[94m=== Running on Qemu\e[22m\e[39m"
	@$(QEMU) $(QEMUOPTS) -boot d -cdrom ./$(BUILD_DIR)/utk_boot.iso -monitor telnet:127.0.0.1:55555,server,nowait

qemu-test-mode: pre-run
	@echo "\e[1m\e[94m=== Running on Qemu TEST MODE\e[22m\e[39m"
	@$(QEMU) $(QEMUOPTS) -boot d -cdrom ./$(BUILD_DIR)/utk_boot.iso -nographic -monitor none

debug: pre-run
	@echo "\e[1m\e[94m=== Running on Qemu DEBUG MODE\e[22m\e[39m"
	@$(QEMU) $(QEMUOPTS) -boot d -cdrom ./$(BUILD_DIR)/utk_boot.iso -S -monitor telnet:127.0.0.1:55555,server,nowait -gdb tcp::1234
