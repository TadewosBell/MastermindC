# Basic Makefile to pull in kernel's KBuild to build an out-of-tree
# kernel module

KDIR ?= /lib/modules/$(shell uname -r)/build

all: modules

clean modules:
	$(MAKE) -C $(KDIR) M=$$PWD $@
