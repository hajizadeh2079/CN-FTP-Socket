#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <ctime>
#include <bits/stdc++.h>
#include <sys/stat.h>
#include <dirent.h>

#define USER_OK_MSG "331: User name okay, need password."
#define USER_PASSWORD_INVALID_MSG "430: Invalid username or password"
#define BAD_SEQ "503: Bad sequence of commands"
#define SUCCESSFUL_LOGIN "230: User logged in, proceed. Logged out if appropriate."
#define NEED_LOGIN "332: Need account for login."
#define SUCCESSFUL_QUIT "221: Successful Quit."
#define WRONG_CMD "501: Syntax error in parameters or arguments"
#define ERROR "500: Error"
#define SUCCESSFUL_CHANGE "250: Successful change."
#define ACCESS_ERROR "550: File unavailable."
#define LS_DONE "226: List transfer done."
#define SUCCESSFUL_DOWNLOAD "226: Successful Download."
#define SIZE_ERROR "425: Can't open data connection."

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

bool is_path_exist(string path) {
    struct stat buffer;
    return (stat (path.c_str(), &buffer) == 0);
}

string get_current_data_time() {
    time_t tt;
    struct tm * ti;
    time (&tt);
    ti = localtime(&tt);
    return string(asctime(ti));
}

vector<string> convert_string_to_vector(string str) {
    vector<string> temp;
    istringstream ss(str);
    string word;
    while(ss >> word)
        temp.push_back(word);
    return temp;
}


class User {
public:
    User(string _user, string _password, string _directory ,bool _admin, int _size) {
        user = _user;
        password = _password;
        directory = _directory;
        admin = _admin;
        size = _size * 1024;
    }

    string get_user() { return user; }
    string get_password() { return password; }
    bool is_admin() { return admin; }
    int get_size() { return size; }
    string get_directory() { return directory; }

    void set_directory(string dir) { directory = dir; }

    void decrease_size(int amount) { size -= amount; }
private:
    string user;
    string password;
    string directory;
    bool admin;
    int size;
};


class Handler {
public:
    string handle_cmd(string cmd, map<int, string> &login_user, map<int, bool> &does_login, map<int, int> &cmd_data, vector<User> &users, vector<string> &special_files, int socket_num) {
        vector<string> cmd_vector = convert_string_to_vector(cmd);
        if(cmd_vector.size() == 0)
            return WRONG_CMD;
        if(cmd_vector[0] == "user" && cmd_vector.size() == 2)
            return check_user(cmd_vector[1], login_user, does_login, users, socket_num);
        if(cmd_vector[0] == "pass" && cmd_vector.size() == 2)
            return check_pass(cmd_vector[1], login_user, does_login, users, socket_num);
        if(cmd_vector[0] == "quit" && cmd_vector.size() == 1)
            return quit_user(login_user, does_login, users, socket_num);
        if(cmd_vector[0] == "pwd" && cmd_vector.size() == 1)
            return show_current_dir(login_user, does_login, users, socket_num);
        if(cmd_vector[0] == "mkd" && cmd_vector.size() == 2)
            return make_new_dir(cmd_vector[1], login_user, does_login, users, socket_num);
        if(cmd_vector[0] == "mkf" && cmd_vector.size() == 2)
            return make_new_file(cmd_vector[1], login_user, does_login, users, socket_num);
        if(cmd_vector[0] == "dele" && cmd_vector.size() == 3) {
            if(cmd_vector[1] == "-d")
                return delete_dir(cmd_vector[2], login_user, does_login, users, socket_num);
            if(cmd_vector[1] == "-f")
                return delete_file(cmd_vector[2], login_user, does_login, users, special_files, socket_num);
        }
        if(cmd_vector[0] == "cwd" && cmd_vector.size() <= 2) {
            if(cmd_vector.size() == 1)
                return change_dir("", login_user, does_login, users, socket_num);
            else
                return change_dir(cmd_vector[1], login_user, does_login, users, socket_num);
        }
        if(cmd_vector[0] == "rename" && cmd_vector.size() == 3)
            return change_file_name(cmd_vector[1], cmd_vector[2], login_user, does_login, users, special_files, socket_num);
        if(cmd_vector[0] == "ls" && cmd_vector.size() == 1)
            return show_ls(login_user, does_login, cmd_data, users, socket_num);
        if(cmd_vector[0] == "retr" && cmd_vector.size() == 2)
            return download_file(cmd_vector[1], login_user, does_login, cmd_data, users, special_files, socket_num);
        if(cmd_vector[0] == "help" && cmd_vector.size() == 1)
            return show_help();
        return WRONG_CMD;
    }

    string show_help() {
        ifstream help_file("help.txt");
        string line;
        string help = "";
        while (getline(help_file, line))
            help = help + line + "\n";
        help_file.close();
        return help;
    }

