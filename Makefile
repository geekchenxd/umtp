# Default compiler settings
WARNINGS = -Wall
APP_TOPDIR	:= $(shell pwd)
LD_LIBS := -lm -lpthread

CROSS_COMPILE :=

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
RANLIB = $(CROSS_COMPILE)ranlib
LD = $(CROSS_COMPILE)ld

TOPDIR := $(PWD)
SRC_DIR := $(TOPDIR)/core
INCLUDE_DIR := $(TOPDIR)/include
LIBUMTP := umtp
EXAMPLES_DIR = $(TOPDIR)/examples

UMTP_LDFLAGS := -L$(TOPDIR)/core -l$(LIBUMTP)
CFLAGS  += $(WARNINGS) -O2
LDFLAGS += $(LD_LIBS) $(UMTP_LDFLAGS)
CFLAGS += -I$(INCLUDE_DIR)

all: examples
.PHONY : all hsfctl libconfig install clean

export CC AR RANLIB STRIP CFLAGS LDFLAGS APP_TOPDIR INCLUDE_DIR CROSS_COMPILE

libumtp:
	@$(MAKE) -C $(SRC_DIR)

examples:libumtp
	@$(MAKE) -C $(EXAMPLES_DIR) all

clean:
	@$(MAKE) -C $(SRC_DIR) clean
	@$(MAKE) -C $(EXAMPLES_DIR) clean
	@$(RM) -r lib share $(TARGET) $(OBJS)

