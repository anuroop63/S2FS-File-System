obj-m += s2fs.o

CONFIG_MODULE_SIG=n

CFLAGS_s2fs.o := -DDEBUG

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

mount:
	sudo insmod s2fs.ko
	sudo mount -t s2fs nodev mnt
	
umount:
	sudo umount ./mnt
	sudo rmmod s2fs.ko
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