    string download_file(string filename, map<int, string> &login_user, map<int, bool> &does_login, map<int, int> &cmd_data, vector<User> &users, vector<string> &special_files, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;

        string path;
        bool admin;
        int size;
        int i;
        for(i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                path = users[i].get_directory() + "/" + filename;
                admin = users[i].is_admin();
                size = users[i].get_size();
                break;
            }
        }

        if (is_accessible(admin, path, special_files) == false)
            return ACCESS_ERROR;

        ifstream dw_file(path);
        if (dw_file.is_open() == false)
            return ERROR;

        string file = "";
        string line;
        while (getline(dw_file, line))
            file = file + line + "\n";
        dw_file.close();

        if (size < file.length())
            return SIZE_ERROR;

        users[i].decrease_size(file.length());
        ofstream log_file("log.txt", ios_base::app);
        log_file << login_user[socket_num] + " downloaded " + filename + " file. Time: " + get_current_data_time();
        log_file.close();
        send(cmd_data[socket_num], file.c_str(), file.size(), 0);
        return SUCCESSFUL_DOWNLOAD;
    }

    string show_ls(map<int, string> &login_user, map<int, bool> &does_login, map<int, int> &cmd_data, vector<User> &users, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;
        string path;
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                path = users[i].get_directory();
                break;
            }
        }
        struct dirent *entry;
        string ls = "";
        DIR *dir = opendir(path.c_str());
        while ((entry = readdir(dir)) != NULL)
            ls = ls + (entry->d_name) + "\n";
        closedir(dir);
        send(cmd_data[socket_num], ls.c_str(), ls.size(), 0);
        return LS_DONE;
    }

    string change_file_name(string from, string to, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, vector<string> &special_files, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;
        int status;
        bool admin;
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                admin = users[i].is_admin();
                if (is_accessible(admin, users[i].get_directory() + "/" + from, special_files) == false)
                    return ACCESS_ERROR;
                string main_dir(get_current_dir_name());
                chdir(users[i].get_directory().c_str());
                status = rename(from.c_str(), to.c_str());
                chdir(main_dir.c_str());
                if(status == 0) {
                    ofstream log_file("log.txt", ios_base::app);
                    log_file << login_user[socket_num] + " renamed " + users[i].get_directory() + "/" + from + " file to " + to + ". Time: " + get_current_data_time();
                    log_file.close();
                    return SUCCESSFUL_CHANGE;
                }
                return ERROR;
            }
        }
        return ERROR;
    }

    string change_dir(string dirname, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;
        string path;
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                path = users[i].get_directory() + "/" + dirname;
                if(dirname == "") {
                    users[i].set_directory(string(get_current_dir_name()));
                    return SUCCESSFUL_CHANGE;
                }
                if(dirname == "..") {
                    if(string(get_current_dir_name()) != users[i].get_directory()) {
                        users[i].set_directory(users[i].get_directory().substr(0, users[i].get_directory().find_last_of("/")));
                        return SUCCESSFUL_CHANGE;
                    }
                    return ERROR;
                }
                if(is_path_exist(path)) {
                    users[i].set_directory(path);
                    return SUCCESSFUL_CHANGE;
                }
            }
        }
        return ERROR;
    }

    string delete_file(string filename, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, vector<string> &special_files, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;

        string path;
        bool admin;
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                path = users[i].get_directory() + "/" + filename;
                admin = users[i].is_admin();
                break;
            }
        }

        if (is_accessible(admin, path, special_files) == false)
            return ACCESS_ERROR;

        if (remove(path.c_str()) == 0) {
            ofstream log_file("log.txt", ios_base::app);
            log_file << login_user[socket_num] + " deleted " + filename + " file. Time: " + get_current_data_time();
            log_file.close();
            return "250: " + filename + " deleted.";
        }
        return ERROR;
    }

    string delete_dir(string dirname, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;

        string path;
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                path = users[i].get_directory() + "/" + dirname;
                break;
            }
        }
        if(rmdir(path.c_str()) == -1)
            return ERROR;

        ofstream log_file("log.txt", ios_base::app);
        log_file << login_user[socket_num] + " deleted " + dirname + " directory. Time: " + get_current_data_time();
        log_file.close();
        return "250: " + dirname + " deleted.";
    }

    string make_new_file(string filename, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;

        string path;
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                path = users[i].get_directory() + "/" + filename;
                break;
            }
        }

        ifstream new_file(path);
        if (new_file.is_open() == true) {
            new_file.close();
            return ERROR;
        }

        ofstream user_file(path);
        user_file.close();
        ofstream log_file("log.txt", ios_base::app);
        log_file << login_user[socket_num] + " made " + filename + " file. Time: " + get_current_data_time();
        log_file.close();
        return "257: " + filename + " created.";
    }

    string make_new_dir(string dirname, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;

        string path;
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                path = users[i].get_directory() + "/" + dirname;
                break;
            }
        }

        if(mkdir(path.c_str(), 0777) == -1)
            return ERROR;

        ofstream log_file("log.txt", ios_base::app);
        log_file << login_user[socket_num] + " made " + dirname + " directory. Time: " + get_current_data_time();
        log_file.close();
        return "257: " + dirname + " created.";
    }

    string show_current_dir(map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;
        for(int i = 0; i < users.size(); i++)
            if(login_user[socket_num] == users[i].get_user())
                return "257: " + users[i].get_directory();
        return ERROR;
    }

    string quit_user(map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        if(does_login.find(socket_num)->second == false)
            return NEED_LOGIN;
        ofstream log_file("log.txt", ios_base::app);
        log_file << login_user[socket_num] + " logged out. Time: " + get_current_data_time();
        log_file.close();
        for(int i = 0; i < users.size(); i++) {
            if(login_user[socket_num] == users[i].get_user()) {
                users[i].set_directory(get_current_dir_name());
                break;
            }
        }
        login_user.erase(socket_num);
        does_login[socket_num] = false;
        return SUCCESSFUL_QUIT;
    }

    string check_pass(string pass, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        map<int ,string>::iterator it;
        it = login_user.find(socket_num);
        if (it == login_user.end() || does_login[socket_num] == true)
            return BAD_SEQ;
        for(int i = 0; i < users.size(); i++) {
            if(it->second == users[i].get_user() && pass == users[i].get_password()) {
                does_login[socket_num] = true;
                ofstream log_file("log.txt", ios_base::app);
                log_file << login_user[socket_num] + " logged in. Time: " + get_current_data_time();
                log_file.close();
                return SUCCESSFUL_LOGIN;
            }
        }
        return USER_PASSWORD_INVALID_MSG;
    }

    string check_user(string user, map<int, string> &login_user, map<int, bool> &does_login, vector<User> &users, int socket_num) {
        if (does_login[socket_num] == true)
            return BAD_SEQ;
        for(int i = 0; i < users.size(); i++) {
            if(user == users[i].get_user()) {
                login_user[socket_num] = user;
                return USER_OK_MSG;
            }
        }
        return USER_PASSWORD_INVALID_MSG;
    }

    bool is_accessible(bool admin, string file_path, vector<string> &special_files) {
        string current(get_current_dir_name());
        current += "/";
        for(int i = 0; i < special_files.size(); i++)
            if(file_path == (current + special_files[i]))
                return admin;
        return true;
    }
};


