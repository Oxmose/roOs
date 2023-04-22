DEP_INCLUDES =

DEP_LIBS =  -larch -lcore -lio -llibc -lapi -ltrace

DEP_MODULES = -L../Arch/bin -L../IO/bin -L../Libs/bin -L../Core/bin -L../Libs/bin

ifeq ($(TESTS), TRUE)
DEP_LIBS    += -larch
DEP_MODULES += -L../tests/bin
endif