ifneq ($(KERNELRELEASE),)
obj-m := rpcdrv.o

else
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: rpcdrv.ko
	sudo mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/misc/rpcdrv
	sudo cp rpcdrv.ko /lib/modules/$(shell uname -r)/kernel/drivers/misc/rpcdrv/
	sudo depmod -a
	sudo modprobe rpcdrv
	sudo cp 10-rpc.rules /lib/udev/rules.d/

rpcdrv.ko: default

uninstall:
	sudo modprobe -r rpcdrv
	sudo rm -R /lib/modules/$(shell uname -r)/kernel/drivers/misc/rpcdrv
	sudo rm /lib/udev/rules.d/10-rpc.rules

clean:
	rm -f rpcdrv.o rpcdrv.ko Module.symvers modules.order rpcdrv.mod.c rpcdrv.mod.o ./.rpcdrv.*

endif
