// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include "pwm_utils.h"

#define DEVICE_PATH "/dev/aircon_dev"

#define AIRCON_MAGIC 'A'
#define AIRCON_GET_LEVEL _IOR(AIRCON_MAGIC, 2, int)

#define AIRCON_LEVEL_OFF  0
#define AIRCON_LEVEL_LOW  1
#define AIRCON_LEVEL_MID  2
#define AIRCON_LEVEL_HIGH 3

#define PWM_CHIP         0
#define PWM_CHANNEL      1  // wiper가 pwm0 쓰므로 여기서는 pwm1
#define PWM_PERIOD_NS    20000000
#define DUTY_LOW_NS       500000   // 2.5%
#define DUTY_MID_NS      1000000   // 5.0%
#define DUTY_HIGH_NS     1500000   // 7.5%
#define DUTY_BOOST_NS    1900000   // 부스트용 (95%)

#define BOOST_DURATION_MS 200

static bool keep_running = true;

void handle_sigint(int sig) {
    keep_running = false;
}

int level_to_duty(int level) {
    switch (level) {
        case AIRCON_LEVEL_LOW:  return DUTY_LOW_NS;
        case AIRCON_LEVEL_MID:  return DUTY_MID_NS;
        case AIRCON_LEVEL_HIGH: return DUTY_HIGH_NS;
        default: return 0;
    }
}

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open /dev/aircon_dev");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);

    pwm_export(PWM_CHIP, PWM_CHANNEL);
    pwm_set_period(PWM_CHIP, PWM_CHANNEL, PWM_PERIOD_NS);
    pwm_enable(PWM_CHIP, PWM_CHANNEL, 0);

    int prev_level = -1;

    printf("Aircon daemon started.\n");

    while (keep_running) {
        int level;
        if (ioctl(fd, AIRCON_GET_LEVEL, &level) < 0) {
            perror("ioctl AIRCON_GET_LEVEL");
            break;
        }

        if (level != prev_level) {
            int duty = level_to_duty(level);

            if (level == AIRCON_LEVEL_OFF) {
                pwm_enable(PWM_CHIP, PWM_CHANNEL, 0);
                printf("Aircon OFF\n");

            } else if (level == AIRCON_LEVEL_LOW) {
                // 부스트 200ms
                pwm_set_duty_cycle(PWM_CHIP, PWM_CHANNEL, DUTY_BOOST_NS);
                pwm_enable(PWM_CHIP, PWM_CHANNEL, 1);
                usleep(BOOST_DURATION_MS * 1000);

                pwm_set_duty_cycle(PWM_CHIP, PWM_CHANNEL, duty);
                printf("Aircon LOW → boost(%d ns) → duty = %d ns\n", DUTY_BOOST_NS, duty);

            } else {
                pwm_set_duty_cycle(PWM_CHIP, PWM_CHANNEL, duty);
                pwm_enable(PWM_CHIP, PWM_CHANNEL, 1);
                printf("Aircon level %d → duty = %d ns\n", level, duty);
            }

            prev_level = level;
        }

        usleep(200000); // 200ms polling 주기
    }

    pwm_enable(PWM_CHIP, PWM_CHANNEL, 0);
    pwm_unexport(PWM_CHIP, PWM_CHANNEL);
    close(fd);

    printf("Aircon daemon terminated.\n");
    return 0;
}
