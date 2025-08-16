// drivers/mytopst/ambient_driver.c
// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "ambient_dev"
#define CLASS_NAME  "ambient_class"


#define AMBIENT_MAGIC 'L'
#define AMBIENT_SET_MODE        _IOW(AMBIENT_MAGIC, 1, char *)
#define AMBIENT_GET_MODE        _IOR(AMBIENT_MAGIC, 2, char *)
#define AMBIENT_SET_BRIGHTNESS  _IOW(AMBIENT_MAGIC, 3, int)
#define AMBIENT_GET_BRIGHTNESS  _IOR(AMBIENT_MAGIC, 4, int)


static int major;
static struct class  *ambient_class;
static struct device *ambient_device;

static char current_mode[16] = "red";  /* 초기 모드 */
static int  current_brightness = 50;   /* 초기 밝기 */


static long ambient_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case AMBIENT_SET_MODE:
        if (copy_from_user(current_mode, (char __user *)arg, sizeof(current_mode)))
            return -EFAULT;
        printk(KERN_INFO "AMBIENT: Set mode to %s\n", current_mode);
        break;

    case AMBIENT_GET_MODE:
        if (copy_to_user((char __user *)arg, current_mode, sizeof(current_mode)))
            return -EFAULT;
        break;

    case AMBIENT_SET_BRIGHTNESS:
        if (copy_from_user(&current_brightness, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        printk(KERN_INFO "AMBIENT: Set brightness to %d\n", current_brightness);
        break;

    case AMBIENT_GET_BRIGHTNESS:
        if (copy_to_user((int __user *)arg, &current_brightness, sizeof(int)))
            return -EFAULT;
        break;

    default:
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = ambient_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = ambient_ioctl,
#endif
};


static int ambient_probe(struct platform_device *pdev)
{
    int ret;

    /* character device 등록 */
    if (major == 0) {
        major = register_chrdev(0, DEVICE_NAME, &fops);
        if (major < 0) {
            dev_err(&pdev->dev, "Failed to register a major number\n");
            return major;
        }
    }

    /* 클래스 생성 (최초 1회) */
    if (!ambient_class) {
        ambient_class = class_create(THIS_MODULE, CLASS_NAME);
        if (IS_ERR(ambient_class)) {
            ret = PTR_ERR(ambient_class);
            ambient_class = NULL;
            unregister_chrdev(major, DEVICE_NAME);
            major = 0;
            return ret;
        }
    }

    /* 디바이스 노드 생성: /dev/ambient_dev (minor 0로 고정) */
    ambient_device = device_create(ambient_class, NULL, MKDEV(major, 0),
                                   NULL, DEVICE_NAME);
    if (IS_ERR(ambient_device)) {
        ret = PTR_ERR(ambient_device);
        ambient_device = NULL;
        class_unregister(ambient_class);
        class_destroy(ambient_class);
        ambient_class = NULL;
        unregister_chrdev(major, DEVICE_NAME);
        major = 0;
        return ret;
    }

    dev_info(&pdev->dev, "AMBIENT driver probed via DT.\n");
    return 0;
}

static int ambient_remove(struct platform_device *pdev)
{
    if (ambient_device) {
        device_destroy(ambient_class, MKDEV(major, 0));
        ambient_device = NULL;
    }
    if (ambient_class) {
        class_unregister(ambient_class);
        class_destroy(ambient_class);
        ambient_class = NULL;
    }
    if (major > 0) {
        unregister_chrdev(major, DEVICE_NAME);
        major = 0;
    }
    dev_info(&pdev->dev, "AMBIENT driver removed.\n");
    return 0;
}

/* DT 매칭: telechips,ambient */
static const struct of_device_id ambient_of_match[] = {
    { .compatible = "telechips,ambient" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ambient_of_match);

static struct platform_driver ambient_platdrv = {
    .probe  = ambient_probe,
    .remove = ambient_remove,
    .driver = {
        .name           = "telechips-ambient",
        .of_match_table = ambient_of_match,
    },
};

module_platform_driver(ambient_platdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("telli");
MODULE_DESCRIPTION("Ambient Light Driver using ioctl (platform driver + DT)");
MODULE_VERSION("1.0");