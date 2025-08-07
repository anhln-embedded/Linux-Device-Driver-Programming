#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#define DEV_MEM_SIZE 512

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnhLN");
MODULE_DESCRIPTION("");

/*pseudo device's memmory */
char device_buffer[DEV_MEM_SIZE];

/*This holds the device number*/
dev_t device_number;

/* Cdev variable*/
struct cdev pcd_cdev;

struct class *class_pcd;
struct device *device_pcd;


/* Function prototypes */
loff_t pcd_lseek(struct file *filp, loff_t off, int whence);
ssize_t pcd_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
ssize_t pcd_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *filp);

/*file operations of the driver*/
struct file_operations pcd_fops = {
	.owner = THIS_MODULE,
	.open = pcd_open,
	.read = pcd_read,
	.write = pcd_write,
	.llseek = pcd_lseek,
	.release = pcd_release,
};

static int __init pcd_driver_init(void)
{
	int ret;
    /*1> Dynamically allocate a device number*/
    ret = alloc_chrdev_region(&device_number, 0, 1, "pseudo_device");
	if (ret < 0) {
		printk(KERN_ALERT "Failed to allocate device number\n");
		goto out;
	}
	printk(KERN_INFO "%s :Device number allocated <major>:<minor> %d:%d\n",__func__ ,MAJOR(device_number), MINOR(device_number));

    /*2> Initialize the cdev structure*/
    cdev_init(&pcd_cdev, &pcd_fops);

    /*3> Register a device (cdev struct) with VFS*/
	pcd_cdev.owner = THIS_MODULE;
    ret = cdev_add(&pcd_cdev, device_number, 1);
	if (ret < 0) {
		printk(KERN_ALERT "Failed to add cdev\n");
		goto unreg_chrdev;
	}

	/*4> Create device class under /sys/class/ */
	class_pcd = class_create("pcd_class");
	if (IS_ERR(class_pcd)) {

		printk(KERN_ALERT "Failed to create class\n");
		ret = PTR_ERR(class_pcd);
		goto cdev_del;
	}

	/*5> populate the sysfs with device information*/
	device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
	if (IS_ERR(device_pcd)) {
		printk(KERN_ALERT "Failed to create device\n");
		ret = PTR_ERR(device_pcd);
		goto class_destroy;
	}

	printk(KERN_INFO "Pseudo device driver initialized\n");	
    return 0;

class_destroy:
	class_destroy(class_pcd);
cdev_del:
	cdev_del(&pcd_cdev);
unreg_chrdev:
	unregister_chrdev_region(device_number, 1);
out: 
	return ret;
}

static void __exit pcd_driver_exit(void)
{
	/*6> Destroy the device*/
	device_destroy(class_pcd, device_number);
	/*7> Destroy the class*/
	class_destroy(class_pcd);
	/*8> Unregister the device number*/
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number, 1);
	printk(KERN_INFO "Pseudo device driver exited\n");
}

loff_t pcd_lseek(struct file *filp, loff_t off, int whence)
{	
	printk(KERN_INFO "lseek requested\n");
	printk(KERN_INFO "current file offset: %lld\n", filp->f_pos);
	loff_t temp;
	switch (whence) {
		case SEEK_SET: 
			if (off < 0 || off > DEV_MEM_SIZE) {
				return -EINVAL; // Invalid argument
			}
			filp->f_pos = off;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + off;
			if (temp < 0 || temp > DEV_MEM_SIZE) {
				return -EINVAL; // Invalid argument
			}
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = DEV_MEM_SIZE + off; 
			if(temp < 0 || temp > DEV_MEM_SIZE) {
				return -EINVAL; // Invalid argument
			}
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL; // Invalid argument
	}
	printk(KERN_INFO "lseek completed, new file offset: %lld\n", filp->f_pos);
    return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "read requested\n");
	printk(KERN_INFO "current file offset: %lld\n", *off);
	
	/* Check if we're at or beyond the end of the buffer */
	if (*off >= DEV_MEM_SIZE) {
		printk(KERN_INFO "EOF reached\n");
		return 0; /* EOF */
	}
	
	/* Adjust the 'count'*/
	if((*off + len) > DEV_MEM_SIZE)
	{
		len = DEV_MEM_SIZE - *off;
	}

	/*coppy to user*/
	if(copy_to_user(buf, device_buffer + *off, len))
	{
		printk(KERN_ALERT "Failed to copy data to user\n");
		return -EFAULT;
	}

	/*Update the offset*/
	*off += len;
	printk(KERN_INFO "read completed, bytes read: %zu\n", len);
	printk(KERN_INFO "updated file offset: %lld\n", *off);
    return len;
}

ssize_t pcd_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "write requested\n");
	printk(KERN_INFO "current file offset: %lld\n", *off);
	/* Adjust the 'count'*/
	if((*off + len) > DEV_MEM_SIZE)
	{
		len = DEV_MEM_SIZE - *off;
	}

	if(len <= 0)
	{
		printk(KERN_ALERT "No space left to write\n");
		return -ENOMEM;
	}

	/*coppy from user*/
	if(copy_from_user(device_buffer + *off, buf, len))
	{
		printk(KERN_ALERT "Failed to copy data from user\n");
		return -EFAULT;
	}
	/*Update the offset*/
	*off += len;
	printk(KERN_INFO "write completed, bytes written: %zu\n", len);
	printk(KERN_INFO "updated file offset: %lld\n", *off);
	printk(KERN_INFO "Data in device buffer: %.*s\n", (int)len, device_buffer + *off - len);
    return len;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Device opened\n");
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Device closed\n");
	/* Note: Don't reset f_pos or f_flags - VFS handles this */
	/* Only clear device-specific resources if needed */
	return 0;
}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);
