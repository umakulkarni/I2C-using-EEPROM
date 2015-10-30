obj-m:= i2c_flash.o

ARCH=x86
CROSS_COMPILE=i586-poky-linux-

all:
	make -C /home/uma/SDK/sysroots/i586-poky-linux/usr/src/kernel M=$(PWD) modules

clean:
	rm -fr *.o
	rm -fr *.ko
	rm -fr *.mod.c
	rm -fr *.order
	rm -fr *.symvers

