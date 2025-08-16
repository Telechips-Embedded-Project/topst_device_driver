#ifndef PWM_UTILS_H
#define PWM_UTILS_H

int pwm_export(int chip, int channel);
int pwm_unexport(int chip, int channel);
int pwm_set_period(int chip, int channel, int period_ns);
int pwm_set_duty_cycle(int chip, int channel, int duty_ns);
int pwm_enable(int chip, int channel, int enable);

#endif // PWM_UTILS_H
