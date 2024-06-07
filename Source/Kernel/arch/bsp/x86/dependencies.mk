DEP_INCLUDES =  -I ../../../libs/libc/includes
DEP_INCLUDES += -I ../../../libs/libapi/includes
DEP_INCLUDES += -I ../../../io/includes
DEP_INCLUDES += -I ../../../core/includes
DEP_INCLUDES += -I ../../../libs/libtrace/includes
DEP_INCLUDES += -I ../../../test_framework/includes
DEP_INCLUDES += -I ../../../time/includes
DEP_INCLUDES += -I ../../cpu/includes

ifeq ($(target_cpu), i386)
	DEP_INCLUDES += -I ../../cpu/i386/includes_private
else ifeq ($(target_cpu), x86_64)
	DEP_INCLUDES += -I ../../cpu/x86_64/includes_private
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown CPU architecture $(target_cpu)\e[22m\e[39m"
	@echo "Available architectures: $(CPU_ARCH_LIST)\n"
	@exit 1
endif

DEP_LIBS =