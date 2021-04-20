/**
 * 
 *       Network Assignment-6
 * 
 *     *--------------------------------*
 *     *   Server-Client Communication  *
 *     *   using SOCK_STREAM (TCP)      * 
 *     *--------------------------------*
 * 
 *      @authors:  Debajyoti Dasgupta    (debajyotidasgupta6@gmail.com)
 *                 Siba Smarak Panigrahi (sibasmarak.p@gmail.com)
 *      @language: C
 *      @subject:  Computer Networks Lab
 *      @topic:    Socket Programming
 *      @session:  2020-21
 * 
 *      @application: SERVER
 *      @file:        file_server.c
 * 
 *      How to run:
 *      -----------
 *      $ gcc file_server.c -o file_server
 *      $ ./file_server
 */

// Server side C/C++ program to demonstrate Socket programming
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8080
#define MAXLEN 100

//---------------- UTILITY FUNCTIONS ---------------

int open_file(const char *s)
{
    /*
     * Find the path of the requested file
     * if it exists somewhere on the server
     */

    char *path = realpath(s, NULL);

    /*
     * If the file doesn't exist on  the 
     * server then return return -1  for
     * indication file not found
     */

    if (!path)
    {
        printf("\033[1;36mRequested File Not Found\033[0m\n\n");
        return -1;
    }
    else
        printf("Resource requested found at location:\n\
        \033[1;36m%s\033[0m\n\n",
               path);

    /*
     * If the path of the file  is  found
     * Then open the file and return  the
     * File descriptor after opening  the 
     * file in read mode
     */

    int fd = open(path, O_RDONLY, 0666);
    return fd;
}

/**         DRIVER CODE         **/

int main(int argc, char const *argv[])
{
    /*
     * First we need to setup the TCP  socket
     * after which we will bind the socket to
     * the  server  address.  After  that the
     * server will wait until datagram packet
     * arrives from the client
     */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) // ############################################################ < 0 , right?
    {
        // scocket creation failure
        perror("\033[0;31mSocket creation failed!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Reuse the port the socket has been attached to
     * Reuse the address of the server
     */
    int opt = 1; // ##################################################### should the type be socklen_t
    int status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (status)
    {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    /*
     *  Filling server information
     *
     * 1. INADDR_ANY -> shows  that  socket
     *                  will  use   all  the
     *                  packets that arrive
     *                  for  any  IP on this
     *                  machine
     *
     * 2. AF_INET -> Using IPv4 addresses
     *
     * 3. PORT -> Running on port 8080
     */
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /*
     * The bind function assigns a local protocol address to a socket
     */
    status = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (status < 0)
    {
        // error in binding
        perror("\033[0;32mBinding the socket Failed!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    /*
     * listen() the socket as accepting connections
     * limits the number of outstanding connections in the socket's listen queue to the backlog argument (here 10)
     */
    status = listen(server_fd, 10);
    if (status < 0)
    {
        perror("\033[0;32mlisten failure!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    /*
     * setup successful
     * waiting for client request
     */
    printf("\
    \033[0;32m\n\
    *************************************\n\
    *                                   *\n\
    *       WELCOME TO THE SERVER       *\n\
    *                                   *\n\
    *      ALL STARTUP PROCESSES        *\n\
    *      SUCCESSFULLY EXECUTED        *\n\
    *                                   *\n\
    *************************************\n\
    \033[0m\n\
    Waiting for client connection ...\n\n");

    /*
     * server listens continuously
     * unless signal SIGINT (ctrl + C) is provided
     */
    while (1)
    {
        /*
         * The accept() call is used by a server to accept a connection request from a client.
         * When a connection is available, the socket created is ready for use to read data from the process that requested the connection.
         * The call accepts the first connection on its queue of pending connections for the given socket socket.
         */
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0)
        {
            perror("\033[0;32maccept failure!!\033[0m\n");
            exit(EXIT_FAILURE);
        }
        // successfully connected
        printf("\
    \033[0;32mConnection Successfull !!\033[0m\n\n");

        /*
         * read the name of client requested file name
         * if not found in the local directory - close the socket
         * if found - but an empy file ##################################################################################################
         * if found - send chunks of data (with a maximum len of MAXLEN)
         */
        char file[MAXLEN];
        int len = read(new_socket, file, MAXLEN);
        file[len] = '\0';
        printf("File Requested by client: \033[0;35m%s\033[0m\n", file);

        int fd = open_file(file);
        if (fd < 0)
        {
            /*
             * FILE_NOT_FOUND
             * shutdown the socket - stop transmission and reading through the socket
             * close the socket
             */

            status = shutdown(new_socket, SHUT_RDWR);
            if (status < 0)
            {
                perror("\033[0;32mError in terminating the connection!!\033[0m\n");
                exit(EXIT_FAILURE);
            }
            close(new_socket);
        }
        else
        {
            /*
             * create a char array of MAXLEN size
             * continuosly read and send data through the socket
             * once the entire content is read - shutdown the socket and close it
             */

            int is_not_empty = 0;

            char buf[MAXLEN];
            while (1)
            {
                int len = read(fd, buf, MAXLEN);
                buf[len] = '\0';
                if (len > 0)
                {
                    is_not_empty = 1;
                    send(new_socket, buf, strlen(buf), 0);
                }
                else
                {
                    if (!is_not_empty)
                    {
                        /**
                         * For handline the
                         * file  if   it is
                         * empty
                         */
                        sleep(1);
                    }
                    status = shutdown(new_socket, SHUT_RDWR);
                    if (status < 0)
                    {
                        perror("\033[0;32mError in terminating the connection!!\033[0m\n");
                        exit(EXIT_FAILURE);
                    }
                    close(new_socket);
                    break;
                }
            }
        }
    }

    printf("\n\
    SERVER TERMINATED !! \n\
    ");
    close(server_fd);
    return 0;
}
