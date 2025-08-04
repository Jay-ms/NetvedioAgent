#include "tcpserver.h"

int tcpserver_init()
{
    int n;
    int connfd;
    struct sockaddr_in server, client;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        return -1;
    }

    printf("socket successfully created..\n");
    bzero(&server, sizeof(server));

    // assign IP, PORT
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    // binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr*)&server, sizeof(server))) != 0) {
        printf("socket bind failed...\n");
        return -1;
    }

    printf("socket successfully binded..\n");

    // now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        return -1;
    }

    printf("server listening...\n");

    socklen_t len = sizeof(client);

    // accept the data packet from client and verification
    connfd = accept(sockfd, (struct sockaddr*)&client, &len);
    if (connfd < 0) {
        printf("server acccept failed...\n");
        return -1;
    }

    printf("server acccept the client...\n");

    return connfd; // Return the connected socket file descriptor
}


void tcpserver_run(int connfd, std::atomic<bool>& servo_mode, std::atomic<int>& yaw_servo_angle, std::atomic<int>& pitch_servo_angle)
{
    char buff[MAX];
    while(1)
    {
        bzero(buff, MAX);
        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));
        printf("From client: %s\n", buff);

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
    close(sockfd);
    return;
}