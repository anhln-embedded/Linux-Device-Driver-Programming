# Tên module
obj-m += led.o

# Tắt stack protection
ccflags-y += -fno-stack-protector

# Đường dẫn tới source kernel đã build
KDIR := /home/anhln/BBB/linux-stable-rcn-ee-6.15.4-bone18
KDIR_HOST := /lib/modules/$(shell uname -r)/build
# Thư mục hiện tại
PWD := $(shell pwd)

# Toolchain cross-compile cho ARM
CROSS_COMPILE := arm-linux-gnueabihf-

# Kiến trúc ARM
ARCH := arm

# Mục build cho ARM
all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

# Mục build cho host
host:
	make -C $(KDIR_HOST) M=$(PWD) modules

# Mục clean
clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

