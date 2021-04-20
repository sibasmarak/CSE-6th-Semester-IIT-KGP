/**
 * 
 *       Network Assignment-8
 * 
 *     *------------------------------------*
 *     *   PEER TO PEER CHAT APPLICATION    * 
 *     *------------------------------------*
 * 
 *      @authors:  Debajyoti Dasgupta    (debajyotidasgupta6@gmail.com)
 *                 Siba Smarak Panigrahi (sibasmarak.p@gmail.com)
 *      @language: C
 *      @subject:  Computer Networks Lab
 *      @topic:    Socket Programming
 *      @session:  2020-21
 * 
 *      @application: CHAT SERVER
 *      @file:        chat_server.cpp
 * 
 *      How to compile:
 *      ---------------
 *      $ make
 *
 *      How to run:
 *      -----------
 *      $ make run
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <bits/stdc++.h>
using namespace std;

#define N_USER 6
#define MSG_MAX 1000000
#define MAX_TIMEOUT 120

time_t start;

double cur_time()
{
    return time(NULL) - start;
}

//----------------- DATA STRUCTURES ------------------

struct user
{
    string name;
    string ip;
    int port;

    user() {}
    user(string _name, string _ip, int _port) : name(_name), ip(_ip), port(_port) {}
};

map<string, user *> user_to_info;
map<string, string> color;
map<int, double> last_time;

//----------------- UTILITY FUNCTIONS ------------------

vector<user> get_user_info();
void print_user_info(vector<user> &);

int main()
{
    start = time(NULL);

    vector<user> user_info = get_user_info();
    print_user_info(user_info);

    string name;
    cout << "Enter user name : ";
    cin >> name;

    if (user_to_info.find(name) == user_to_info.end())
    {
        cout << "\033[0;35mNot a valid user name...make sure to [Select user from table / All Caps]\033[0m" << endl;
        exit(1);
    }

    user *current_user = user_to_info[name];

    /*
     * First we need to setup the TCP  socket
     * after which we will bind the socket to
     * the  server  address.  After  that the
     * server will wait until datagram packet
     * arrives from the client
     */

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        // socket creation failure
        perror("\033[0;31mSocket creation failed!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Reuse the port the socket has been attached to
     * Reuse the address of the server
     */
    int opt = 1;
    int status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (status)
    {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    /*
     *  Filling server information
     *
     * 1. ip  --------> shows   that  socket
     *                  will  use   all  the
     *                  packets  that arrive
     *                  for  the provided IP 
     *                  on the machine
     *
     * 2. AF_INET -> Using IPv4 addresses
     *
     * 3. PORT -> Running on which port 
     */
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(current_user->ip.c_str());
    address.sin_port = htons(current_user->port);

    /*
     * The bind function assigns a local protocol address to a socket
     */
    status = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (status < 0)
    {
        // error in binding
        perror("\033[0;31mBinding the socket Failed!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    last_time[server_fd] = cur_time();

    /*
     * listen() the socket as accepting connections
     * limits the number of outstanding connections in the socket's listen queue to the backlog argument (here 10)
     */
    status = listen(server_fd, 10);
    if (status < 0)
    {
        perror("\033[0;31mlisten failure!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    vector<pair<int, string>> fds;
    fds.push_back({STDIN_FILENO, "++"});
    fds.push_back({server_fd, current_user->name});

    fd_set readfds;

    int result;

    cout << "\
    \033[0;32m\n\
    \033[30;48;2;102;255;0m*************************************\033[0m\n\
    \033[30;48;2;255;162;0m*                                   *\033[0m\n\
    \033[30;48;2;102;255;0m*      WELCOME TO THE P2P APP       *\033[0m\n\
    \033[30;48;2;255;162;0m*                                   *\033[0m\n\
    \033[30;48;2;102;255;0m*      ALL STARTUP PROCESSES        *\033[0m\n\
    \033[30;48;2;255;162;0m*      SUCCESSFULLY EXECUTED        *\033[0m\n\
    \033[30;48;2;102;255;0m*                                   *\033[0m\n\
    \033[30;48;2;255;162;0m*************************************\033[0m\n\
    \033[0m\n\
    Enter message of the form  [peer/message]\n\n"
         << endl;

    while (1)
    {
        // for (auto it = fds.begin(); it != fds.end(); ++it)
        // {
        //     cout << "\033[30;48;2;" << color[it->second] << ";0m"
        //          << "[ TIME = " << last_time[it->first] << " ] [ NAME = "<<it->second << " ]\033[0m" << endl;
        // }

        FD_ZERO(&readfds);
        for (int i = fds.size() - 1; i >= 2; i--)
        {
            if (cur_time() - last_time[fds[i].first] > MAX_TIMEOUT)
            {
                cout << "\033[0;35mSocket Connection With " << fds[i].second << " Timedout ... !!\033[0m" << endl;
                close(fds[i].first);
                last_time.erase(fds[i].first);
                fds.erase(fds.begin() + i);
            }
        }

        do
        {
            for (auto &i : fds)
                FD_SET(i.first, &readfds);

            struct timeval timeout;
            timeout.tv_sec = MAX_TIMEOUT;
            timeout.tv_usec = 0;
            result = select(max_element(fds.begin(), fds.end())->first + 1, &readfds, NULL, NULL, &timeout);
        } while (result == -1 && errno == EINTR);

        if (!result)
        {
            cout << "\033[0;35mConnection Timed Out ...!!!\n\033[0m" << endl;
            exit(errno);
        }

        if (FD_ISSET(server_fd, &readfds))
        {
            struct sockaddr_in clientaddr;
            int clientaddrlen = sizeof(clientaddr);
            int new_socket = accept(server_fd, (struct sockaddr *)&clientaddr, (socklen_t *)&clientaddrlen);

            if (new_socket < 0)
                perror("\033[0;31Socket accept failed..!!\033[0m\n");

            int port = ntohs(clientaddr.sin_port);
            auto _user = find_if(user_info.begin(), user_info.end(), [&](user &u) { return u.port == port; });

            if (_user == user_info.end())
            {
                cout << "Not a Peer...!!" << endl;

                /* Ignore any request */
                continue;
            }

            auto _fd = find_if(fds.begin(), fds.end(), [&](pair<int, string> &p) { return p.second == _user->name; });

            if (_fd == fds.end())
                fds.push_back({new_socket, _user->name});
            else
                _fd->first = new_socket;

            last_time[new_socket] = cur_time();
        }
        else
        {
            if (FD_ISSET(STDIN_FILENO, &readfds))
            {
                char buffer[MSG_MAX];
                string peer;
                string message;

                bzero(buffer, sizeof(buffer));
                int len = read(STDIN_FILENO, buffer, sizeof(message));
                buffer[len] = '\0';

                peer = strtok(buffer, "/");
                message = strtok(NULL, "/");

                char *mtok;
                while (mtok = strtok(NULL, "/"))
                    message += "/" + string(mtok);

                if (user_to_info.find(peer) == user_to_info.end())
                {
                    cout << "Peer " << peer << " unavailable in User Info List" << endl;
                    continue;
                }

                user *peer_user = user_to_info[peer];
                auto _fd = find_if(fds.begin(), fds.end(), [&](pair<int, string> &p) { return p.second == peer; });

                if (_fd == fds.end())
                {
                    int client;
                    if ((client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        perror("\n Socket creation error \n");
                        exit(errno);
                    }

                    int opt = 1;
                    int status = setsockopt(client, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

                    bzero(&address, sizeof(address));
                    address.sin_family = AF_INET;
                    address.sin_addr.s_addr = inet_addr(current_user->ip.c_str());
                    address.sin_port = htons(current_user->port);
                    /*
                     * The bind function assigns a local protocol address to a socket
                     */
                    status = bind(client, (struct sockaddr *)&address, sizeof(address));
                    if (status < 0)
                    {
                        // error in binding
                        perror("\033[0;31mBinding the socket Failed!!\033[0m\n");
                        exit(EXIT_FAILURE);
                    }

                    struct sockaddr_in client_addr;
                    client_addr.sin_family = AF_INET;
                    client_addr.sin_addr.s_addr = inet_addr(peer_user->ip.c_str());
                    client_addr.sin_port = htons(peer_user->port);

                    if (connect(client, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
                    {
                        printf("\nConnection Failed \n");
                        continue;
                    }
                    fds.push_back({client, peer_user->name});
                    _fd = prev(fds.end());
                }
                int ret = write(_fd->first, message.c_str(), message.length());
                if (ret < 0)
                    cout << "Error in sending message..!!" << endl;

                last_time[_fd->first] = cur_time();
            }
            else
            {
                for (auto &fd : fds)
                {
                    if (fd.second == current_user->name)
                        continue;
                    if (FD_ISSET(fd.first, &readfds))
                    {
                        char message[MSG_MAX];
                        int len = read(fd.first, message, MSG_MAX);
                        if (len < 0)
                            cout << "Error in retrieving message" << endl;
                        else if (len == 0)
                        {
                            close(fd.first);
                            last_time.erase(fd.first);
                            fds.erase(find(fds.begin(), fds.end(), fd));
                        }
                        else
                        {
                            message[len] = '\0';
                            cout << flush << "\033[30;48;2;" << color[fd.second] << ";0mMessage from " << fd.second << " :\033[0m " << message << endl;

                            last_time[fd.first] = cur_time();
                        }

                        break;
                    }
                }
            }
        }
    }
}

vector<user> get_user_info()
{
    /**
     * A - 10.0.10.1 - 8001
     * B - 10.0.10.2 - 8002
     * C - 10.0.10.3 - 8003
     * D - 10.0.10.4 - 8004
     * E - 10.0.10.5 - 8005
     * F - 10.0.10.6 - 8006
     */

    vector<user> user_info;

    user_info.emplace_back(user("A", "127.0.0.1", 8001));
    user_info.emplace_back(user("B", "127.0.0.1", 8002));
    user_info.emplace_back(user("C", "127.0.0.1", 8003));
    user_info.emplace_back(user("D", "127.0.0.1", 8004));
    user_info.emplace_back(user("E", "127.0.0.1", 8005));
    user_info.emplace_back(user("F", "127.0.0.1", 8006));

    user_to_info["A"] = &user_info[0];
    user_to_info["B"] = &user_info[1];
    user_to_info["C"] = &user_info[2];
    user_to_info["D"] = &user_info[3];
    user_to_info["E"] = &user_info[4];
    user_to_info["F"] = &user_info[5];

    color["A"] = "255;162";
    color["B"] = "102;255";
    color["C"] = "255;162";
    color["D"] = "102;255";
    color["E"] = "255;162";
    color["F"] = "102;255";

    return user_info;
}

void print_user_info(vector<user> &users)
{
    /**
     * -------------------------------
     * |  USER | IP ADDRESS | PORT   |   
     * -------------------------------
     */

    int suser = 10;
    int sip = 16;
    int sport = 10;

    cout << "\
    ----------------------------------------\n\
    |   USER   |   IP ADDRESS   |   PORT   |\n\
    ----------------------------------------\
    " << endl;

    for (auto &i : users)
    {

        string output = "    ";
        output += "|";

        output += "\033[30;48;2;" + color[i.name] + ";0m";
        output += string((suser - i.name.length()) / 2, ' ');
        output += i.name;
        output += string((suser - i.name.length() + 1) / 2, ' ');
        output += "\033[0m";

        output += "|";

        output += "\033[30;48;2;" + color[i.name] + ";0m";
        output += string((sip - i.ip.length()) / 2, ' ');
        output += i.ip;
        output += string((sip - i.ip.length() + 1) / 2, ' ');
        output += "\033[0m";

        output += "|";

        output += "\033[30;48;2;" + color[i.name] + ";0m";
        output += string((sport - to_string(i.port).length()) / 2, ' ');
        output += to_string(i.port);
        output += string((sport - to_string(i.port).length() + 1) / 2, ' ');
        output += "\033[0m";
        output += "|";

        cout << output << endl;
    }

    cout << "\
    ----------------------------------------\n"
         << endl;
}