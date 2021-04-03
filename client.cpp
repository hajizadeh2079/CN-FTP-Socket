#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;


int main(int argc, char* argv[]) {

    int port_cmd, sockfd_cmd, port_data, sockfd_data;
    struct sockaddr_in serv_addr_cmd, serv_addr_data;
    char buffer_cmd[1024] = {0};
    char buffer_data[1024] = {0};

    port_cmd = atoi(argv[1]);
    port_data = atoi(argv[2]);

    sockfd_cmd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr_cmd.sin_family = AF_INET;
    serv_addr_cmd.sin_port = htons(port_cmd);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr_cmd.sin_addr.s_addr);

    sockfd_data = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr_data.sin_family = AF_INET;
    serv_addr_data.sin_port = htons(port_data);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr_data.sin_addr.s_addr);

    connect(sockfd_cmd, (struct sockaddr *)&serv_addr_cmd, sizeof(serv_addr_cmd));
    connect(sockfd_data, (struct sockaddr *)&serv_addr_data, sizeof(serv_addr_data));

    return 0;
}