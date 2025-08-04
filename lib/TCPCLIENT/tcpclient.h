#ifndef __TCPCLIENT_H
#define __TCPCLIENT_H
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <atomic>
#include <pwm.h>

#define HOST "192.168.31.100"
#define PORT 6666
#define MAX 10*1024
#define DEVICE_ID "device1\n"

int tcpclient_init();
void tcpclient_run(int connfd, std::atomic<bool>& servo_mode, std::atomic<int>& yaw_servo_angle, std::atomic<int>& pitch_servo_angle);

#endif