#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "headlamp_dev"
#define HEADLAMP_MAGIC 'H'

#define HEADLAMP_GPIO (83)

// ioctl 명령 정의
#define HEADLAMP_SET_STATE _IOW(HEADLAMP_MAGIC, 0, int) // 값 쓰기 (0=off, 1=on)
#define HEADLAMP_GET_STATE _IOR(HEADLAMP_MAGIC, 1, int) // 값 읽기


static int major;
static struct class *cls;
static int headlamp_state = 0; // 0: off, 1: on

static long headlamp_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int value;

    switch (cmd) {
        case HEADLAMP_SET_STATE:
            if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            if (value == 0) {
                gpio_set_value(HEADLAMP_GPIO, 0);
                headlamp_state = 0;
                pr_info("[headlamp_driver] ioctl: HEADLAMP OFF\n");
            } else if (value == 1) {
                gpio_set_value(HEADLAMP_GPIO, 1);
                headlamp_state = 1;
                pr_info("[headlamp_driver] ioctl: HEADLAMP ON\n");
            } else {
                return -EINVAL;
            }
            break;

        case HEADLAMP_GET_STATE:
            value = gpio_get_value(HEADLAMP_GPIO);
            if (copy_to_user((int __user *)arg, &value, sizeof(int)))
                return -EFAULT;
            pr_info("[headlamp_driver] ioctl: HEADLAMP GET (%d)\n", value);
            break;

        default:
            pr_err("[headlamp_driver] ioctl: unknown cmd %u\n", cmd);
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = headlamp_ioctl,
};

static int __init headlamp_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    cls = class_create(THIS_MODULE, "headlamp_class");
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    gpio_request(HEADLAMP_GPIO, "headlamp_gpio");
    gpio_direction_output(HEADLAMP_GPIO, 0);
    return 0;
}

static void __exit headlamp_exit(void) {
    gpio_set_value(HEADLAMP_GPIO, 0);
    gpio_free(HEADLAMP_GPIO);
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
}

module_init(headlamp_init);
module_exit(headlamp_exit);
MODULE_LICENSE("GPL");