class Server {
public:
    void config() {
        string current_dir(get_current_dir_name());
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
            users.push_back(User(user, password, current_dir, (admin == "true"), atoi(size.c_str())));
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
        while (true) {
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
                does_login[new_sock_cmd] = false;

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
                cmd_data[new_sock_cmd] = new_sock_data;

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
                    memset(buffer_cmd, 0, sizeof(buffer_cmd));
                    valread = read(client_sock_cmd[i], buffer_cmd, 2048);
                    if (valread == 0) {
                        for(int i = 0; i < users.size(); i++) {
                            if(login_user[client_sock_cmd[i]] == users[i].get_user()) {
                                users[i].set_directory(get_current_dir_name());
                                break;
                            }
                        }
                        login_user.erase(client_sock_cmd[i]);
                        does_login.erase(client_sock_cmd[i]);
                        close(cmd_data[client_sock_cmd[i]]);
                        for (i = 0; i < cli_size_data; i++)
                            if (client_sock_data[i] == cmd_data[client_sock_cmd[i]])
                                client_sock_data[i] = 0;
                        cmd_data.erase(client_sock_cmd[i]);
                        close(client_sock_cmd[i]);
                        client_sock_cmd[i] = 0;
                    }
                    else {
                        string cmd(buffer_cmd);
                        string result = handler.handle_cmd(cmd, login_user, does_login, cmd_data, users, special_files, client_sock_cmd[i]);
                        memset(buffer_cmd, 0, sizeof(buffer_cmd));
                        strcpy(buffer_cmd, result.c_str());
                        send(client_sock_cmd[i], buffer_cmd, 2048, 0);
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
    map<int, string> login_user;
    map<int, bool> does_login;
    map<int, int> cmd_data;
    struct sockaddr_in addr_cmd;
    struct sockaddr_in addr_data;
    int sockfd_cmd;
    int sockfd_data;
    int *client_sock_cmd;
    int *client_sock_data;
    char buffer_cmd[2048] = {0};
    Handler handler;
};


int main(int argc, char const *argv[]) {
    Server server;
    server.config();
    server.setup();
    server.run();
    return 0;
}