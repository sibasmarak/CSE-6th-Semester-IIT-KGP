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
 *      @application: CLIENT
 *      @file:        file_client.c
 * 
 *      How to run:
 *      -----------
 *      $ gcc file_client.c -o client
 *      $ ./client
 */

// Client side C/C++ program to demonstrate Socket programming
#include <poll.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAXLEN 100

//------------------- UTILITY FUNCTIONS -------------

int open_file(const char *s)
{

    /*
     * If the path of the file  is  found
     * Then open the file and return  the
     * File descriptor after opening  the 
     * file in read mode else create  the
     * file  and  then  return  the  file 
     * pointer 
     */

    int fd = open(s, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    return fd;
}

int check(char c)
{
    return c == ',' || c == ';' || c == ':' || c == '.' || isspace(c);
}

int get_words(const char *buf)
{
    /*
     * obtain the number of words in char array buf
     * consider all given punctuations as separator
     */
    int n = strlen(buf);
    int words = 0, cur = 0;
    for (int i = 0; i < n; i++)
    {
        if (check(buf[i]))
        {
            words += (cur > 0 ? 1 : 0);
            cur = 0;
        }
        else
            cur++;
    }
    return words;
}

/**         DRIVER CODE         **/

int main(int argc, char const *argv[])
{
    /*
     * create the socket
     * assign the necessary socket information for connection request
     * convert IPv4 address from text to binary
     * send a connection request to the server
     */
    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    /**
     * Set  time  out  for 
     * receive if the file
     * is empty. The server 
     * will send NULL value
     * for  empty  hence we 
     * need  to  handle the 
     * time out interval
     */

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    /*
     * inet_pton() used to convert IPv4 address from text to binary
     * serv_addr is complete - send a connection request with connect()
     */
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    // connection successfully established
    printf("\
    Connection is successfully established !!\n\
    \033[0;32m\n\
    *************************************\n\
    *                                   *\n\
    *       WELCOME TO THE CLIENT       *\n\
    *                                   *\n\
    *      ALL STARTUP PROCESSES        *\n\
    *      SUCCESSFULLY EXECUTED        *\n\
    *                                   *\n\
    *************************************\n\
    \033[0m\n\n");

    /*
     * read the file name
     * send the request to server the file name
     */
    char file[MAXLEN];
    printf("Enter the file name: ");
    scanf("%s", file);
    printf("\n\033[0;32mSending request to server ...\033[0m\n\n");
    send(sock, file, strlen(file), 0);

    // struct pollfd pfd[] = {{sock, POLLIN, 0}};
    // poll(pfd, 1, 5);
    // printf("%d\n", pfd[0].revents);

    int l = sizeof(serv_addr);

    /*
         * File successfully found in the server
         * write the obtained data into an output file
         * store the number of words read with the aid of get_words()
         */
    printf("\nReceiving data for \033[0;35m%s\033[0m\n", file);
    int fd = open_file("output.txt");
    int size_of_file = 0;
    int words_in_file = 0;
    char buffer[MAXLEN];
    int is_file_present = 0;

    if (fd < 0)
    {
        printf("Couldn't open the output file\n\n");
        return 0;
    }

    // if the buf received is null
    // file is empty
    // close the connection - set the flag to be 1 (1 means file empty)
    // else flag remains 0 - and reads the data
    // check what is currently happening fo file not found and emmpty file

    while (1)
    {
        // The chunk size used by the server is not known to the client ###################################################################
        int len = recv(sock, buffer, MAXLEN, 0);
        if (!is_file_present)
            if (!len)
            {
                printf("\033[1;36mERR 01: File Not Found\033[0m\n\n");
                shutdown(sock, O_RDWR);
                close(sock);
                return 0;
            }
            else if (len == -1)
            {
                printf("\033[1;36mEmpty File\033[0m\n\n");
                break;
            }

        if (!len && is_file_present)
            break;

        buffer[len] = '\0';
        is_file_present = 1;
        size_of_file += len;
        words_in_file += get_words((const char *)buffer);
        write(fd, buffer, strlen(buffer));
    }

    /*
         * if the len of data received is zero
         * close the file descriptor of output file
         * shutdown the socket
         * close the socket
         */
    close(fd);
    shutdown(sock, O_RDWR);
    close(sock);

    int lend = strlen(buffer);
    if (lend > 0 && !check(buffer[lend - 1]))
        words_in_file++;

    /*
         * Print the size of the file
         * Print the number of words in the file
         */
    printf("\n\
        \033[0;32m> File Transfer is Successful!! <\033[0m\n\n\
        ************************************\n\
            \033[0;35mSize  of  file\033[0m  = \033[1;36m%d bytes\033[0m\n\
            \033[0;35mNumber of words\033[0m = \033[1;36m%d\033[0m\n\
        ************************************\n\n",
           size_of_file, words_in_file);

    printf("Output Written in file \033[0;31moutput.txt\033[0m\n");

    return 0;
}
