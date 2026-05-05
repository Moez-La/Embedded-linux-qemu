KDIR := /tmp/buildroot-2023.11/output/build/linux-6.1.44
CROSS_COMPILE := /tmp/buildroot-2023.11/output/host/bin/arm-buildroot-linux-gnueabi-
ARCH := arm

obj-m += chardev.o

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
