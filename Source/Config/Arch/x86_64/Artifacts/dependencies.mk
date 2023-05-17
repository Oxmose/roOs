DEP_INCLUDES =

DEP_LIBS =  -larch
DEP_LIBS += -lcore
DEP_LIBS += -lio
DEP_LIBS += -llibc
DEP_LIBS += -lapi
DEP_LIBS += -ltrace
DEP_LIBS += -ltime

DEP_MODULES =  -L../Arch/bin
DEP_MODULES += -L../IO/bin
DEP_MODULES += -L../Libs/bin
DEP_MODULES += -L../Core/bin
DEP_MODULES += -L../Time/bin


ifeq ($(TESTS), TRUE)
DEP_LIBS    += -ltestframework
DEP_MODULES += -L../TestFramework/bin
endif