export ARCH=arm
export CROSS_COMPILE=/home/jiangxj/r16_xlj/sdk/lichee/brandy/gcc-linaro/bin/arm-linux-gnueabi-
EXTRA_CFLAGS += $(DEBFLAGS) -Wall

obj-m += xlj_pwm.o

KDIR ?= ~/r16_xlj/sdk/lichee/linux-3.4
PWD := $(shell pwd)
all:
	make $(EXTRA_CFLAGS) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.c *.symvers modules.order .pwm* .tmp_versions *.c~ *.cmd .xlj_pwm*

