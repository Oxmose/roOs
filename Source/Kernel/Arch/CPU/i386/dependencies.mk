DEP_INCLUDES =  -I ../../../Libs/libc/includes
DEP_INCLUDES += -I ../../../Libs/libapi/includes
DEP_INCLUDES += -I ../../../IO/includes
DEP_INCLUDES += -I ../../../Core/includes
DEP_INCLUDES += -I ../../Board/x86/includes
DEP_INCLUDES += -I ../../../Libs/libtrace/includes

ifeq ($(TESTS), TRUE)
	DEP_INCLUDES += -I ../../../TestFramework/includes
endif

DEP_LIBS =