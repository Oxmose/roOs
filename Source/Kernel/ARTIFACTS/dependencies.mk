DEP_INCLUDES =

DEP_LIBS =  -larch

DEP_MODULES = -L../Arch/bin

ifeq ($(TESTS), TRUE)
DEP_LIBS    += -larch
DEP_MODULES += -L../tests/bin
endif