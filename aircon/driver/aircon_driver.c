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

#define AIRCON_MAGIC 'A'
#define AIRCON_SET_LEVEL _IOW(AIRCON_MAGIC, 1, int)
#define AIRCON_GET_LEVEL _IOR(AIRCON_MAGIC, 2, int)

#define AIRCON_LEVEL_OFF  0
#define AIRCON_LEVEL_LOW  1
#define AIRCON_LEVEL_MID  2
#define AIRCON_LEVEL_HIGH 3

#define PWM_PERIOD_NS       20000000  // 20ms
#define PWM_DUTY_LOW_NS      500000   // 2.5% duty
#define PWM_DUTY_MID_NS     1000000   // 5% duty
#define PWM_DUTY_HIGH_NS    1500000   // 7.5% duty

static struct pwm_device *aircon_pwm;
static int aircon_level = AIRCON_LEVEL_OFF;

static void apply_pwm_state(struct pwm_device *pwm, u32 duty_ns, u32 period_ns, bool enable)
{
    struct pwm_state state;
    pwm_get_state(pwm, &state);
    state.period = period_ns;
    state.duty_cycle = duty_ns;
    state.enabled = enable;
    pwm_apply_state(pwm, &state);
}

static long aircon_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int user_val;

    switch (cmd) {
        case AIRCON_SET_LEVEL:
            if (copy_from_user(&user_val, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            if (user_val < AIRCON_LEVEL_OFF || user_val > AIRCON_LEVEL_HIGH)
                return -EINVAL;

            switch (user_val) {
                case AIRCON_LEVEL_LOW:
                    apply_pwm_state(aircon_pwm, PWM_DUTY_LOW_NS, PWM_PERIOD_NS, true);
                    break;
                case AIRCON_LEVEL_MID:
                    apply_pwm_state(aircon_pwm, PWM_DUTY_MID_NS, PWM_PERIOD_NS, true);
                    break;
                case AIRCON_LEVEL_HIGH:
                    apply_pwm_state(aircon_pwm, PWM_DUTY_HIGH_NS, PWM_PERIOD_NS, true);
                    break;
                case AIRCON_LEVEL_OFF:
                default:
                    apply_pwm_state(aircon_pwm, 0, PWM_PERIOD_NS, false);
                    break;
            }
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

static int aircon_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;

    aircon_pwm = devm_pwm_get(dev, "aircon_pwm");
    if (IS_ERR(aircon_pwm)) {
        dev_err(dev, "failed to get pwm device\n");
        return PTR_ERR(aircon_pwm);
    }

    dev_info(dev, "aircon driver probed successfully\n");
    return misc_register(&aircon_miscdev);
}

static int aircon_remove(struct platform_device *pdev)
{
    if (aircon_pwm)
        pwm_disable(aircon_pwm);
    misc_deregister(&aircon_miscdev);
    return 0;
}

static const struct of_device_id aircon_of_match[] = {
    { .compatible = "telechips,aircon-pwm" },
    {},
};
MODULE_DEVICE_TABLE(of, aircon_of_match);

static struct platform_driver aircon_driver = {
    .probe = aircon_probe,
    .remove = aircon_remove,
    .driver = {
        .name = "aircon_pwm_driver",
        .of_match_table = aircon_of_match,
    },
};

module_platform_driver(aircon_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Telechips Embedded Project");
MODULE_DESCRIPTION("TOPST D3-G Aircon Control Driver with PWM (HW)");
