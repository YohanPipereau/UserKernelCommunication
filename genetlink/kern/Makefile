CC := $(CROSS_COMPILE)gcc
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := genlbench_test.o genlbench.o
else
	# Otherwise we were called directly from the command
	# line; invoke the kernel build system.
	KDIR := /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
endif

all:
	$(MAKE) -C $(KDIR) M=${shell pwd} modules

clean:
	-$(MAKE) -C $(KDIR) M=${shell pwd} clean || true
	-rm *.o *.ko *.mod.{c,o} modules.order Module.symvers || true
