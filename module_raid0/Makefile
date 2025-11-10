# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux ext4-filesystem routines.
#

obj-m += module_raid0.o

module_raid0-y	:= super.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

module_raid0-y	+= fs/read_write.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
