# Tên module
obj-m += gpio-sysfs.o

# Đường dẫn tới source kernel đã build
KDIR := /home/anhln/BBB/linux
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

