DEP_INCLUDES  = -I ../Libs/libc/includes
DEP_INCLUDES += -I ../Libs/libapi/includes
DEP_INCLUDES += -I ../Libs/libtrace/includes
DEP_INCLUDES += -I ../Time/includes
DEP_INCLUDES += -I ../Core/includes

ifeq ($(target_cpu), i386)
	DEP_INCLUDES += -I ../Arch/CPU/i386/includes
	DEP_INCLUDES += -I ../Arch/Board/x86/includes
else ifeq ($(target_cpu), x86_64)
	DEP_INCLUDES += -I ../Arch/CPU/x86_64/includes
	DEP_INCLUDES += -I ../Arch/Board/x86/includes
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown CPU architecture $(target_cpu)\e[22m\e[39m"
	@echo "Available architectures: $(CPU_ARCH_LIST)\n"
	@exit 1
endif

DEP_LIBS =