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

#define BUF_SIZE 4096

void *child_func(void *data);

int main(int argc, char *argv[])
{
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    int conn_fd;
    int *id;
    char buf[BUF_SIZE];
    int n;
    int rc;
    // time_t timing;
    // char date[14];
    // struct tm *curr_time;

    pthread_t child;

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

    if((pthread_create(&child, NULL, child_func, id)) != 0){
        printf("pthread_create error\n");
    }

    /* infinite loop of reading from terminal, sending the data, and printing
     * what we get back */
    while((n = read(0, buf, BUF_SIZE)) > 0) {

        if((send(conn_fd, buf, n, 0)) == -1){
            perror("send");
        }

        // if((n = recv(conn_fd, buf, BUF_SIZE, 0)) == -1){
        //     perror("recv");
        // }
        
        // printf("%s", buf);

        // //Print out time
        // if((timing = time(NULL)) == (time_t)-1){
        //     perror("time");
        // }
        // curr_time = localtime(&timing);
        // strftime(date, BUF_SIZE, "%H:%M:%S", curr_time);
        
        // printf("%s: ",date);

        // printf("%s",buf);
    }

    printf("CLOSING CONNECTION\n");
    if((close(conn_fd)) == -1){
        perror("close");
    }
}

void *child_func(void *data)
{
    char buf[BUF_SIZE];
    int n;
    int conn_fd;

    conn_fd = *(int *)data;

    while((n = recv(conn_fd, buf, BUF_SIZE, 0)) != -1){
        printf("%s", buf);
    }
    if(n == -1){
        perror("recv");
    }

    return NULL;
}
