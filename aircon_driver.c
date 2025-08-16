// drivers/mytopst/aircon_driver.c
// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define AIRCON_MAGIC 'A'
#define AIRCON_SET_LEVEL _IOW(AIRCON_MAGIC, 1, int)
#define AIRCON_GET_LEVEL _IOR(AIRCON_MAGIC, 2, int)

#define AIRCON_LEVEL_OFF  0
#define AIRCON_LEVEL_LOW  1
#define AIRCON_LEVEL_MID  2
#define AIRCON_LEVEL_HIGH 3

static int aircon_level = AIRCON_LEVEL_OFF;

static long aircon_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int user_val;

    switch (cmd) {
    case AIRCON_SET_LEVEL:
        if (copy_from_user(&user_val, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        if (user_val < AIRCON_LEVEL_OFF || user_val > AIRCON_LEVEL_HIGH)
            return -EINVAL;
        aircon_level = user_val;
        break;

    case AIRCON_GET_LEVEL:
        if (copy_to_user((int __user *)arg, &aircon_level, sizeof(int)))
            return -EFAULT;
        break;

    default:
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations aircon_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = aircon_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = aircon_ioctl,
#endif
};

static struct miscdevice aircon_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "aircon_dev",
    .fops  = &aircon_fops,
    .mode  = 0666,
};

static int aircon_probe(struct platform_device *pdev)
{
    int ret;

    ret = misc_register(&aircon_miscdev);
    if (ret) {
        dev_err(&pdev->dev, "misc_register failed: %d\n", ret);
        return ret;
    }

    dev_info(&pdev->dev, "aircon driver probed (state-only), /dev/%s\n",
             aircon_miscdev.name);
    return 0;
}

static int aircon_remove(struct platform_device *pdev)
{
    misc_deregister(&aircon_miscdev);
    dev_info(&pdev->dev, "aircon driver removed\n");
    return 0;
}

static const struct of_device_id aircon_of_match[] = {
    { .compatible = "telechips,aircon-pwm" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, aircon_of_match);

static struct platform_driver aircon_platdrv = {
    .probe  = aircon_probe,
    .remove = aircon_remove,
    .driver = {
        .name           = "telechips-aircon",
        .of_match_table = aircon_of_match,
    },
};

module_platform_driver(aircon_platdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("telli");
MODULE_DESCRIPTION("TOPST D3-G Aircon State-Only Driver (platform + DT)");
