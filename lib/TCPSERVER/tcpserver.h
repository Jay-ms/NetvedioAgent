#ifndef __TCPSERVER_H
#define __TCPSERVER_H

#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <atomic>
#include <pwm.h>

#define MAX 10*1024
#define PORT 6666
extern int sockfd; // Global socket file descriptor

int tcpserver_init();
void tcpserver_run(int connfd, std::atomic<bool>& servo_mode, std::atomic<int>& yaw_servo_angle, std::atomic<int>& pitch_servo_angle);

#endif