// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pwm_utils.h"

#define DEVICE_PATH "/dev/wiper_dev"

#define WIPER_MAGIC 'W'
#define WIPER_SET_MODE _IOW(WIPER_MAGIC, 1, int)
#define WIPER_GET_MODE _IOR(WIPER_MAGIC, 2, int)

#define WIPER_MODE_OFF  0
#define WIPER_MODE_FAST 1
#define WIPER_MODE_SLOW 2

#define PWM_CHIP        0
#define PWM_CHANNEL     0
#define PWM_PERIOD_NS   20000000  // 20ms (50Hz)

#define ANGLE_MIN       0
#define ANGLE_MAX       180
#define DUTY_MIN_NS     1000000   // 1.0ms
#define DUTY_MAX_NS     2000000   // 2.0ms

#define FAST_DELAY_US   3000      // 2ms
#define SLOW_DELAY_US   4000      // 4ms

static bool keep_running = true;

void handle_sigint(int sig) {
    keep_running = false;
}

// 각도 → 듀티 변환
unsigned int angle_to_duty(int angle)
{
    if (angle < ANGLE_MIN) angle = ANGLE_MIN;
    if (angle > ANGLE_MAX) angle = ANGLE_MAX;
    return DUTY_MIN_NS + (DUTY_MAX_NS - DUTY_MIN_NS) * angle / 180;
}

int main(void)
{
    int fd, mode = WIPER_MODE_OFF;

    signal(SIGINT, handle_sigint);

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open wiper_dev");
        return EXIT_FAILURE;
    }

    pwm_export(PWM_CHIP, PWM_CHANNEL);
    pwm_set_period(PWM_CHIP, PWM_CHANNEL, PWM_PERIOD_NS);
    pwm_enable(PWM_CHIP, PWM_CHANNEL, 0);

    printf("Wiper daemon started (sweeping).\n");

    while (keep_running) {
        if (ioctl(fd, WIPER_GET_MODE, &mode) < 0) {
            perror("ioctl(GET_MODE)");
            break;
        }

        if (mode == WIPER_MODE_OFF) {
            unsigned int duty = angle_to_duty(90); // 중간
            pwm_set_duty_cycle(PWM_CHIP, PWM_CHANNEL, duty);
            pwm_enable(PWM_CHIP, PWM_CHANNEL, 1);
            usleep(100000); // 100ms
            continue;
        }

        int delay_us = (mode == WIPER_MODE_FAST) ? FAST_DELAY_US : SLOW_DELAY_US;

        // 0 → 180
        for (int angle = 0; angle <= 180 && keep_running; angle++) {
            unsigned int duty = angle_to_duty(angle);
            pwm_set_duty_cycle(PWM_CHIP, PWM_CHANNEL, duty);
            pwm_enable(PWM_CHIP, PWM_CHANNEL, 1);
            usleep(delay_us);
        }

        // 180 → 0
        for (int angle = 180; angle >= 0 && keep_running; angle--) {
            unsigned int duty = angle_to_duty(angle);
            pwm_set_duty_cycle(PWM_CHIP, PWM_CHANNEL, duty);
            pwm_enable(PWM_CHIP, PWM_CHANNEL, 1);
            usleep(delay_us);
        }
    }

    pwm_enable(PWM_CHIP, PWM_CHANNEL, 0);
    pwm_unexport(PWM_CHIP, PWM_CHANNEL);
    close(fd);

    printf("Wiper daemon terminated.\n");
    return 0;
}
