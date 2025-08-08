#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "platform.h"

/*Device private data struct*/
struct pcdev_private_data
{
	struct pcdev_platform_data pdata; /* Platform data */
	char *buffer;                     /* Pointer to device buffer */
	dev_t dev_num;                    /* Device number */
	struct cdev cdev;                 /* Character device structure */
	struct device *device_pcd;        /* Device structure */
};

/*Driver private data structure*/
struct pcdrv_private_data
{
	int total_devices;         /* Total number of devices */
	dev_t device_num_base;     /* Base device number */
	struct class *class_pcd;   /* Device class */
	struct device *device_pcd; /* Device structure */
};

struct pcdrv_private_data pcdrv_data;

/* File operations functions */
static int pcd_open(struct inode *inode, struct file *filp) { return 0; }

static int pcd_release(struct inode *inode, struct file *filp) { return 0; }

static ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos) { return 0; }

static ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos) { return count; }

/* File operations structure */
struct file_operations pcd_fops = {
    .owner = THIS_MODULE,
    .open = pcd_open,
    .release = pcd_release,
    .read = pcd_read,
    .write = pcd_write,
};

/*gets call when matched platform device is found*/
static int pcd_platform_driver_probe(struct platform_device *pdev)
{
	printk(KERN_INFO "A device is detected\n");
	int ret = 0;
	struct pcdev_private_data *dev_data;
	struct pcdev_platform_data *pdata;
	/*1. Get the platform data*/
	pdata = (struct pcdev_platform_data *)dev_get_platdata(&pdev->dev);
	if (!pdata)
	{
		printk(KERN_ERR "No platform data found\n");
		ret = -EINVAL;
		goto out;
	}
	/*2. Dynamically allocate memory for the device private data*/
	dev_data = kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL);
	if (!dev_data)
	{
		printk(KERN_ERR "Failed to allocate memory for device data\n");
		ret = -ENOMEM;
		goto out;
	}

	/*Save the device private data pointer in platform device structure*/
	dev_set_drvdata(&pdev->dev, dev_data);

	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	printk(KERN_INFO "Platform device %s with serial number %s probed\n", pdata->serial_number, pdata->serial_number);

	/*3. Dynamically allocate memory for the device buffer using size information from the platform data*/
	dev_data->buffer = kzalloc(dev_data->pdata.size, GFP_KERNEL);
	if (!dev_data->buffer)
	{
		printk(KERN_ERR "Failed to allocate memory for device buffer\n");
		ret = -ENOMEM;
		goto dev_data_free;
	}
	/*4. Get the device number*/
	dev_data->dev_num = pcdrv_data.device_num_base + pdev->id;
	/*5. Do cdev init and cdev add*/
	cdev_init(&dev_data->cdev, &pcd_fops);
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "Failed to add cdev for device %d\n", pdev->id);
		goto buffer_free;
	}
	/*6. Create device file for the detected platform device =*/
	dev_data->device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "pcdev-%d", pdev->id);
	if (IS_ERR(dev_data->device_pcd))
	{
		printk(KERN_ERR "Failed to create device %d\n", pdev->id);
		ret = PTR_ERR(dev_data->device_pcd);
		cdev_del(&dev_data->cdev);
		goto cdev_del;
	}
	pcdrv_data.total_devices++;
	printk(KERN_INFO "Device %s created with size %d and permissions 0x%x\n", pdata->serial_number, dev_data->pdata.size, dev_data->pdata.perm);
	return 0;
	/*7. Error handling*/
cdev_del:
	cdev_del(&dev_data->cdev);
buffer_free:
	kfree(dev_data->buffer);
dev_data_free:
	kfree(dev_data);
out:
	printk(KERN_ERR "Device probe failed\n");
	return ret;
}

/*gets called when platform device is removed*/
static void pcd_platform_driver_remove(struct platform_device *pdev)
{
	struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
	/*1. Remove a device that was created with device_create()*/
	device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
	/*2. Remove a cdev entry from the system*/
	cdev_del(&dev_data->cdev);
	/*3. Free the menory held by device*/
	kfree(dev_data->buffer);
	kfree(dev_data);

	pcdrv_data.total_devices--;
	/*4. Log the removal*/
	printk(KERN_INFO "Platform driver remove called\n");
}

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .driver =
        {
            .name = "pseudo-char_device",
        },
};

#define MAX_DEVICES 10

static int __init pcd_platform_driver_init(void)
{
	/*1. Dynamiccally allocate a device number for MAX_DEVICES*/
	int ret;
	ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES, "pcdevs");
	if (ret < 0)
	{
		printk(KERN_ERR "Failed to allocate device number\n");
		return ret;
	}

	/*2. Create device class under /sys/class*/
	pcdrv_data.class_pcd = class_create("pcd_class");
	if (IS_ERR(pcdrv_data.class_pcd))
	{
		unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
		printk(KERN_ERR "Failed to create device class\n");
		return PTR_ERR(pcdrv_data.class_pcd);
	}
	/*3. Register a platform driver*/
	platform_driver_register(&pcd_platform_driver);
	printk(KERN_INFO "Platform driver registered\n");
	return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
	/*1.Unregister the platform driver*/
	platform_driver_unregister(&pcd_platform_driver);
	/*2. Class destroy*/
	class_destroy(pcdrv_data.class_pcd);
	/*3. Unregister device number*/
	unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
	printk(KERN_INFO "Platform driver unregistered\n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnhLN");
MODULE_DESCRIPTION("Platform driver for pseudo character device");
