# Tên module
obj-m += led_driver_device_tree.o

# Đường dẫn tới source kernel đã build
KDIR := /home/anhln/BBB/linux-stable-rcn-ee-6.15.4-bone18
KDIR_HOST := /lib/modules/$(shell uname -r)/build
# Thư mục hiện tại
PWD := $(shell pwd)

# Toolchain cross-compile cho ARM
CROSS_COMPILE := arm-linux-gnueabihf-

# Kiến trúc ARM
ARCH := arm

# Device Tree source và binary files
DTS_SRC := bbb-led.dtso
DTB_TARGET := bbb-led.dtbo

# Mục build cho ARM - bao gồm cả module và device tree overlay
all: modules dtb

# Build kernel modules
modules:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

# Build device tree overlay
dtb: $(DTB_TARGET)

$(DTB_TARGET): $(DTS_SRC)
	@echo "Building device tree overlay: $(DTS_SRC) -> $(DTB_TARGET)"
	dtc -@ -I dts -O dtb -o $(DTB_TARGET) $(DTS_SRC)
	@echo "Device tree overlay built successfully"

# Mục build cho host
host:
	make -C $(KDIR_HOST) M=$(PWD) modules

# Mục install overlay vào configfs
install-overlay: $(DTB_TARGET)
	@echo "Installing overlay to configfs..."
	sudo mkdir -p /sys/kernel/config/device-tree/overlays/bbb-led
	sudo cp $(DTB_TARGET) /sys/kernel/config/device-tree/overlays/bbb-led/dtbo
	@echo "Overlay installed successfully"

# Mục remove overlay từ configfs
remove-overlay:
	@echo "Removing overlay from configfs..."
	-sudo rmdir /sys/kernel/config/device-tree/overlays/bbb-led 2>/dev/null || true
	@echo "Overlay removed"

# Mục clean
clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	rm -f $(DTB_TARGET)

# Phony targets
.PHONY: all modules dtb host install-overlay remove-overlay clean

