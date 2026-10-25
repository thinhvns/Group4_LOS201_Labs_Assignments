# ============================================================
# Embedded Linux Assignment 02
# Author      : Tran Nguyen Hai Dang
# Student ID  : SE202009
# Target      : ARMv7 / Linux 5.15
# ============================================================

KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15

ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

obj-m += sms_sensor_driver.o

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) \
		M=$(CURDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		modules

clean:
	$(MAKE) -C $(KDIR) \
		M=$(CURDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		clean
