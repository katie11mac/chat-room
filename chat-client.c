/*
 * echo-client.c
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#define BUF_SIZE 4096

void *receive_message(void *data);
void sigint_handler(int sig_num);

int main(int argc, char *argv[])
{
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    int conn_fd;
    // pointer to hold conn_fd in order to pass to thread
    int *id;
    char buf[BUF_SIZE];
    int n;
    int rc;
    pthread_t child;

    // Handle signal when server closes
    signal(SIGQUIT, sigint_handler);

    dest_hostname = argv[1];
    dest_port     = argv[2];

    /* create a socket */
    if((conn_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
    }

    /* client usually doesn't bind, which lets kernel pick a port number */
    /* but we do need to find the IP address of the server */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0){
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    /* connect to the server */
    if(connect(conn_fd, res->ai_addr, res->ai_addrlen) < 0){
        perror("connect");
        exit(2);
    }

    printf("Connected\n");

    id = &conn_fd;

    if((pthread_create(&child, NULL, receive_message, id)) != 0){
        printf("pthread_create error\n");
    }

    /* infinite loop of reading from terminal, sending the data, and printing
     * what we get back */
    while((n = read(0, buf, BUF_SIZE)) > 0) {

        if((send(conn_fd, buf, n, 0)) == -1){
            perror("send");
        }

    }

    printf("Exiting.\n");

    if((close(conn_fd)) == -1){
        perror("close");
    }
}

/*
* infinite loop for receiving messages from the server
*/
void *receive_message(void *data)
{
    char buf[BUF_SIZE];
    int n;
    int conn_fd;

    // Handle signal when server closes
    signal(SIGQUIT, sigint_handler);

    conn_fd = *(int *)data;

    while((n = recv(conn_fd, buf, BUF_SIZE, 0)) != -1){
        printf("%s", buf);
    }
    if(n == -1){
        perror("recv");
    }

    return NULL;
}

void sigint_handler(int sig_num)
{
    exit(0);
}