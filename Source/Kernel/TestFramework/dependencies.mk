DEP_INCLUDES  = -I ../IO/includes
DEP_INCLUDES += -I ../Core/includes
DEP_INCLUDES += -I ../Libs/libc/includes
DEP_INCLUDES += -I ../Libs/libtrace/includes
DEP_INCLUDES += -I ../Libs/libapi/includes

ifeq ($(target), x86_i386)
# Build the architecture module
	DEP_INCLUDES += -I ../Arch/CPU/i386/includes
else ifeq ($(target), x86_64)
# Build the architecture module
	DEP_INCLUDES += -I ../Arch/CPU/x86_64/includes
endif

DEP_LIBS =