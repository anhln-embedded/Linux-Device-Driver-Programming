#include <linux/module.h>
#include <linux/platform_device.h>

#include "platform.h"

void pcdev_release(struct device *dev)
{
	printk(KERN_INFO "Platform device released\n");
}

// 1. Create 2 platform data
struct pcdev_platform_data pcdev_pdata[2] = {
	[0] = {
		.size = 512,
		.perm = RDWR, /* RDONLY */
		.serial_number = "PCDEV1",
	},
	[1] = {
		.size = 1024,
		.perm = RDWR, /* WRONLY */
		.serial_number = "PCDEV2",
	}
};

// 2. Create 2 platform devices
struct platform_device platform_pcdev_1 = {
	.name = "pseudo-char_device",
	.id = 0,
	.dev = {
		.platform_data = &pcdev_pdata[0],
		.release = pcdev_release,
	},
};

struct platform_device platform_pcdev_2 = {
	.name = "pseudo-char_device",
	.id = 1,
	.dev = {
		.platform_data = &pcdev_pdata[1],
		.release = pcdev_release,
	},
};

static int __init pcdev_platform_init(void)
{
	platform_device_register(&platform_pcdev_1);
	platform_device_register(&platform_pcdev_2);
	printk(KERN_INFO "Platform devices registered\n");
	return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev_1);
	platform_device_unregister(&platform_pcdev_2);
	printk(KERN_INFO "Platform devices unregistered\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnhLN");
MODULE_DESCRIPTION("Module for platform device registration");
