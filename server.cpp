#include <iostream>
#include <fstream>
#include <string>
#include <vector>
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
private:
    int command_channel_port;
    int data_channel_port;
    vector<User> users;
    vector<string> special_files;
};


int main(int argc, char const *argv[]) {
    Server server;
    server.config();
    return 0;
}