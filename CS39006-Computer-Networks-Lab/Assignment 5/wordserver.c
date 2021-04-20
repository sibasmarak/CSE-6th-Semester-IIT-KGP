/**
 * 
 *       Network Assignment-5
 * 
 *     *--------------------------------*
 *     *   Server-Client Communication  *
 *     *   using datagrams (UDP)        * 
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
 *      @file:        wordserver.c
 * 
 *      How to run:
 *      -----------
 *      $ gcc wordserver.c -o wordserver
 *      $ ./wordserver
 */

/**
 *  Server side implementation of UDP (datagram)
 *  based client-server model
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAXLINE 1024

//----------- UTILITY FUNCTIONS ------------

FILE *open_file(const char *s)
{
    /** 
     * Find the path of the requested file
     * if it exists somewhere on the server
     */

    char *path = realpath(s, NULL);

    /**
     * If the file doesn't exist on  the 
     * server then return a null pointer
     * indication file not found
     */

    if (!path)
    {
        printf("\033[1;36mRequested File Not Found\033[0m\n\n");
        return NULL;
    }
    else
        printf("Resource requested found at location:\n\
        \033[1;36m%s\033[0m\n\n",
               path);

    /**
     * If the path of the file  is  found
     * Then open the file and return  the
     * File pinter after opening the file
     * in read mode
     */

    FILE *fin = fopen(path, "r");
    return fin;
}

void close_file(FILE *f)
{
    /**
     * Close the file pointer 
     * that  was   previously 
     * opened   for   reading
     */
    fclose(f);
}

/**         DRIVER CODE         **/
int main()
{

    /**
     * First we need to setup the UDP  socket
     * after which we will bind the socket to
     * the  server  address.  After  that the 
     * server will wait until datagram packet 
     * arrives from the client
     */

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    /**
     * Create the socket file descriptor
     */

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("\033[0;31mSocket creation failed!!\033[0m\n");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    /**
     *  Filling server information
     * 
     * 1. INADDR_ANY -> shows  that  socket 
     *                  will  use   all  the 
     *                  datagram that arrive 
     *                  for  any  IP on this 
     *                  machine
     *
     * 2. AF_INET -> Using IPv4 addresses
     * 
     * 3. PORT -> Running on port 8080 
     */

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    /** 
     * Bind the socket with the server address
     */

    if (bind(sockfd, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0)
    {
        perror("\033[0;32mBinding the socket Failed!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    printf("\
    Connection is successfully established !!\n\
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
    Waiting for client request ...\n\n");

    /**
     * Keep listening on the port from the 
     * client unless the  client  requests
     * for  a  valid file. A valid file is 
     * one which exists in the  server  as
     * well  as  the first word is "HELLO"
     * 
     * File name is stored in Char[] file
     * Error message is in Char[] err_msg
     */

    char file[MAXLINE];
    char word[MAXLINE];
    char *err_msg;
    FILE *fin;

    int sz = sizeof(cliaddr);
    int len = recvfrom(sockfd, (char *)file, MAXLINE,
                       MSG_WAITALL, (struct sockaddr *)&cliaddr,
                       &sz);
    file[len] = '\0';
    printf("File Requested by client: \033[0;35m%s\033[0m\n", file);

    fin = open_file(file);
    if (!fin)
    {
        /**
             * send  FILE_NOT_FOUND  message
             * to  the  client  if  the file
             * requested was  not  found  on 
             * the server (i.e. fin is NULL)
             * After this  close  the socket
             * file descriptor
             */
        err_msg = "FILE_NOT_FOUND";
        sendto(sockfd, (const char *)err_msg, strlen(err_msg),
               MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
               sz);

        close(sockfd);
        return 0;
    }
    else
    {
        /**
             * We  read  the first content of the 
             * file to ensure that it starts with 
             * a HELLO.  This is required to make 
             * sure that the file requested is in 
             * the correct format.
             */

        fscanf(fin, "%s", word);
        if (strcasecmp(word, "HELLO") != 0)
        {
            /**
                 * If the requested  file is not in
                 * the  correct   format  then  the 
                 * server will send a error message 
                 * indicating  that the file is not 
                 * in correct format and close  the
                 * socket  file  descriptor.
                 */

            err_msg = "WRONG_FILE_FORMAT";
            sendto(sockfd, (const char *)err_msg, strlen(err_msg),
                   MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
                   sz);

            close(sockfd);
            return 0;
        }
        else
        {
            /**
                 * If the file is in  correct  format 
                 * then send the hello message ( that 
                 * was  read  from  the file ) to the 
                 * client.
                 */

            sendto(sockfd, (const char *)word, strlen(word),
                   MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
                   sz);
        }
    }

    /**
     * Read the message  from the   client
     * which will be of the  format  WORDi
     * which will represent  the ith  word
     * that needs to be read from the file 
     * and sent. 
     * The communication between client  and
     * the server ends when the word END has
     * been read by the user hence after END
     * has  been  sent  by  the  server  the 
     * connection is closed
     */
    while (1)
    {
        fscanf(fin, "%s", word);
        sz = sizeof(cliaddr);

        /**
         * Recieve the  message  requestion
         * for the  WORDi  from  the client 
         * side  and then send the response 
         * with the  word requested back to 
         * the client
         */
        char msg[MAXLINE];
        int len = recvfrom(sockfd, (char *)msg, MAXLINE,
                           MSG_WAITALL, (struct sockaddr *)&cliaddr,
                           &sz);
        msg[len] = '\0';

        /**
         * Assert that the request  in  the datagram
         * is for the ith word,  that is the request 
         * is of the proper type otherwise terminate 
         * connection
         */

        printf("Request for  \033[0;31m%s\033[0m\n", msg);
        assert(msg[0] == 'W' && msg[1] == 'O' && msg[2] == 'R' && msg[3] == 'D');
        printf("Sending \033[0;32m%s\033[0m\n\n", word);

        sendto(sockfd, (const char *)word, strlen(word),
               MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
               sz);

        if (strcasecmp(word, "END") == 0)
            break;
    }

    close(sockfd);
    return 0;
}
