
// ambient_driver.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "ambient_dev"
#define AMBIENT_MAGIC 'L'

#define AMBIENT_SET_MODE           _IOW(AMBIENT_MAGIC, 1, char*)
#define AMBIENT_GET_MODE_STR       _IOR(AMBIENT_MAGIC, 2, char*)
#define AMBIENT_SET_BRIGHTNESS     _IOW(AMBIENT_MAGIC, 3, char *)
#define AMBIENT_GET_BRIGHTNESS_STR _IOR(AMBIENT_MAGIC, 4, char *)

#define MAX_MODE_LEN 32
#define MAX_BRIGHTNESS_LEN 16

static char current_mode[32] = "off";
static char brightness_str[16] = "mid";  // default: mid

static int major;
static struct class *cls;

static long ambient_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case AMBIENT_SET_MODE:
            if (copy_from_user(current_mode, (char *)arg, sizeof(current_mode)))
                return -EFAULT;
            printk(KERN_INFO "AMBIENT: Set mode to %s\n", current_mode);
            break;
        case AMBIENT_GET_MODE_STR:
	    if (copy_to_user((char __user *)arg, current_mode, strnlen(current_mode, MAX_MODE_LEN) + 1))
      		return -EFAULT;
            break;
        case AMBIENT_SET_BRIGHTNESS:
            if (copy_from_user(brightness_str, (char *)arg, sizeof(brightness_str)))
                return -EFAULT;
            printk(KERN_INFO "AMBIENT: Set brightness to %s\n", brightness_str);
            break;
        case AMBIENT_GET_BRIGHTNESS_STR:
            if (copy_to_user((char *)arg, brightness_str, strnlen(brightness_str, MAX_BRIGHTNESS_LEN) + 1))
                return -EFAULT;
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = ambient_ioctl,
    .owner = THIS_MODULE,
};

static int __init ambient_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    cls = class_create(THIS_MODULE, "ambient_class");
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    printk(KERN_INFO "AMBIENT driver loaded.\n");
    return 0;
}

static void __exit ambient_exit(void) {
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "AMBIENT driver unloaded.\n");
}

module_init(ambient_init);
module_exit(ambient_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Telechips Team");
MODULE_DESCRIPTION("Ambient LED Strip Control Driver with IOCTL");
