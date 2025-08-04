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

    // ����TCP����
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
    while(1)
    {
        bzero(buff, MAX);
        // ��ȡ����
        int n = recv(connfd, buff, sizeof(buff) - 1, 0); 
        if (n <= 0) {
            printf("read error or connection closed by server\n");
            break;
        }
        // ��ӡ���յ�������
        buff[n] = '\0'; // ȷ���ַ�����null��β
        buff[strcspn(buff, "\r\n")] = '\0';
        printf("From Server: %s\n", buff);

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
                    yaw_servo_angle += 5;
                    printf("Yaw angle increased to: %d\n", yaw_servo_angle.load());
                    break;
                case 'r': // Decrease yaw angle
                    yaw_servo_angle -= 5;
                    printf("Yaw angle decreased to: %d\n", yaw_servo_angle.load());
                    break;
                case 'u': // Increase pitch angle
                    pitch_servo_angle += 5;
                    printf("Pitch angle increased to: %d\n", pitch_servo_angle.load());
                    break;
                case 'd': // Decrease pitch angle
                    pitch_servo_angle -= 5;
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