// drivers/mytopst/headlamp_driver.c
// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>

#define DEVICE_NAME "headlamp_dev"
#define CLASS_NAME  "headlamp_class"

#define HEADLAMP_MAGIC        'H'
#define HEADLAMP_SET_STATE    _IOW(HEADLAMP_MAGIC, 0, int) /* 0:off, 1:on */
#define HEADLAMP_GET_STATE    _IOR(HEADLAMP_MAGIC, 1, int)

struct headlamp_priv {
	struct device    *dev;
	struct gpio_desc *lamp;        
	int               state;     
};

static int            major;
static struct class  *cls;
static struct device *devnode;
static struct headlamp_priv *g_priv;

static long headlamp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int val;

	if (!g_priv || !g_priv->lamp)
		return -ENODEV;

	switch (cmd) {
	case HEADLAMP_SET_STATE:
		if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
			return -EFAULT;
		if (val == 0) {
			gpiod_set_value_cansleep(g_priv->lamp, 0);
			g_priv->state = 0;
			pr_info("[headlamp_driver] ioctl: HEADLAMP OFF\n");
		} else if (val == 1) {
			gpiod_set_value_cansleep(g_priv->lamp, 1);
			g_priv->state = 1;
			pr_info("[headlamp_driver] ioctl: HEADLAMP ON\n");
		} else {
			return -EINVAL;
		}
		break;

	case HEADLAMP_GET_STATE:
		val = gpiod_get_value_cansleep(g_priv->lamp);
		if (copy_to_user((int __user *)arg, &val, sizeof(int)))
			return -EFAULT;
		pr_info("[headlamp_driver] ioctl: HEADLAMP GET (%d)\n", val);
		break;

	default:
		pr_err("[headlamp_driver] ioctl: unknown cmd %u\n", cmd);
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations fops = {
	.owner          = THIS_MODULE,      
	.unlocked_ioctl = headlamp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = headlamp_ioctl,
#endif
};

static int headlamp_probe(struct platform_device *pdev)
{
	int ret;
	struct headlamp_priv *priv;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev  = &pdev->dev;
	priv->lamp = devm_gpiod_get(&pdev->dev, "headlamp", GPIOD_OUT_LOW);
	if (IS_ERR(priv->lamp)) {
		dev_err(&pdev->dev, "failed to get headlamp-gpios\n");
		return PTR_ERR(priv->lamp);
	}
	priv->state = 0; /* 기본 OFF */

	/* character device 등록 */
	if (major == 0) {
		major = register_chrdev(0, DEVICE_NAME, &fops);
		if (major < 0) {
			dev_err(&pdev->dev, "register_chrdev failed: %d\n", major);
			return major;
		}
	}
	if (!cls) {
		cls = class_create(THIS_MODULE, CLASS_NAME);
		if (IS_ERR(cls)) {
			ret = PTR_ERR(cls);
			cls = NULL;
			unregister_chrdev(major, DEVICE_NAME);
			major = 0;
			return ret;
		}
	}
	devnode = device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(devnode)) {
		ret = PTR_ERR(devnode);
		devnode = NULL;
		class_destroy(cls);
		cls = NULL;
		unregister_chrdev(major, DEVICE_NAME);
		major = 0;
		return ret;
	}

	platform_set_drvdata(pdev, priv);
	g_priv = priv; 

	dev_info(&pdev->dev, "headlamp driver probed, /dev/%s (major=%d)\n",
	         DEVICE_NAME, major);
	return 0;
}

static int headlamp_remove(struct platform_device *pdev)
{
	struct headlamp_priv *priv = platform_get_drvdata(pdev);

	/* 안전하게 OFF */
	if (priv && priv->lamp)
		gpiod_set_value_cansleep(priv->lamp, 0);

	if (devnode) {
		device_destroy(cls, MKDEV(major, 0));
		devnode = NULL;
	}
	if (cls) {
		class_destroy(cls);
		cls = NULL;
	}
	if (major > 0) {
		unregister_chrdev(major, DEVICE_NAME);
		major = 0;
	}
	g_priv = NULL;

	dev_info(&pdev->dev, "headlamp driver removed\n");
	return 0;
}

/* DT 매칭 */
static const struct of_device_id headlamp_of_match[] = {
	{ .compatible = "telechips,headlamp" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, headlamp_of_match);

static struct platform_driver headlamp_platdrv = {
	.probe  = headlamp_probe,
	.remove = headlamp_remove,
	.driver = {
		.name           = "telechips-headlamp",
		.of_match_table = headlamp_of_match,
	},
};

module_platform_driver(headlamp_platdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JSY Project");
MODULE_DESCRIPTION("Headlamp driver (ioctl ABI kept, platform + DT)");
