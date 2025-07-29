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

#define PWM_PERIOD_NS     20000000  // 20ms
#define PWM_DUTY_FAST_NS  1500000   // 1.5ms
#define PWM_DUTY_SLOW_NS  800000    // 0.8ms

static struct pwm_device *wiper_pwm;
static int wiper_mode = WIPER_MODE_OFF;

static void apply_pwm_state(struct pwm_device *pwm, u32 duty_ns, u32 period_ns, bool enable)
{
    struct pwm_state state;
    pwm_get_state(pwm, &state);
    state.period = period_ns;
    state.duty_cycle = duty_ns;
    state.enabled = enable;
    pwm_apply_state(pwm, &state);
}

static long wiper_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int user_val;

    switch (cmd) {
        case WIPER_SET_MODE:
            if (copy_from_user(&user_val, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            if (user_val < WIPER_MODE_OFF || user_val > WIPER_MODE_SLOW)
                return -EINVAL;

            switch (user_val) {
                case WIPER_MODE_FAST:
                    apply_pwm_state(wiper_pwm, PWM_DUTY_FAST_NS, PWM_PERIOD_NS, true);
                    break;
                case WIPER_MODE_SLOW:
                    apply_pwm_state(wiper_pwm, PWM_DUTY_SLOW_NS, PWM_PERIOD_NS, true);
                    break;
                case WIPER_MODE_OFF:
                default:
                    apply_pwm_state(wiper_pwm, 0, PWM_PERIOD_NS, false);
                    break;
            }
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
    struct device *dev = &pdev->dev;

    wiper_pwm = devm_pwm_get(dev, "wiper_pwm");
    if (IS_ERR(wiper_pwm)) {
        dev_err(dev, "failed to get pwm device\n");
        return PTR_ERR(wiper_pwm);
    }

    dev_info(dev, "wiper driver probed successfully\n");
    return misc_register(&wiper_miscdev);
}

static int wiper_remove(struct platform_device *pdev)
{
    if (wiper_pwm)
        pwm_disable(wiper_pwm);
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
MODULE_DESCRIPTION("TOPST D3-G Wiper Control Driver with PWM (HW)");
