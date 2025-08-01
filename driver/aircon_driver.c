// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

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
    .owner = THIS_MODULE,
    .unlocked_ioctl = aircon_ioctl,
};

static struct miscdevice aircon_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "aircon_dev",
    .fops = &aircon_fops,
    .mode = 0666,
};

static int __init aircon_init(void)
{
    return misc_register(&aircon_miscdev);
}

static void __exit aircon_exit(void)
{
    misc_deregister(&aircon_miscdev);
}

module_init(aircon_init);
module_exit(aircon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Telechips Embedded Project");
MODULE_DESCRIPTION("TOPST D3-G Aircon State-Only Driver");
