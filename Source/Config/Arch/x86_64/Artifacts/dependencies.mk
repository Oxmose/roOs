DEP_INCLUDES =

DEP_LIBS =  -larch -lcore -lio -llibc -lapi -ltrace -ltestframework

DEP_MODULES = -L../Arch/bin -L../IO/bin -L../Libs/bin -L../Core/bin -L../Libs/bin -L../TestFramework/bin

ifeq ($(TESTS), TRUE)
DEP_LIBS    += -larch
DEP_MODULES += -L../TestFramework/bin
endif