KERNELDIR ?= /lib/modules/$(shell uname -r)/build/
PWD := $(shell pwd)

ifneq ($(KERNELRELEASE),)

obj-m := bmb.o

else

.PHONY: default modules clean

default: modules

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

endif
