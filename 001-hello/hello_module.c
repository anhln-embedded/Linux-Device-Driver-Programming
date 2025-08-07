#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnhLN");
MODULE_DESCRIPTION("Simple Hello World Kernel Module");

static int __init hello_init(void)
{
    printk(KERN_INFO "AnhLN Hello world!\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "AnhLN Goodbye world!\n");
}

module_init(hello_init);
module_exit(hello_exit);

