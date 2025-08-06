#include "tcpclient.h"

int tcpclient_init()
{
    int sockfd, ret;
    struct sockaddr_in server;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("create an endpoint for communication fail!\n");
        return -1;
    }
    
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(HOST);

    // 建立TCP连接
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        printf("connect server fail...\n");
        close(sockfd);
        return -1;
    }

    printf("connect server success...\n");
    write(sockfd, DEVICE_ID, strlen(DEVICE_ID));            // Send Device ID to server
    return sockfd;
}

void tcpclient_run(int connfd, std::atomic<bool>& servo_mode, std::atomic<int>& yaw_servo_angle, std::atomic<int>& pitch_servo_angle)
{
    char buff[MAX];
    printf("TCP client is running...\n");
    int angleSpeed = 5; // Default angle speed
    while(1)
    {
        bzero(buff, MAX);
        // 读取数据
        int n = recv(connfd, buff, sizeof(buff) - 1, 0); 
        if (n <= 0) {
            printf("read error or connection closed by server\n");
            break;
        }
        // 打印接收到的数据
        buff[n] = '\0'; // 确保字符串以null结尾
        
        // 先移除换行符
        buff[strcspn(buff, "\r\n")] = '\0';
        printf("From Server: %s\n", buff);
        
        // 提取最后一个字符作为速度值并转换为角度步进
        int len = strlen(buff);
        char speedChar = buff[len - 1];
        if (speedChar >= '1' && speedChar <= '9') {
            angleSpeed = speedChar - '0';
            printf("angle speed: %d\n", angleSpeed);
        } 


        if(strcmp("manual", buff) == 0)
        {
            servo_mode = true;
        }
        else if(strcmp("auto", buff) == 0)
        {
            servo_mode = false;
        }

        if(servo_mode)
        {
            switch (buff[0]) {
                case 'l': // Increase yaw angle
                    yaw_servo_angle += angleSpeed;
                    printf("Yaw angle increased to: %d\n", yaw_servo_angle.load());
                    break;
                case 'r': // Decrease yaw angle
                    yaw_servo_angle -= angleSpeed;
                    printf("Yaw angle decreased to: %d\n", yaw_servo_angle.load());
                    break;
                case 'u': // Increase pitch angle
                    pitch_servo_angle += angleSpeed;
                    printf("Pitch angle increased to: %d\n", pitch_servo_angle.load());
                    break;
                case 'd': // Decrease pitch angle
                    pitch_servo_angle -= angleSpeed;
                    printf("Pitch angle decreased to: %d\n", pitch_servo_angle.load());
                    break;
                case 'R': // Decrease pitch angle
                    yaw_servo_angle = 45; // Reset yaw angle
                    pitch_servo_angle = 90; // Reset pitch angle
                    printf("Yaw and Pitch angles reset to default.\n");
                    break;
                default:
                    printf("Unknown command: %s\n", buff);
            }
            yaw_servo_set_angle(yaw_servo_angle);
            pitch_servo_set_angle(pitch_servo_angle);
        }
    }

    close(connfd);
    return;
}