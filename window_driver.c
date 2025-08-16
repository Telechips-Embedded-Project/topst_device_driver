// drivers/mytopst/window_driver.c
// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/mutex.h>

#define DEVICE_NAME "window_dev"
#define CLASS_NAME  "window_class"


#define WINDOW_MAGIC        'M'
#define WINDOW_SET_STATE    _IOW(WINDOW_MAGIC, 0, int)  /* 0:stop, 1:up(open), 2:down(close) */
#define WINDOW_GET_STATE    _IOR(WINDOW_MAGIC, 1, int)

struct window_priv {
	struct device       *dev;
	struct gpio_desc    *in1;
	struct gpio_desc    *in2;
	struct gpio_desc    *limit_lower;  /* NO 스위치: 눌림=LOW */
	struct gpio_desc    *limit_upper;  /* NO 스위치: 눌림=LOW */
	struct task_struct  *thread;
	int                  current_level; /* 0/1/2 */
	struct mutex         lock;
};


static int            major;
static struct class  *window_class;
static struct device *window_device;
static struct window_priv *g_priv; 

/* ===== PWM/모터 제어 쓰레드  ===== */
static int pwm_thread_fn(void *arg)
{
	struct window_priv *priv = (struct window_priv *)arg;

	while (!kthread_should_stop()) {
		int lvl;

		/* 현재 모드 스냅샷 */
		mutex_lock(&priv->lock);
		lvl = priv->current_level;
		mutex_unlock(&priv->lock);

		/* 리미트 스위치 체크 (눌림=0) */
		if (lvl == 1 /* up */) {
			int up_pressed = 1; /* default: not pressed (HIGH) */
			if (priv->limit_upper)
				up_pressed = gpiod_get_value_cansleep(priv->limit_upper);
			if (up_pressed == 0) {
				mutex_lock(&priv->lock);
				priv->current_level = 0; /* stop */
				mutex_unlock(&priv->lock);
				dev_info(priv->dev, "[window_dev] upper limit triggered, motor stop\n");
				lvl = 0;
			}
		} else if (lvl == 2 /* down */) {
			int low_pressed = 1;
			if (priv->limit_lower)
				low_pressed = gpiod_get_value_cansleep(priv->limit_lower);
			if (low_pressed == 0) {
				mutex_lock(&priv->lock);
				priv->current_level = 0;
				mutex_unlock(&priv->lock);
				dev_info(priv->dev, "[window_dev] lower limit triggered, motor stop\n");
				lvl = 0;
			}
		}

		/* H-Bridge 구동 (IN1/IN2) */
		switch (lvl) {
		case 0: /* stop */
			gpiod_set_value_cansleep(priv->in1, 0);
			gpiod_set_value_cansleep(priv->in2, 0);
			break;
		case 1: /* up(open) */
			gpiod_set_value_cansleep(priv->in1, 1);
			gpiod_set_value_cansleep(priv->in2, 0);
			break;
		case 2: /* down(close) */
			gpiod_set_value_cansleep(priv->in1, 0);
			gpiod_set_value_cansleep(priv->in2, 1);
			break;
		default:
			gpiod_set_value_cansleep(priv->in1, 0);
			gpiod_set_value_cansleep(priv->in2, 0);
			break;
		}

		msleep(10);
	}
	return 0;
}

