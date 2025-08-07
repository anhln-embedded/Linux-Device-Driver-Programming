#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512

#define NO_OF_DEVICES 4

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnhLN");
MODULE_DESCRIPTION("");

/*pseudo device's memmory */
char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];

struct pcdev_private_data
{
	char *buffer; // Pointer to device buffer
	unsigned size; // Size of the device buffer
	const char *serial_number; // Serial number of the device
	int perm;
	struct cdev cdev; // Character device structure
};

struct pcdrv_private_data {
	int total_devices;
	/*This holds the device number*/
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

struct pcdrv_private_data pcdrv_data = 
{
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		[0] = {
			.buffer = device_buffer_pcdev1,
			.size = MEM_SIZE_MAX_PCDEV1,
			.serial_number = "PCDEV1",
			.perm = 0x1, /*RDONLY*/
		},
		[1] = {
			.buffer = device_buffer_pcdev2,
			.size = MEM_SIZE_MAX_PCDEV2,
			.serial_number = "PCDEV2",
			.perm = 0x10, /*WRONLY*/
		},
		[2] = {
			.buffer = device_buffer_pcdev3,
			.size = MEM_SIZE_MAX_PCDEV3,
			.serial_number = "PCDEV3",
			.perm = 0x11 /*RDWR*/,
		},
		[3] = {
			.buffer = device_buffer_pcdev4,
			.size = MEM_SIZE_MAX_PCDEV4,
			.serial_number = "PCDEV4",
			.perm = 0x11, /*RDWR*/
		}
	}
};


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
    int i;
    
    /* 1. Allocate device numbers */
    ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pseudo_device");
    if (ret < 0) {
        printk(KERN_ALERT "Failed to allocate device number\n");
        return ret;
    }

    /* 2. Create device class */
    pcdrv_data.class_pcd = class_create("pcd_class");
    if (IS_ERR(pcdrv_data.class_pcd)) {
        printk(KERN_ALERT "Failed to create class\n");
        ret = PTR_ERR(pcdrv_data.class_pcd);
        goto unreg_chrdev;
    }

    /* 3. Create devices in loop */
    for (i = 0; i < NO_OF_DEVICES; i++) {
        printk(KERN_INFO "%s: Device number <major>:<minor> %d:%d\n", 
               __func__, MAJOR(pcdrv_data.device_number + i), MINOR(pcdrv_data.device_number + i));
        
        /* Initialize cdev */
        cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
        pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
        
        /* Add cdev */
        ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number + i, 1);
        if (ret < 0) {
            printk(KERN_ALERT "Failed to add cdev for device %d\n", i);
            goto cdev_del;
        }
        
        /* Create device */
        pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, 
                                            pcdrv_data.device_number + i, NULL, "pcd%d", i);
        if (IS_ERR(pcdrv_data.device_pcd)) {
            printk(KERN_ALERT "Failed to create device %d\n", i);
            ret = PTR_ERR(pcdrv_data.device_pcd);
            cdev_del(&pcdrv_data.pcdev_data[i].cdev);
            goto cdev_del;
        }
    }

    printk(KERN_INFO "Pseudo device driver initialized with %d devices\n", NO_OF_DEVICES);
    return 0;

cdev_del:
    for (i = i - 1; i >= 0; i--) {
        device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number + i);
        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }
    class_destroy(pcdrv_data.class_pcd);
unreg_chrdev:
    unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
    return ret;
}

static void __exit pcd_driver_exit(void)
{
    int i;
    
    /* Cleanup all devices */
    for (i = 0; i < NO_OF_DEVICES; i++) {
        device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number + i);
        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }
    
    class_destroy(pcdrv_data.class_pcd);
    unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
    
    printk(KERN_INFO "Pseudo device driver exited\n");
}

