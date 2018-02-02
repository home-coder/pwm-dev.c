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
#include <linux/pwm.h>

#define DEVNAME            "xlj-pwm"
#define TEST_PWM		   0
#define XLJ_PWM_CHANNEL    0

#define XLJ_PWM_IOC_MAGIC 'm'
#define PWM_CONFIG _IOW(XLJ_PWM_IOC_MAGIC,0,int)
#define PWM_ONOFF _IOW(XLJ_PWM_IOC_MAGIC, 1, int)

struct config_wrapper {
	u32 freq;
	u32 div;
	u32 pol;
} cwrapper;

typedef struct {
	u32 channel;
	u32 polarity;
	u32 period_ns;
	u32 duty_ns;
	u32 divides;
	u32 enabled;
} pwm_info;

typedef struct {
	struct cdev cdev;
	dev_t devid;
	struct class *class;
	struct device *device;
	struct pwm_device *pwm_dev;
	pwm_info pwminfo;
} xlj_pwm;

static struct pwm_device *platform_pwm_request(int pwm_id)
{
	struct pwm_device *pwm_dev;

	pwm_dev = pwm_request(pwm_id, "xlj-pwm");
	if (NULL == pwm_dev || IS_ERR(pwm_dev)) {
		printk("xlj pwm request pwm %d fail!\n", pwm_id);
	} else {
		printk("xlj pwm request pwm %d success!\n", pwm_id);
	}

	return pwm_dev;
}

#if TEST_PWM
static void test_ioctl_pwm(xlj_pwm * xpwm, u32 freq, u32 div, u32 pol)
{
	xpwm->pwminfo.divides = div;
	xpwm->pwminfo.polarity = pol;
	xpwm->pwminfo.period_ns = 1000 * 1000 * 1000 / freq;
	xpwm->pwminfo.duty_ns = (div * xpwm->pwminfo.period_ns) / 256;	//like 11% 25% is also OK !
	pwm_set_polarity(xpwm->pwm_dev, xpwm->pwminfo.polarity);
	pwm_config(xpwm->pwm_dev, xpwm->pwminfo.duty_ns,
		   xpwm->pwminfo.period_ns);

	pwm_enable(xpwm->pwm_dev);
}
#endif

/*
   default initializition
*/
static void xlj_pwm_setup(xlj_pwm * xpwm)
{
	xpwm->pwminfo.channel = XLJ_PWM_CHANNEL;
	xpwm->pwm_dev = platform_pwm_request(xpwm->pwminfo.channel);
	//TODO -----  Default
#if TEST_PWM
	int retry = 6;
	while (retry--) {
		test_ioctl_pwm(xpwm, 2000, 30, 1);
		msleep(1);
	}
#endif
}

static int xpwm_open(struct inode *inode, struct file *filp)
{
	xlj_pwm *xpwm = container_of(inode->i_cdev,
				     xlj_pwm, cdev);

	filp->private_data = xpwm;

	return 0;
}

static int xpwm_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
   AndroidThings Example Below:
   The following example configures the PWM to cycle at 120Hz (period of 8.33ms) with a duty of 25% (on-time of 2.08ms every cycle):
   pwm.setPwmFrequencyHz(120);
   pwm.setPwmDutyCycle(25);
   pwm.setEnabled(true);
*/

static long xpwm_unlocked_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	int ret = -1;
	unsigned long pwmonoff = 0;
	xlj_pwm *xpwm = filp->private_data;

	switch (cmd) {
	case PWM_CONFIG:{
			// tripple args to be checked
			ret = copy_from_user(&cwrapper,
					     (struct config_wrapper __user *)arg,
					     sizeof(struct config_wrapper));
			if (ret) {
				return -EFAULT;
			}

			xpwm->pwminfo.divides = cwrapper.div;
			xpwm->pwminfo.polarity = cwrapper.pol;
			xpwm->pwminfo.period_ns = 1000 * 1000 * 1000 / cwrapper.freq;
			xpwm->pwminfo.duty_ns = (cwrapper.div * xpwm->pwminfo.period_ns) / 256;	//like 11% 25% is also OK !
			pwm_set_polarity(xpwm->pwm_dev, xpwm->pwminfo.polarity);
			pwm_config(xpwm->pwm_dev, xpwm->pwminfo.duty_ns,
				   xpwm->pwminfo.period_ns);
		}
		break;
	case PWM_ONOFF:{
			get_user(pwmonoff, (unsigned long __user*)arg);
			if (pwmonoff) {
				pwm_enable(xpwm->pwm_dev);
			} else {
				pwm_disable(xpwm->pwm_dev);
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

static struct file_operations xpwm_fops = {
	.owner = THIS_MODULE,
	.open = xpwm_open,
	.release = xpwm_release,
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

	xpwm->device =
	    device_create(xpwm->class, NULL, xpwm->devid, NULL, DEVNAME);
	if (!xpwm->device) {
		printk("xlj pwm create device failed\n");
		goto fail2;
	}

	printk("=====================setup===========================\n");
	xlj_pwm_setup(xpwm);

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

static void xlj_pwm_release(struct device *dev)
{
	return;
}

static struct platform_device xlj_pwm_device = {
	.name = "xlj_pwm",
	.id = -1,
	.dev = {
		.release = xlj_pwm_release,
		},
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
