// ambient_driver.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "ambient_dev"
#define CLASS_NAME "ambient_class"
#define AMBIENT_MAGIC 'L'

#define AMBIENT_SET_MODE        _IOW(AMBIENT_MAGIC, 1, char *)
#define AMBIENT_GET_MODE        _IOR(AMBIENT_MAGIC, 2, char *)
#define AMBIENT_SET_BRIGHTNESS  _IOW(AMBIENT_MAGIC, 3, int)
#define AMBIENT_GET_BRIGHTNESS  _IOR(AMBIENT_MAGIC, 4, int)

static int major;
static struct class* ambient_class = NULL;
static struct device* ambient_device = NULL;

static char current_mode[16] = "red";   // 초기 모드: red
static int current_brightness = 50;     // 초기 밝기: 50

static long ambient_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
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

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = ambient_ioctl,
};

static int __init ambient_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register a major number\n");
        return major;
    }

    ambient_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(ambient_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(ambient_class);
    }

    ambient_device = device_create(ambient_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(ambient_device)) {
        class_destroy(ambient_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(ambient_device);
    }

    printk(KERN_INFO "AMBIENT driver loaded.\n");
    return 0;
}

static void __exit ambient_exit(void) {
    device_destroy(ambient_class, MKDEV(major, 0));
    class_unregister(ambient_class);
    class_destroy(ambient_class);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "AMBIENT driver unloaded.\n");
}

module_init(ambient_init);
module_exit(ambient_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Ambient Light Driver using ioctl");
MODULE_VERSION("1.0");
