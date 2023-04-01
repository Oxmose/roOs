DEP_INCLUDES =  -I ../Libs/libc/includes
DEP_INCLUDES += -I ../Libs/libapi/includes
DEP_INCLUDES += -I ../IO/includes

ifeq ($(target_cpu), i386)
	DEP_INCLUDES += -I ../Arch/CPU/i386/includes
else ifeq ($(target_cpu), x86_64)
	DEP_INCLUDES += -I ../Arch/CPU/x86_64/includes
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown CPU architecture $(target_cpu)\e[22m\e[39m"
	@echo "Available architectures: $(CPU_ARCH_LIST)\n"
	@exit 1
endif

DEP_LIBS =