/* ===== 파일 연산/IOCTL ===== */
static long window_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int level;

	if (!g_priv)
		return -ENODEV;

	switch (cmd) {
	case WINDOW_SET_STATE:
		if (copy_from_user(&level, (int __user *)arg, sizeof(int)))
			return -EFAULT;
		if (level < 0 || level > 2)
			return -EINVAL;
		mutex_lock(&g_priv->lock);
		g_priv->current_level = level;
		mutex_unlock(&g_priv->lock);
		dev_info(g_priv->dev, "[window_dev] level changed to %d\n", level);
		break;

	case WINDOW_GET_STATE:
		mutex_lock(&g_priv->lock);
		level = g_priv->current_level;
		mutex_unlock(&g_priv->lock);
		if (copy_to_user((int __user *)arg, &level, sizeof(int)))
			return -EFAULT;
		dev_info(g_priv->dev, "[window_dev] GET_STATE: %d\n", level);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int window_open(struct inode *inode, struct file *file)
{
	pr_info("[window_dev] device opened\n");
	return 0;
}

static int window_release(struct inode *inode, struct file *file)
{
	pr_info("[window_dev] device closed\n");
	return 0;
}

static const struct file_operations window_fops = {
	.owner          = THIS_MODULE,
	.open           = window_open,
	.release        = window_release,
	.unlocked_ioctl = window_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = window_ioctl,
#endif
};

static int window_probe(struct platform_device *pdev)
{
	int ret;
	struct window_priv *priv;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	mutex_init(&priv->lock);
	priv->current_level = 0;

	/* DT에서 GPIO 가져오기: in1-gpios, in2-gpios, limit-lower-gpios, limit-upper-gpios */
	priv->in1 = devm_gpiod_get(&pdev->dev, "in1", GPIOD_OUT_LOW);
	if (IS_ERR(priv->in1)) {
		dev_err(&pdev->dev, "failed to get in1-gpios\n");
		return PTR_ERR(priv->in1);
	}
	priv->in2 = devm_gpiod_get(&pdev->dev, "in2", GPIOD_OUT_LOW);
	if (IS_ERR(priv->in2)) {
		dev_err(&pdev->dev, "failed to get in2-gpios\n");
		return PTR_ERR(priv->in2);
	}
	/* limit 스위치 */
	priv->limit_lower = devm_gpiod_get_optional(&pdev->dev, "limit-lower", GPIOD_IN);
	if (IS_ERR(priv->limit_lower))
		return PTR_ERR(priv->limit_lower);
	priv->limit_upper = devm_gpiod_get_optional(&pdev->dev, "limit-upper", GPIOD_IN);
	if (IS_ERR(priv->limit_upper))
		return PTR_ERR(priv->limit_upper);

	/* character device 등록  */
	if (major == 0) {
		major = register_chrdev(0, DEVICE_NAME, &window_fops);
		if (major < 0) {
			dev_err(&pdev->dev, "register_chrdev failed: %d\n", major);
			return major;
		}
	}
	if (!window_class) {
		window_class = class_create(THIS_MODULE, CLASS_NAME);
		if (IS_ERR(window_class)) {
			ret = PTR_ERR(window_class);
			window_class = NULL;
			unregister_chrdev(major, DEVICE_NAME);
			major = 0;
			return ret;
		}
	}
	window_device = device_create(window_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(window_device)) {
		ret = PTR_ERR(window_device);
		window_device = NULL;
		class_destroy(window_class);
		window_class = NULL;
		unregister_chrdev(major, DEVICE_NAME);
		major = 0;
		return ret;
	}

	
	priv->thread = kthread_run(pwm_thread_fn, priv, "window_dev_thread");
	if (IS_ERR(priv->thread)) {
		ret = PTR_ERR(priv->thread);
		device_destroy(window_class, MKDEV(major, 0));
		window_device = NULL;
		class_destroy(window_class);
		window_class = NULL;
		unregister_chrdev(major, DEVICE_NAME);
		major = 0;
		return ret;
	}

	platform_set_drvdata(pdev, priv);
	g_priv = priv; 

	dev_info(&pdev->dev, "window driver probed, /dev/%s (major=%d)\n", DEVICE_NAME, major);
	return 0;
}

static int window_remove(struct platform_device *pdev)
{
	struct window_priv *priv = platform_get_drvdata(pdev);

	if (priv && priv->thread)
		kthread_stop(priv->thread);

	/* 안전 정지 */
	if (priv) {
		gpiod_set_value_cansleep(priv->in1, 0);
		gpiod_set_value_cansleep(priv->in2, 0);
	}

	if (window_device) {
		device_destroy(window_class, MKDEV(major, 0));
		window_device = NULL;
	}
	if (window_class) {
		class_destroy(window_class);
		window_class = NULL;
	}
	if (major > 0) {
		unregister_chrdev(major, DEVICE_NAME);
		major = 0;
	}

	g_priv = NULL;
	dev_info(&pdev->dev, "window driver removed\n");
	return 0;
}

/* DT 매칭: telechips,window-hbridge */
static const struct of_device_id window_of_match[] = {
	{ .compatible = "telechips,window-hbridge" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, window_of_match);

static struct platform_driver window_platdrv = {
	.probe  = window_probe,
	.remove = window_remove,
	.driver = {
		.name           = "telechips-window",
		.of_match_table = window_of_match,
	},
};

module_platform_driver(window_platdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("telli");
MODULE_DESCRIPTION("Window DC motor direction driver with limit switch auto-stop (platform + DT)");
