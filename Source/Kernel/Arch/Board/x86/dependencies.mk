DEP_INCLUDES =  -I ../../../Libs/libc/includes
DEP_INCLUDES += -I ../../../Libs/libapi/includes
DEP_INCLUDES += -I ../../../IO/includes
DEP_INCLUDES += -I ../../../Core/includes
DEP_INCLUDES += -I ../../../Libs/libtrace/includes

ifeq ($(target_cpu), i386)
	DEP_INCLUDES += -I ../../CPU/i386/includes
else ifeq ($(target_cpu), x86_64)
	DEP_INCLUDES += -I ../../CPU/x86_64/includes
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown CPU architecture $(target_cpu)\e[22m\e[39m"
	@echo "Available architectures: $(CPU_ARCH_LIST)\n"
	@exit 1
endif

ifeq ($(TESTS), TRUE)
	DEP_INCLUDES += -I ../../../TestFramework/includes
endif

DEP_LIBS =