#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define DEVICE_NAME "window_dev"
#define WINDOW_MAGIC 'M'
#define WINDOW_SET_STATE _IOW(WINDOW_MAGIC, 0, int)
#define WINDOW_GET_STATE _IOR(WINDOW_MAGIC, 1, int)
// BCM 핀 번호
#define IN1_GPIO         (84)
#define IN2_GPIO         (85)
#define LIMIT_LOWER_GPIO (90)   // 하단 리미트 스위치 (NO)
#define LIMIT_UPPER_GPIO (65)   // 상단 리미트 스위치 (NO)

static int major;
static struct class *window_class = NULL;
static struct device *window_device = NULL;

static int current_level = 0;
static struct task_struct *pwm_task;

// ====== 데몬(쓰레드) 함수 ======
static int pwm_thread_fn(void *arg)
{
    while (!kthread_should_stop())
    {
        // UP 동작 중: 상단 리미트 스위치 눌리면 멈춤
        if (current_level == 1) {
            if (gpio_get_value(LIMIT_UPPER_GPIO) == 0) { // NO: 눌림=LOW
                current_level = 0;
                printk(KERN_INFO "[window_dev] upper limit triggered, motor stop\n");
            }
        }
        // DOWN 동작 중: 하단 리미트 스위치 눌리면 멈춤
        else if (current_level == 2) {
            if (gpio_get_value(LIMIT_LOWER_GPIO) == 0) { // NO: 눌림=LOW
                current_level = 0;
                printk(KERN_INFO "[window_dev] lower limit triggered, motor stop\n");
            }
        }

        switch(current_level)
        {
            case 0: // stop
                gpio_set_value(IN1_GPIO, 0);
                gpio_set_value(IN2_GPIO, 0);
                break;
            case 1: // up(open)
                gpio_set_value(IN1_GPIO, 1);
                gpio_set_value(IN2_GPIO, 0);
                break;
            case 2: // down(close)
                gpio_set_value(IN1_GPIO, 0);
                gpio_set_value(IN2_GPIO, 1);
                break;
            default:
                gpio_set_value(IN1_GPIO, 0);
                gpio_set_value(IN2_GPIO, 0);
                break;
        }
        msleep(10);
    }
    return 0;
}

// ====== ioctl 함수 ======
static long window_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int level;

    switch(cmd)
    {
        case WINDOW_SET_STATE:
            if (copy_from_user(&level, (int __user *)arg, sizeof(int)))
                return -EFAULT;

            if (level < 0 || level > 2)
                return -EINVAL;

            current_level = level;
            printk(KERN_INFO "[window_dev] level changed to %d\n", level);
            break;
	case WINDOW_GET_STATE:
	    level = current_level;
	    if (copy_to_user((int __user *)arg, &level, sizeof(int)))
                return -EFAULT;
            printk(KERN_INFO "[window_dev] GET_STATE: %d\n", level);
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

// ====== 파일 연산 함수 ======
static int window_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[window_dev] device opened\n");
    return 0;
}

static int window_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[window_dev] device closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = window_open,
    .release = window_release,
    .unlocked_ioctl = window_ioctl,
};

// ====== 커널 모듈 초기화/해제 ======
static int __init window_init(void)
{
    printk(KERN_INFO "[window_dev] init\n");

    if (!gpio_is_valid(IN1_GPIO) || !gpio_is_valid(IN2_GPIO) ||
        !gpio_is_valid(LIMIT_LOWER_GPIO) || !gpio_is_valid(LIMIT_UPPER_GPIO)) {
        printk(KERN_ERR "[window_dev] invalid GPIO\n");
        return -ENODEV;
    }

    gpio_request(IN1_GPIO, "IN1");
    gpio_request(IN2_GPIO, "IN2");
    gpio_request(LIMIT_LOWER_GPIO, "LIMIT_LOWER");
    gpio_request(LIMIT_UPPER_GPIO, "LIMIT_UPPER");

    gpio_direction_output(IN1_GPIO, 0);
    gpio_direction_output(IN2_GPIO, 0);
    gpio_direction_input(LIMIT_LOWER_GPIO);
    gpio_direction_input(LIMIT_UPPER_GPIO);

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        gpio_free(IN1_GPIO);
        gpio_free(IN2_GPIO);
        gpio_free(LIMIT_LOWER_GPIO);
        gpio_free(LIMIT_UPPER_GPIO);
        return major;
    }

    window_class = class_create(THIS_MODULE, "window_class");
    if (IS_ERR(window_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        gpio_free(IN1_GPIO);
        gpio_free(IN2_GPIO);
        gpio_free(LIMIT_LOWER_GPIO);
        gpio_free(LIMIT_UPPER_GPIO);
        return PTR_ERR(window_class);
    }

    window_device = device_create(window_class, NULL, MKDEV(major,0), NULL, DEVICE_NAME);

    pwm_task = kthread_run(pwm_thread_fn, NULL, "window_dev_thread");
    if (IS_ERR(pwm_task)) {
        device_destroy(window_class, MKDEV(major,0));
        class_destroy(window_class);
        unregister_chrdev(major, DEVICE_NAME);
        gpio_free(IN1_GPIO);
        gpio_free(IN2_GPIO);
        gpio_free(LIMIT_LOWER_GPIO);
        gpio_free(LIMIT_UPPER_GPIO);
        return PTR_ERR(pwm_task);
    }

    printk(KERN_INFO "[window_dev] loaded /dev/%s major=%d\n", DEVICE_NAME, major);
    return 0;
}

static void __exit window_exit(void)
{
    kthread_stop(pwm_task);

    device_destroy(window_class, MKDEV(major,0));
    class_destroy(window_class);
    unregister_chrdev(major, DEVICE_NAME);

    gpio_set_value(IN1_GPIO, 0);
    gpio_set_value(IN2_GPIO, 0);
    gpio_free(IN1_GPIO);
    gpio_free(IN2_GPIO);
    gpio_free(LIMIT_LOWER_GPIO);
    gpio_free(LIMIT_UPPER_GPIO);

    printk(KERN_INFO "[window_dev] unloaded\n");
}

module_init(window_init);
module_exit(window_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Window DC motor direction driver with limit switch auto-stop (NO type)");
