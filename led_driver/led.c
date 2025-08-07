#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define DEVICE		5
#define DEVICE_NAME "led_30"

#define GPIO_ADDR_BASE  0x44E07000
#define ADDR_SIZE       (0x1000)
#define GPIO_SETDATAOUT_OFFSET          0x194
#define GPIO_CLEARDATAOUT_OFFSET        0x190
#define DATA_IN_REG						0x138
#define GPIO_OE_OFFSET                  0x134
#define LED                             ~(1 << 30)
#define DATA_OUT			(1 << 30)


#define MAGIC_NO	100
#define SEND_DATA_CMD	_IOW(MAGIC_NO, 1, char*)
#define IOCTL_DATA_LEN 1024

static dev_t dev_num;
struct class *my_class;
struct cdev my_cdev;
char data[4096];
char config_data[IOCTL_DATA_LEN];


void __iomem *base_addr;
uint32_t reg_data;
uint32_t old_pin_mode;

static int my_open(struct inode *inode, struct file *file);
static int my_close(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *flip, char __user *user_buf, size_t len, loff_t *offs);
static ssize_t my_write(struct file *flip, const char __user *user_buf, size_t len, loff_t *offs);
static long my_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write,
	.unlocked_ioctl = my_ioctl,
};

static int __init func_init(void)
{
	memset(data, 0, sizeof(data));
	alloc_chrdev_region(&dev_num, 0, DEVICE, DEVICE_NAME);
	my_class = class_create( DEVICE_NAME);
	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;
	cdev_add(&my_cdev, dev_num, 1);
	device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);

	base_addr = ioremap(GPIO_ADDR_BASE, ADDR_SIZE);
	printk(KERN_INFO "LED driver initialized\n");
	return 0;
}


static void __exit func_exit(void)
{
	cdev_del(&my_cdev);
	device_destroy(my_class, dev_num);
	class_destroy(my_class);
	unregister_chrdev(dev_num, DEVICE_NAME);
	iounmap(base_addr);
	printk(KERN_INFO "LED driver exited\n");
}

static int my_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "LED device opened\n");
	/* Initialize GPIO for LED control */
	reg_data = readl_relaxed(base_addr + GPIO_OE_OFFSET); // GPIO_OE
	old_pin_mode = reg_data;
	reg_data &= LED; // Set pin 30 as output
	writel_relaxed(reg_data, base_addr + GPIO_OE_OFFSET);

	return 0;
}
static int my_close(struct inode *inode, struct file *file){
	printk(KERN_INFO "LED device closed\n");
	return 0;
}
static ssize_t my_read(struct file *flip, char __user *user_buf, size_t len, loff_t *offs){
	char *data = NULL;
	printk(KERN_INFO "AnhLN %s, %d, read request\n", __func__, __LINE__);
	data = kmalloc(len, GFP_KERNEL);
	memset(data, 0, len);

	return 0;
}	
static ssize_t my_write(struct file *flip, const char __user *user_buf, size_t len, loff_t *offs){
	char data = '\0';
	printk(KERN_INFO "AnhLN %s, %d, write request\n", __func__, __LINE__);
	if (copy_from_user(&data, user_buf, sizeof(data))) {
		printk(KERN_ALERT "Failed to copy data from user\n");
		return -EFAULT;
	}
	switch(data)
	{
		case '0':
			writel_relaxed(DATA_OUT, base_addr + GPIO_SETDATAOUT_OFFSET);
			break;
		case '1':
			writel_relaxed(DATA_OUT, base_addr + GPIO_CLEARDATAOUT_OFFSET);
			break;
		default:
			printk(KERN_ALERT "Invalid data received: '%c' (ASCII: %d)\n", data, (int)data);
			return -EINVAL;
	}
	return len;
}
static long my_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){
	int ret;
	switch(cmd)
	{
		case SEND_DATA_CMD:
			memset(config_data, 0, IOCTL_DATA_LEN);
			ret = copy_from_user(config_data, (char*)arg, IOCTL_DATA_LEN);
			if (ret != 0) {
				printk(KERN_ALERT "Failed to copy data from user: %d bytes remaining\n", ret);
				return -EFAULT;
			}
			printk(KERN_INFO "AnhLN %s, %d, ioctl message = %s\n", __func__, __LINE__, config_data);
		break;
		default:
			return -EINVAL; // Invalid command
	}
	return 0;
}

module_init(func_init);
module_exit(func_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnhLN");
MODULE_DESCRIPTION("A example character device driver");