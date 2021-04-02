#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;


int find_nth_occur(string str, char ch, int n) {
    int occur = 0;
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == ch)
            occur++;
        if (occur == n)
            return i;
    }
    return -1;
}


class User {
public:
    User(string _user, string _password, bool _admin, int _size) {
        user = _user;
        password = _password;
        admin = _admin;
        size = _size;
    }

    string get_user() { return user; }
    string get_password() { return password; }
    bool is_admin() { return admin; }
    int get_size() { return size; }

    void decrease_size(int amount) { size -= amount; }
private:
    string user;
    string password;
    bool admin;
    int size;
};


class Server {
public:
    void config() {
        ifstream file("config.json");
        string temp;
        getline(file, temp, ',');
        command_channel_port = atoi(temp.substr(temp.find(':') + 1).c_str());
        getline(file, temp, ',');
        data_channel_port = atoi(temp.substr(temp.find(':') + 1).c_str());
        getline(file, temp, '[');
        getline(file, temp, ']');
        stringstream stream(temp);
        string user, password, admin, size;
        while (getline(stream, temp, ',')) {
            user = temp.substr(find_nth_occur(temp, '"', 3) + 1, find_nth_occur(temp, '"', 4) - find_nth_occur(temp, '"', 3) - 1);
            getline(stream, temp, ',');
            password = temp.substr(find_nth_occur(temp, '"', 3) + 1, find_nth_occur(temp, '"', 4) - find_nth_occur(temp, '"', 3) - 1);
            getline(stream, temp, ',');
            admin = temp.substr(find_nth_occur(temp, '"', 3) + 1, find_nth_occur(temp, '"', 4) - find_nth_occur(temp, '"', 3) - 1);
            getline(stream, temp, ',');
            size = temp.substr(find_nth_occur(temp, '"', 3) + 1, find_nth_occur(temp, '"', 4) - find_nth_occur(temp, '"', 3) - 1);
            users.push_back(User(user, password, (admin == "true"), atoi(size.c_str())));
        }
        getline(file, temp, '[');
        getline(file, temp, ']');
        stringstream new_stream(temp);
        string special_file;
        while (getline(new_stream, temp, ',')) {
            special_file = temp.substr(find_nth_occur(temp, '"', 1) + 1, find_nth_occur(temp, '"', 2) - find_nth_occur(temp, '"', 1) - 1);
            special_files.push_back(special_file);
        }
    }

    void setup() {
        int opt = 1;
        int i;

        client_sock_cmd = (int*)malloc(10 * sizeof(int));
        for (i = 0; i < 10; i++)
            client_sock_cmd[i] = 0;

        client_sock_data = (int*)malloc(10 * sizeof(int));
        for (i = 0; i < 10; i++)
            client_sock_data[i] = 0;

        sockfd_cmd = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(sockfd_cmd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));
        addr_cmd.sin_family = AF_INET;
        addr_cmd.sin_port = htons(command_channel_port);
        inet_pton(AF_INET, "127.0.0.1", &addr_cmd.sin_addr.s_addr);

        sockfd_data = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(sockfd_data, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));
        addr_data.sin_family = AF_INET;
        addr_data.sin_port = htons(data_channel_port);
        inet_pton(AF_INET, "127.0.0.1", &addr_data.sin_addr.s_addr);

        bind(sockfd_cmd, (struct sockaddr *)&addr_cmd, sizeof(addr_cmd));
        bind(sockfd_data, (struct sockaddr *)&addr_data, sizeof(addr_data));

        listen(sockfd_cmd, 5);
        listen(sockfd_data, 5);
    }

    void run() {
        fd_set fds;
        int max_sd, i, len, new_sock_cmd, new_sock_data, flag, valread;
        int cli_size_cmd = 10;
        int cli_size_data = 10;
        while (1) {
            FD_ZERO(&fds);
            FD_SET(sockfd_cmd, &fds);
            max_sd = sockfd_cmd;
            for (i = 0; i < cli_size_cmd; i++) {
                if (client_sock_cmd[i] > 0)
                    FD_SET(client_sock_cmd[i], &fds);
                if (client_sock_cmd[i] > max_sd)
                    max_sd = client_sock_cmd[i];
            }

            select(max_sd + 1, &fds, NULL, NULL, NULL);

            if (FD_ISSET(sockfd_cmd, &fds)) {
                len = sizeof(addr_cmd);
                new_sock_cmd = accept(sockfd_cmd, (struct sockaddr *)&addr_cmd, (socklen_t *)&len);

                flag = 0;
                for (i = 0; i < cli_size_cmd; i++) {
                    if (client_sock_cmd[i] == 0) {
                        client_sock_cmd[i] = new_sock_cmd;
                        flag = 1;
                        break;
                    }
                }
                if (flag == 0) {
                    cli_size_cmd++;
                    client_sock_cmd = (int*)realloc(client_sock_cmd, cli_size_cmd * sizeof(int));
                    client_sock_cmd[cli_size_cmd - 1] = new_sock_cmd;
                }

                len = sizeof(addr_data);
                new_sock_data = accept(sockfd_data, (struct sockaddr *)&addr_data, (socklen_t *)&len);

                flag = 0;
                for (i = 0; i < cli_size_data; i++) {
                    if (client_sock_data[i] == 0) {
                        client_sock_data[i] = new_sock_data;
                        flag = 1;
                        break;
                    }
                }
                if (flag == 0) {
                    cli_size_data++;
                    client_sock_data = (int*)realloc(client_sock_data, cli_size_data * sizeof(int));
                    client_sock_data[cli_size_data - 1] = new_sock_data;
                }
            }

            for (i = 0; i < cli_size_cmd; i++) {
                if (FD_ISSET(client_sock_cmd[i], &fds)) {
                    valread = read(client_sock_cmd[i], buffer_cmd, 1024);
                    if (valread == 0) {
                        close(client_sock_cmd[i]);
                        client_sock_cmd[i] = 0;
                        close(client_sock_data[i]);
                        client_sock_data[i] = 0;
                    }
                    else {
                        // Process Command
                    }
                }
            }
        }
    }
private:
    int command_channel_port;
    int data_channel_port;
    vector<User> users;
    vector<string> special_files;
    struct sockaddr_in addr_cmd;
    struct sockaddr_in addr_data;
    int sockfd_cmd;
    int sockfd_data;
    int *client_sock_cmd;
    int *client_sock_data;
    char buffer_cmd[1024] = {0};
};


int main(int argc, char const *argv[]) {
    Server server;
    server.config();
    server.setup();
    server.run();
    return 0;
}