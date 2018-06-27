CC := $(CROSS_COMPILE)gcc

obj-m := mmapdev.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean :
	$(MAKE) -C $(KDIR) M=$(PWD) clean

