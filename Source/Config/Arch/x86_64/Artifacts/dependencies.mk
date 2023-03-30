DEP_INCLUDES =

DEP_LIBS =  -larch -lio -llibc -lapi -lio

DEP_MODULES = -L../Arch/bin -L../IO/bin -L../Libs/bin

ifeq ($(TESTS), TRUE)
DEP_LIBS    += -larch
DEP_MODULES += -L../tests/bin
endif