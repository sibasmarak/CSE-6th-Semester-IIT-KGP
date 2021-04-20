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
 *      @application: CLIENT
 *      @file:        wordclient.c
 * 
 *      How to run:
 *      -----------
 *      $ gcc wordclient.c -o wordclient
 *      $ ./wordclient
 */

/**
 *  Client side implementation of UDP (datagram)
 *  based client-server model
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define MAXLINE 1024

//----------- UTILITY FUNCTIONS ------------

FILE *open_file(const char *file)
{
    /**
     * Open the requested file for 
     * writing  in   the   current 
     * working directory. Once the
     * file is opened successfully
     * return the file pointer
     */

    FILE *fout = fopen(file, "w");
    if (!fout)
    {
        /**
         * If we were unable to open 
         * the  required  file  then 
         * report  it  to  the client
         */
        printf("\033[0;35mERROR: Could not open the required file!! \033[0m\n");
    }
    return fout;
}

/**         DRIVER CODE         **/
int main()
{

    /**
     * For the client side the implementation
     * does not require binding, we just need 
     * to make a socket fill  in  the  server 
     * information, after  that  we can start 
     * sending the  datagrams to  the  server
     */

    /** 
     * Creating socket file descriptor
     */

    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("\033[33;32mSocket creation failed!!\033[0m\n");
        exit(EXIT_FAILURE);
    }

    /** 
     * Filling server information
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

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

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
    \033[0m\n\
    Sending request to server ...\n\n");

    /**
     * The client  program  first  reads the
     * file name from the user ( from  which
     * data needs to be read from the sever)
     * 
     * Then this string is sent in the form 
     * of the datagram to the server. After
     * this  the  client  will  wait for the
     * response from the server and the take 
     * further actions
     * 
     * file -> stores the name of the file 
     *         given as input.
     * 
     * status -> 0 - Some error occured
     *           1 - Successfull query
     * 
     * resp -> contains the response received 
     *         from the server
     * 
     * outfile -> file in which output will 
     *            be written
     */

    int status = 1;
    FILE *fout;
    char file[MAXLINE];
    char resp[MAXLINE];
    char *outfile = "output.txt";

    int sz = sizeof(servaddr);
    printf("Enter the file name: ");
    scanf("%s", file);

    /**
     * Send the request in the datagram
     * Request  will contain a filename
     */

    sendto(sockfd, (const char *)file, strlen(file),
           MSG_CONFIRM, (const struct sockaddr *)&servaddr,
           sz);

    /**
     * Receive the response from the 
     * server and decide for further 
     * process  what actions will be 
     * taken.
     */

    int len = recvfrom(sockfd, (char *)resp, MAXLINE,
                       MSG_WAITALL, (struct sockaddr *)&servaddr,
                       &sz);

    resp[len] = '\0';
    if (strcasecmp(resp, "HELLO") == 0)
    {
        /**
         * If we recieve  "HELLO"  in  response
         * then it means that  the  server  has 
         * found the file we requested  for and 
         * the file is of prober format, so  we
         * open the output file for writing and
         * set the status code to 1.
         */

        fout = open_file(outfile);
        status = 1;
    }
    else if (strcasecmp(resp, "FILE_NOT_FOUND") == 0)
    {
        /**
         * If we receive  a  file  not 
         * found  response  means that
         * the  server  was  unable to 
         * find the file requested for 
         * hence  we  report error and 
         * break out
         */

        printf("\033[0;31m\
        *************************\n\
        *                       *\n\
        *     File Not Found    *\n\
        *                       *\n\
        *************************\n\
        \033[0m\n");

        close(sockfd);
        return 0;
    }
    else if (strcasecmp(resp, "WRONG_FILE_FORMAT") == 0)
    {
        /**
         * If we receive a  wrong file 
         * format response  means that
         * the   server  was  able  to 
         * find the file requested for
         * but the  file  was  not  of 
         * prope r format ( start with 
         * "HELLO" ) hence  we  report 
         * error and break out
         */

        printf("\033[0;31m\
        *************************\n\
        *                       *\n\
        *   Wrong File Format   *\n\
        *                       *\n\
        *************************\n\
        \033[0m\n");

        close(sockfd);
        return 0;
    }
    else
    {
        /**
         * Here  we  handle  the case when 
         * client wasn't able to recognise 
         * the  message  returned from the 
         * server 
         */

        printf("\033[0;31mReturn value not Recognised\033[0m\n\n");
        return 0;
    }

    char word[MAXLINE], msg[MAXLINE];
    int count = 0;

    /**
     * We will run  a loop where in  each
     * iteration  we will request for one
     * word   from  the server.  Once  we 
     * receive the word "END" we can stop 
     * our communication.
     * 
     * for stoping the commnication, we will
     * 
     * 1. close the file descriptor of the
     *    socket.
     * 2. Close the file we were writing in
     * 3. Make the status 0,  since we need 
     *    not iterate any more
     * 
     * count -> maintains count of  the  word 
     *          that we are currently reading
     * 
     * msg -> message that the  client  will
     *        send to the server for quering
     *        the next word
     * 
     * word -> contains the word that we will
     *         receive as a response
     * 
     * In each iteration the client  will send
     * a  request  of  the type "WORDi"  which 
     * will mean that the client is requesting
     * for the ith word. This is done till we
     * get the word "END" as response
     */

    while (status)
    {
        /**
         * Increment the count of the word 
         * to be read
         */

        ++count;
        sz = sizeof(servaddr);

        /**
         * the message will  contain  the  request
         * that the client will sen to the  server
         * so message will contain a word  of  the
         * form "WORDi". Then we send this message
         * to the server
         */

        sprintf(msg, "WORD%d", count);
        printf("Request for  \033[0;31m%s\033[0m\n", msg);
        sendto(sockfd, (const char *)msg, strlen(msg),
               MSG_CONFIRM, (const struct sockaddr *)&servaddr,
               sz);

        /**
         * Receive the response  from  the
         * server. Response  will  contain 
         * the ith word from the file that 
         * the client requested the server
         * to read from.
         */

        int len = recvfrom(sockfd, (char *)word, MAXLINE,
                           MSG_WAITALL, (struct sockaddr *)&servaddr,
                           &sz);

        word[len] = '\0';
        printf("Received \033[0;32m%s\033[0m\n\n", word);

        /**
         * If we recerive the word "END"
         * then  we  need  to  stop  any 
         * further iterations  and  also
         * we need to  close the file in 
         * which  we  were  writing  the 
         * output.
         */

        if (strcasecmp(word, "END") == 0)
        {
            fclose(fout);
            printf("Output Written in file \033[0;31moutput.txt\033[0m\n");
            status = 0;
        }

        /**
         * Write  the  word that we received as
         * response from the server in the file 
         * that is opened for writing
         */

        fprintf(fout, "%s\n", word);
    }

    /**
     * Close   the   socket  file   desctriptor
     * This marks the end of all communications
     */

    close(sockfd);
    return 0;
}
