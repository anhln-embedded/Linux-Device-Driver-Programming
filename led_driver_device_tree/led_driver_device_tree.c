#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnhLN");
MODULE_DESCRIPTION("LED Driver using Device Tree");

struct led_drvdata {
    struct gpio_desc *gpiod;
    struct miscdevice miscdev;
    struct mutex lock;
};

static int led_open(struct inode *inode, struct file *filp)
{
    struct miscdevice *mdev = filp->private_data;
    struct led_drvdata *drvdata = container_of(mdev, struct led_drvdata, miscdev);
    filp->private_data = drvdata;
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
    struct led_drvdata *drvdata = filp->private_data;
    char ch;

    if (len < 1)
        return -EINVAL;

    if (copy_from_user(&ch, buf, 1))
        return -EFAULT;

    mutex_lock(&drvdata->lock);
    if (ch == '1') {
        gpiod_set_value_cansleep(drvdata->gpiod, 1);
    } else if (ch == '0') {
        gpiod_set_value_cansleep(drvdata->gpiod, 0);
    } else {
        mutex_unlock(&drvdata->lock);
        return -EINVAL;
    }
    mutex_unlock(&drvdata->lock);

    return len;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
};

static int led_probe(struct platform_device *pdev)
{
    struct led_drvdata *drvdata;
    int ret;

    drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
    if (!drvdata)
        return -ENOMEM;

    drvdata->gpiod = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(drvdata->gpiod)) {
        dev_err(&pdev->dev, "Failed to get GPIO from Device Tree\n");
        return PTR_ERR(drvdata->gpiod);
    }

    mutex_init(&drvdata->lock);

    drvdata->miscdev.minor = MISC_DYNAMIC_MINOR;
    drvdata->miscdev.name = "led_dt";
    drvdata->miscdev.fops = &led_fops;

    ret = misc_register(&drvdata->miscdev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register misc device\n");
        return ret;
    }

    platform_set_drvdata(pdev, drvdata);
    dev_info(&pdev->dev, "LED driver probed successfully\n");
    return 0;
}

static void led_remove(struct platform_device *pdev)
{
    struct led_drvdata *drvdata = platform_get_drvdata(pdev);

    misc_deregister(&drvdata->miscdev);
    dev_info(&pdev->dev, "LED driver removed\n");
}

static const struct of_device_id led_dt_ids[] = {
    { .compatible = "anhln,bbb-led" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, led_dt_ids);

static struct platform_driver led_driver = {
    .driver = {
        .name = "bbb-led",
        .of_match_table = led_dt_ids,
    },
    .probe = led_probe,
    .remove = led_remove,
};

module_platform_driver(led_driver);