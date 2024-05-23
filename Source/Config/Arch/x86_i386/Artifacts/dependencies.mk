DEP_INCLUDES =
DEP_LIBS     =
DEP_MODULES  =

ifeq ($(TESTS), TRUE)
DEP_LIBS = --start-group -ltestframework
endif
DEP_LIBS += --whole-archive -larch
DEP_LIBS += -lcore
DEP_LIBS += -lio
DEP_LIBS += -lapi
DEP_LIBS += -ltrace
DEP_LIBS += -ltime
DEP_LIBS += -llibc
DEP_LIBS += --whole-archive -lrawdtb
ifeq ($(TESTS), TRUE)
DEP_LIBS += --end-group
endif

DEP_MODULES += -L../Arch/bin
DEP_MODULES += -L../IO/bin
DEP_MODULES += -L../Libs/bin
DEP_MODULES += -L../Core/bin
DEP_MODULES += -L../Time/bin
DEP_MODULES += -L../ARTIFACTS
ifeq ($(TESTS), TRUE)
DEP_MODULES += -L../TestFramework/bin
endif