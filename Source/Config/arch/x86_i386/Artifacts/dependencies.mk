DEP_INCLUDES =
DEP_LIBS     =
DEP_MODULES  =

ifeq ($(TESTS), TRUE)
DEP_LIBS += --start-group -ltestframework
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

DEP_MODULES += -L../arch/bin
DEP_MODULES += -L../io/bin
DEP_MODULES += -L../libs/bin
DEP_MODULES += -L../core/bin
DEP_MODULES += -L../time/bin
DEP_MODULES += -L../ARTIFACTS
ifeq ($(TESTS), TRUE)
DEP_MODULES += -L../test_framework/bin
endif