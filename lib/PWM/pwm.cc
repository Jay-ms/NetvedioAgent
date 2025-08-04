#include "pwm.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int direction = 1;
int duty_ns_Steer1 ;
int duty_ns_Steer2 ;

int pwm_export(const char *pwm_chip)
{
    char path[128];
    snprintf(path, sizeof(path), PWM_PATH"/%s/export", pwm_chip);
    
    FILE *fp = fopen(path, "w");
    if(!fp) {
        if (errno == EBUSY) return 0;           // PWM already exported
        perror("Failed to open PWM export");
        return -1;
    }
    fprintf(fp, PWM_CHANNEL);
    fclose(fp);
    usleep(100000); // Wait for the system to create pwm0 directory
    return 0;
}

int pwm_unexport(const char *pwm_chip)
{
    char path[128];
    snprintf(path, sizeof(path), PWM_PATH"/%s/unexport", pwm_chip);
    FILE *fp = fopen(path, "w");
    if(!fp){
        perror("Failed to open PWM unexport");
        return -1;
    }
    fprintf(fp, PWM_CHANNEL);
    fclose(fp);
    return 0;
}

int pwm_enable(const char *pwm_chip, bool enable)
{
    char path[128];
    snprintf(path, sizeof(path), PWM_PATH"/%s/pwm%s/enable", pwm_chip, PWM_CHANNEL);
    FILE *fp = fopen(path, "w");
    if(!fp) {
        perror("Failed to open PWM enable");
        return -1;
    }
    fprintf(fp, enable ? "1" : "0");
    fclose(fp);
    return 0;
}

int pwm_set_period(const char *pwm_chip, int period_ns)
{
    char path[128];
    snprintf(path, sizeof(path), PWM_PATH"/%s/pwm%s/period", pwm_chip, PWM_CHANNEL);
    FILE *fp = fopen(path, "w");
    if(!fp){
        perror("Failed to open PWM period");
        return -1;
    }
    fprintf(fp, "%d", period_ns);
    fclose(fp);
    return 0;
}

int pwm_set_duty_cycle(const char *pwm_chip, int duty_ns)
{
    char path[128];
    snprintf(path, sizeof(path), PWM_PATH"/%s/pwm%s/duty_cycle", pwm_chip, PWM_CHANNEL);
    FILE *fp = fopen(path, "w");
    if(!fp) {
        perror("Failed to open PWM duty cycle");
        return -1;
    }
    fprintf(fp, "%d", duty_ns);
    fclose(fp);
    return 0;
}

void servo_init(void)
{
    pwm_export("pwmchip10");
    pwm_set_period("pwmchip10",PERIOD_NS);
    pwm_enable("pwmchip10", true);
    pwm_set_duty_cycle("pwmchip10", MID_DUTY_CYCLE_NS_Steer1); // Set initial duty cycle to 1/2 of the period

    pwm_export("pwmchip11");
    pwm_set_period("pwmchip11",PERIOD_NS);
    pwm_enable("pwmchip11", true);
    pwm_set_duty_cycle("pwmchip11", MID_DUTY_CYCLE_NS_Steer2);
}

void servo_control(void)
{
    duty_ns_Steer1 += direction * 50000; // Increment or decrement by 50ms
    if (duty_ns_Steer1 >= MAX_DUTY_CYCLE_NS_Steer1)
    {
        duty_ns_Steer1 = MAX_DUTY_CYCLE_NS_Steer1; // Cap at maximum duty cycle
        direction = -1; // Change direction to decrement
    }
    else if (duty_ns_Steer1 <= MIN_DUTY_CYCLE_NS_Steer1)
    {
        duty_ns_Steer1 = MIN_DUTY_CYCLE_NS_Steer1; // Cap at minimum duty cycle
        direction = 1; // Change direction to increment
    }
    pwm_set_duty_cycle("pwmchip10",duty_ns_Steer1); // Set the new duty cycle

    duty_ns_Steer2 += direction * 50000; // Increment or decrement by 50ms
    if (duty_ns_Steer2 >= MAX_DUTY_CYCLE_NS_Steer2)
    {
        duty_ns_Steer2 = MAX_DUTY_CYCLE_NS_Steer2; // Cap at maximum duty cycle
        direction = -1; // Change direction to decrement
    }
    else if (duty_ns_Steer2 <= MIN_DUTY_CYCLE_NS_Steer2)
    {
        duty_ns_Steer2 = MIN_DUTY_CYCLE_NS_Steer2; // Cap at minimum duty cycle
        direction = 1; // Change direction to increment
    }
    pwm_set_duty_cycle("pwmchip11",duty_ns_Steer2);
}

void yaw_servo_set_angle(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 90) angle = 90;

    // Calculate the duty cycle based on the angle
    duty_ns_Steer1 = 1000000 + angle * 1000000 / 90;
    printf("Setting yaw servo angle to %d, duty_ns_Steer1 = %d\n", angle, duty_ns_Steer1);
    pwm_set_duty_cycle("pwmchip10", duty_ns_Steer1);
}

void pitch_servo_set_angle(int angle)
{
    if (angle < 45) angle = 45;
    if (angle > 135) angle = 135;

    // Calculate the duty cycle based on the angle
    duty_ns_Steer2 = 1000000 + angle * 1000000 / 90;
    printf("Setting pitch servo angle to %d, duty_ns_Steer2 = %d\n", angle, duty_ns_Steer2);
    pwm_set_duty_cycle("pwmchip11", duty_ns_Steer2);
}

void servo_deinit(void)
{
    pwm_unexport("pwmchip10");
    pwm_unexport("pwmchip11");
}