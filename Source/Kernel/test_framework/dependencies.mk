DEP_INCLUDES  = -I ../io/includes
DEP_INCLUDES += -I ../core/includes
DEP_INCLUDES += -I ../libs/libc/includes
DEP_INCLUDES += -I ../libs/libtrace/includes
DEP_INCLUDES += -I ../libs/libapi/includes
DEP_INCLUDES += -I ../arch/cpu/includes
DEP_INCLUDES += -I ../sync/includes

ifeq ($(target), x86_i386)
	DEP_INCLUDES += -I ../arch/cpu/i386/includes_private
else ifeq ($(target), x86_64)
	DEP_INCLUDES += -I ../arch/cpu/x86_64/includes_private
endif