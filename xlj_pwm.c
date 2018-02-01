#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/string.h>

#define DEVNAME "xlj-pwm"

typedef struct {
	struct cdev cdev;
	dev_t devid;
	struct class *class;
	struct device *device;

	struct {
		unsigned int dev;
		u32 channel;
		u32 polarity;
		u32 period_ns;
		u32 duty_ns;
		u32 enabled;
	} pwm_info;
} xlj_pwm;

static int xpwm_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int xpwm_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long xpwm_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations xpwm_fops = {
	.owner      = THIS_MODULE,
	.open       = xpwm_open,
	.release    = xpwm_release,
	.unlocked_ioctl = xpwm_unlocked_ioctl,
};

static int __devinit xlj_pwm_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct cdev *pcdev;
	xlj_pwm *xpwm = kzalloc(sizeof(xlj_pwm), GFP_KERNEL);
	if (!xpwm) {
		return -ENOMEM;
	}

	alloc_chrdev_region(&xpwm->devid, 0, 1, DEVNAME);
	pcdev = cdev_alloc();
	xpwm->cdev = *pcdev;
	cdev_init(&xpwm->cdev, &xpwm_fops);
	xpwm->cdev.owner = THIS_MODULE;
	ret = cdev_add(&xpwm->cdev, xpwm->devid, 1);
	if (ret) {
		printk("cdev_add xlj pwm failed\n");
		goto fail0;
	}

	xpwm->class = class_create(THIS_MODULE, DEVNAME);
	if (IS_ERR(xpwm->class)) {
		printk("xlj pwm create device class failed\n");
		goto fail1;
	}

	xpwm->device = device_create(xpwm->class, NULL, xpwm->devid, NULL, DEVNAME);
	if (!xpwm->device) {
		printk("xlj pwm create device failed\n");
		goto fail2;
	}

	return 0;

fail2:
	device_destroy(xpwm->class, xpwm->devid);
fail1:
	class_destroy(xpwm->class);
fail0:
	cdev_del(&xpwm->cdev);
	kfree(xpwm);

	return ret;
}

static int xlj_pwm_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver xlj_pwm_driver = {
	.probe = xlj_pwm_probe,
	.remove = xlj_pwm_remove,
	.driver = {
		.name = "xlj_pwm",
		.owner = THIS_MODULE,
	},
};

static struct platform_device xlj_pwm_device = {
	.name   = "xlj_pwm",
	.id = -1,
};


static __init int module_xlj_pwm_init(void)
{
	int ret = -1;
	printk("Hello World\n");
	ret = platform_device_register(&xlj_pwm_device);
	if (ret < 0) {
		printk("ERROR:Fail to platform_device_register!\n");
		return ret;
	}

	ret = platform_driver_register(&xlj_pwm_driver);
	if (ret < 0) {
		printk("ERROR:Fail to platform_driver_register!\n");
		return ret;
	}

	return ret;
}

static __exit void module_xlj_pwm_exit(void)
{
	printk("Byebye World\n");
	platform_device_unregister(&xlj_pwm_device);
	platform_driver_unregister(&xlj_pwm_driver);
}

//insmod xxx.ko
module_init(module_xlj_pwm_init);
//rmmod xxx.ko
module_exit(module_xlj_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("oneface");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("xlj_pwm");