ssize_t pcd_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    struct pcdev_private_data *pcdev_data;
    int minor_no;
    
    /* Get device data from inode */
    minor_no = MINOR(filp->f_inode->i_rdev);
    
    /* Validate minor number */
    if (minor_no >= NO_OF_DEVICES) {
        printk(KERN_ALERT "Invalid device number: %d\n", minor_no);
        return -ENODEV;
    }
    
    pcdev_data = &pcdrv_data.pcdev_data[minor_no];
    
    printk(KERN_INFO "read requested on device pcd%d\n", minor_no);
    printk(KERN_INFO "current file offset: %lld\n", *off);
    
    /* Check permissions */
    if (!(pcdev_data->perm & 0x1)) {
        printk(KERN_ALERT "Device pcd%d: Read permission denied\n", minor_no);
        return -EPERM;
    }
    
    /* Check EOF */
    if (*off >= pcdev_data->size) {
        printk(KERN_INFO "EOF reached\n");
        return 0;
    }
    
    /* Adjust length */
    if ((*off + len) > pcdev_data->size) {
        len = pcdev_data->size - *off;
    }
    
    /* Copy to user */
    if (copy_to_user(buf, pcdev_data->buffer + *off, len)) {
        printk(KERN_ALERT "Failed to copy data to user\n");
        return -EFAULT;
    }
    
    /* Update offset */
    *off += len;
    printk(KERN_INFO "read completed, bytes read: %zu\n", len);
    
    return len;
}

ssize_t pcd_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    struct pcdev_private_data *pcdev_data;
    int minor_no;
    
    /* Get device data */
    minor_no = MINOR(filp->f_inode->i_rdev);
    
    /* Validate minor number */
    if (minor_no >= NO_OF_DEVICES) {
        printk(KERN_ALERT "Invalid device number: %d\n", minor_no);
        return -ENODEV;
    }
    
    pcdev_data = &pcdrv_data.pcdev_data[minor_no];
    
    printk(KERN_INFO "write requested on device pcd%d\n", minor_no);
    printk(KERN_INFO "current file offset: %lld\n", *off);
    
    /* Check permissions */
    if (!(pcdev_data->perm & 0x10)) {
        printk(KERN_ALERT "Device pcd%d: Write permission denied\n", minor_no);
        return -EPERM;
    }
    
    /* Adjust length */
    if ((*off + len) > pcdev_data->size) {
        len = pcdev_data->size - *off;
    }
    
    if (len <= 0) {
        printk(KERN_ALERT "No space left to write\n");
        return -ENOMEM;
    }
    
    /* Copy from user */
    if (copy_from_user(pcdev_data->buffer + *off, buf, len)) {
        printk(KERN_ALERT "Failed to copy data from user\n");
        return -EFAULT;
    }
    
    /* Update offset */
    *off += len;
    printk(KERN_INFO "write completed, bytes written: %zu\n", len);
    
    return len;
}

int pcd_open(struct inode *inode, struct file *filp)
{
    int minor_no = MINOR(inode->i_rdev);
    struct pcdev_private_data *pcdev_data;
    
    /* Validate minor number */
    if (minor_no >= NO_OF_DEVICES) {
        printk(KERN_ALERT "Invalid device number: %d\n", minor_no);
        return -ENODEV;
    }
    
    pcdev_data = &pcdrv_data.pcdev_data[minor_no];
    
    /* Store device data in private_data for easy access */
    filp->private_data = pcdev_data;
    
    printk(KERN_INFO "Device pcd%d opened (serial: %s, size: %u, perm: 0x%x)\n", 
           minor_no, pcdev_data->serial_number, pcdev_data->size, pcdev_data->perm);
    
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
    int minor_no = MINOR(inode->i_rdev);
    
    printk(KERN_INFO "Device pcd%d closed\n", minor_no);
    return 0;
}

loff_t pcd_lseek(struct file *filp, loff_t off, int whence)
{
    struct pcdev_private_data *pcdev_data = filp->private_data;
    loff_t temp;
    
    printk(KERN_INFO "lseek requested\n");
    printk(KERN_INFO "current file offset: %lld\n", filp->f_pos);
    
    switch (whence) {
        case SEEK_SET:
            if (off < 0 || off > pcdev_data->size) {
                return -EINVAL;
            }
            filp->f_pos = off;
            break;
        case SEEK_CUR:
            temp = filp->f_pos + off;
            if (temp < 0 || temp > pcdev_data->size) {
                return -EINVAL;
            }
            filp->f_pos = temp;
            break;
        case SEEK_END:
            temp = pcdev_data->size + off;
            if (temp < 0 || temp > pcdev_data->size) {
                return -EINVAL;
            }
            filp->f_pos = temp;
            break;
        default:
            return -EINVAL;
    }
    
    printk(KERN_INFO "lseek completed, new file offset: %lld\n", filp->f_pos);
    return filp->f_pos;
}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);
