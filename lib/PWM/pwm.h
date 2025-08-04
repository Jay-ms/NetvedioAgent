#ifndef __PWM_H
#define __PWM_H

#include <stdio.h>

#define PWM_PATH "/sys/class/pwm"
#define PWM_CHANNEL "0"
#define PERIOD_NS 20000000                  // 20ms period
#define MIN_DUTY_CYCLE_NS_Steer1 1000000            // 0бу 1ms
#define MID_DUTY_CYCLE_NS_Steer1 1500000            // 90бу 2ms
#define MAX_DUTY_CYCLE_NS_Steer1 3000000           // 180бу 3ms

#define MIN_DUTY_CYCLE_NS_Steer2 1000000            // 0бу 1ms
#define MID_DUTY_CYCLE_NS_Steer2 2000000            // 90бу 2ms
#define MAX_DUTY_CYCLE_NS_Steer2 3000000            // 180бу 3ms

int pwm_export(const char *pwm_chip);
int pwm_unexport(const char *pwm_chip);
int pwm_enable(const char *pwm_chip, bool enable);
int pwm_set_period(const char *pwm_chip, int period_ns);
int pwm_set_duty_cycle(const char *pwm_chip, int duty_ns);
void servo_init(void);
void servo_control(void);
void yaw_servo_set_angle(int angle);
void pitch_servo_set_angle(int angle);
void servo_deinit(void);

#endif
