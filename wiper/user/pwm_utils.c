#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "pwm_utils.h"

#define SYSFS_PWM_BASE "/sys/class/pwm"

static int write_sysfs(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror(path);
        return -1;
    }

    if (write(fd, value, strlen(value)) < 0) {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int pwm_export(int chip, int channel)
{
    char path[128];
    char val[16];
    snprintf(path, sizeof(path), SYSFS_PWM_BASE "/pwmchip%d/export", chip);
    snprintf(val, sizeof(val), "%d", channel);
    return write_sysfs(path, val);
}

int pwm_unexport(int chip, int channel)
{
    char path[128];
    char val[16];
    snprintf(path, sizeof(path), SYSFS_PWM_BASE "/pwmchip%d/unexport", chip);
    snprintf(val, sizeof(val), "%d", channel);
    return write_sysfs(path, val);
}

int pwm_set_period(int chip, int channel, int period_ns)
{
    char path[128];
    char val[32];
    snprintf(path, sizeof(path), SYSFS_PWM_BASE "/pwmchip%d/pwm%d/period", chip, channel);
    snprintf(val, sizeof(val), "%d", period_ns);
    return write_sysfs(path, val);
}

int pwm_set_duty_cycle(int chip, int channel, int duty_ns)
{
    char path[128];
    char val[32];
    snprintf(path, sizeof(path), SYSFS_PWM_BASE "/pwmchip%d/pwm%d/duty_cycle", chip, channel);
    snprintf(val, sizeof(val), "%d", duty_ns);
    return write_sysfs(path, val);
}

int pwm_enable(int chip, int channel, int enable)
{
    char path[128];
    snprintf(path, sizeof(path), SYSFS_PWM_BASE "/pwmchip%d/pwm%d/enable", chip, channel);
    return write_sysfs(path, enable ? "1" : "0");
}
