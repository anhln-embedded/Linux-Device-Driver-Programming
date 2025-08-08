# LED Driver với Device Tree Overlay cho BeagleBone Black

## Tổng quan

Project này implements một LED driver sử dụng Device Tree overlay để control LED thông qua GPIO trên BeagleBone Black. Driver sử dụng GPIO0_30 (P9_11) để điều khiển LED.

## Cấu trúc Project

```
led_driver_device_tree/
├── led_driver_device_tree.c    # Driver source code
├── bbb-led.dtso               # Device Tree overlay source
├── Makefile                   # Build script
├── README.md                  # Hướng dẫn này
├── led_driver_device_tree.ko  # Compiled kernel module (sau khi build)
└── bbb-led.dtbo              # Compiled device tree overlay (sau khi build)
```

## Hardware Setup

### Kết nối LED

```
BeagleBone Black P9_11 (GPIO0_30) --> Resistor (220Ω) --> LED Anode
LED Cathode --> GND (P9_1 hoặc P9_2)
```

### Pinout Reference
- **P9_11**: GPIO0_30 (Driver output)
- **P9_1/P9_2**: Ground

## Build Instructions

### Prerequisites

```bash
# Cài đặt toolchain cross-compile
sudo apt-get install gcc-arm-linux-gnueabihf

# Có sẵn kernel source đã compile cho BBB
# Đường dẫn trong Makefile: /home/anhln/BBB/linux-stable-rcn-ee-6.15.4-bone18
```

### Build Process

```bash
# Build cả kernel module và device tree overlay
make all

# Hoặc build riêng lẻ
make modules    # Chỉ build kernel module
make dtb       # Chỉ build device tree overlay

# Clean build artifacts
make clean
```

### Build Output

Sau khi build thành công sẽ có:
- `led_driver_device_tree.ko` - Kernel module
- `bbb-led.dtbo` - Device tree overlay binary

## Deployment và Testing

### Bước 1: Copy Files sang BeagleBone Black

```bash
# Từ máy development
scp led_driver_device_tree.ko bbb-led.dtbo debian@<BBB_IP>:~/
```

### Bước 2: Setup trên BeagleBone Black

```bash
# SSH vào BBB
ssh debian@<BBB_IP>

# 1. Load Device Tree Overlay
sudo mkdir -p /sys/kernel/config/device-tree/overlays/bbb-led
sudo cp bbb-led.dtbo /sys/kernel/config/device-tree/overlays/bbb-led/dtbo

# 2. Verify overlay loaded
ls /sys/kernel/config/device-tree/overlays/
```

### Bước 3: Load và Test Driver

```bash
# 1. Load driver module
sudo insmod led_driver_device_tree.ko

# 2. Verify device creation
ls -la /dev/bbb-led*

# 3. Check kernel messages
dmesg | tail -10

# 4. Test LED control
echo 1 | sudo tee /dev/bbb-led    # Bật LED
sleep 2
echo 0 | sudo tee /dev/bbb-led    # Tắt LED
```

### Bước 4: Cleanup

```bash
# Remove driver
sudo rmmod led_driver_device_tree

# Remove overlay
sudo rmdir /sys/kernel/config/device-tree/overlays/bbb-led
```

## Driver API

### Device File
- **Path**: `/dev/bbb-led`
- **Permissions**: Writable by root/sudo

### Operations

#### Write Operation
```bash
# Bật LED
echo 1 > /dev/bbb-led

# Tắt LED  
echo 0 > /dev/bbb-led
```

#### Read Operation
```bash
# Đọc trạng thái hiện tại
cat /dev/bbb-led
# Output: 0 (OFF) hoặc 1 (ON)
```

## Device Tree Overlay Details

### bbb-led.dtso

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "ti,beaglebone", "ti,beaglebone-black";

    fragment@0 {
        target-path = "/";
        __overlay__ {
            bbb_led: bbb-led@0 {
                compatible = "anhln,bbb-led";
                status = "okay";
                led-gpios = <&gpio0 30 0>; // GPIO0_30 (P9_11), GPIO_ACTIVE_HIGH
            };
        };
    };
};
```

### Key Properties
- **compatible**: `"anhln,bbb-led"` - Driver matching string
- **led-gpios**: `<&gpio0 30 0>` - GPIO0_30, Active High
- **target-path**: `"/"` - Add to root node

## Troubleshooting

### Common Issues

#### 1. Device không được tạo
```bash
# Check dmesg for errors
dmesg | grep -i "bbb-led\|gpio\|overlay"

# Verify overlay loaded
cat /sys/kernel/config/device-tree/overlays/bbb-led/status
```

#### 2. Permission denied khi write
```bash
# Use sudo
echo 1 | sudo tee /dev/bbb-led

# Check device permissions
ls -la /dev/bbb-led*
```

#### 3. GPIO already in use
```bash
# Check GPIO usage
cat /sys/kernel/debug/gpio

# Find conflicting drivers
lsmod | grep gpio
```

#### 4. Overlay compilation error
```bash
# Check device tree syntax
dtc -@ -I dts -O dtb -o test.dtbo bbb-led.dtso

# Common fixes:
# - Ensure GPIO node references are correct
# - Check syntax for missing semicolons/braces
```

### Debug Commands

```bash
# Check loaded overlays
ls /sys/kernel/config/device-tree/overlays/

# Check GPIO status
cat /sys/kernel/debug/gpio | grep -A5 -B5 "gpio-30"

# Monitor kernel messages
dmesg -w

# Check driver loaded
lsmod | grep led_driver

# Verify device major/minor
ls -la /dev/bbb-led*
cat /proc/devices | grep bbb-led
```

## Code Structure

### Driver Components

1. **Platform Driver**: Matches device tree compatible string
2. **GPIO Management**: Uses GPIO descriptor API
3. **Character Device**: Provides user space interface
4. **File Operations**: Read/Write LED state

### Key Functions

- `led_probe()`: Device initialization and GPIO setup
- `led_remove()`: Cleanup and GPIO release  
- `led_write()`: Handle user write commands
- `led_read()`: Return current LED state

## Development Notes

### GPIO API Usage
```c
// Get GPIO from device tree
led_gpio = gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);

// Set GPIO value
gpiod_set_value(led_gpio, value);

// Get GPIO value  
current_value = gpiod_get_value(led_gpio);
```

### Device Tree Integration
```c
static const struct of_device_id led_of_match[] = {
    { .compatible = "anhln,bbb-led" },
    { }
};
MODULE_DEVICE_TABLE(of, led_of_match);
```

## License

This project is for educational purposes. Modify and use as needed.

## Author

- **anhln-embedded**
- Linux Device Driver Programming
- BeagleBone Black GPIO Control
