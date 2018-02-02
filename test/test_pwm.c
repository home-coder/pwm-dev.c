#include <stdio.h>
#include <linux/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>

//ioctl magic
#define XLJ_PWM_IOC_MAGIC 'm'
#define PWM_CONFIG _IOW(XLJ_PWM_IOC_MAGIC,0,int)
#define PWM_ONOFF _IOW(XLJ_PWM_IOC_MAGIC, 1, int)

struct config_wrapper {
	unsigned int freq;
	unsigned int div;
	unsigned int  pol;
} cwrapper;

int main()
{
	//open
	int fd = -1, ret = -1;

	fd = open("/dev/xlj-pwm", O_RDWR);
	if (fd == -1) {
		printf("open failed\n");
		return -1;
	}

	//config
	cwrapper.freq = 2000; //2K Hz
	cwrapper.div = 33;
	cwrapper.pol = 1;
	ret = ioctl(fd, PWM_CONFIG, (unsigned long)&cwrapper);
	if (ret < 0) {
		printf("====ioctl config failed====\n");
	}
	//enable
	int on = 1;
	ret = ioctl(fd, PWM_ONOFF, &on);
	if (ret < 0) {
		printf("====ioctl config failed====\n");
	}

	return 0;
}
