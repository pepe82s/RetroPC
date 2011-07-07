ifneq ($(KERNELRELEASE),)
obj-m := rpcdrv.o

else
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: rpcdrv.ko
	sudo insmod rpcdrv.ko

rpcdrv.ko: default

uninstall:
	sudo rmmod rpcdrv
endif
