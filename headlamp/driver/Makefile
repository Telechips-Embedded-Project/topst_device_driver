obj-m += headlamp_driver.o
KDIR := /home/jsy/topst/build-main/tmp/work/tcc8050_main-telechips-linux/linux-telechips/5.4.159-r0/build
CROSS_COMPILE := aarch64-telechips-linux-
ARCH := arm64

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
