// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/pwm.h>

#define WIPER_MAGIC 'W'
#define WIPER_SET_MODE _IOW(WIPER_MAGIC, 1, int)
#define WIPER_GET_MODE _IOR(WIPER_MAGIC, 2, int)

#define WIPER_MODE_OFF  0
#define WIPER_MODE_FAST 1
#define WIPER_MODE_SLOW 2

static int wiper_mode = WIPER_MODE_OFF;

static long wiper_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int user_val;

    switch (cmd) {
        case WIPER_SET_MODE:
            if (copy_from_user(&user_val, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            if (user_val < WIPER_MODE_OFF || user_val > WIPER_MODE_SLOW)
                return -EINVAL;
            wiper_mode = user_val;
            break;

        case WIPER_GET_MODE:
            if (copy_to_user((int __user *)arg, &wiper_mode, sizeof(int)))
                return -EFAULT;
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

static const struct file_operations wiper_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = wiper_ioctl,
};

static struct miscdevice wiper_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "wiper_dev",
    .fops = &wiper_fops,
    .mode = 0666,
};

static int wiper_probe(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "wiper driver probed successfully\n");
    return misc_register(&wiper_miscdev);
}

static int wiper_remove(struct platform_device *pdev)
{
    misc_deregister(&wiper_miscdev);
    return 0;
}

static const struct of_device_id wiper_of_match[] = {
    { .compatible = "telechips,wiper-pwm" },
    {},
};
MODULE_DEVICE_TABLE(of, wiper_of_match);

static struct platform_driver wiper_driver = {
    .probe = wiper_probe,
    .remove = wiper_remove,
    .driver = {
        .name = "wiper_pwm_driver",
        .of_match_table = wiper_of_match,
    },
};

module_platform_driver(wiper_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Telechips Embedded Project");
MODULE_DESCRIPTION("TOPST D3-G Wiper Control Driver with Mode Storage via ioctl");
