/*
 * echo-server.c
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#define BACKLOG 10
#define BUF_SIZE 4096 // can we assume a max message size?

void *child_func(void *data);

struct client_info
{
    struct sockaddr_in remote_sa;
    int conn_fd;
}

int main(int argc, char *argv[])
{
    char *listen_port;
    int listen_fd, conn_fd;
    struct addrinfo hints, *res;
    int rc;
    struct sockaddr_in remote_sa;
    socklen_t addrlen;
    struct client_info new_client;

    pthread_t child;

    // hints.ai_canonname = "Unknown";

    listen_port = argv[1];

    /* create a socket */
    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
    }

    /* bind it to a port */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    if((bind(listen_fd, res->ai_addr, res->ai_addrlen)) == -1){
        perror("bind");
    }

    /* start listening */
    if((listen(listen_fd, BACKLOG)) == -1){
        perror("listen");
    }

    /* infinite loop of accepting new connections and handling them */
    while(1) {
        /* accept a new connection (will block until one appears) */
        addrlen = sizeof(remote_sa);
        if((conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen)) == -1){
            perror("conn_fd");
        }

        new_client->remote_sa = remote_sa;
        new_client->conn_fd = conn_fd;
        // Create thread for each child
        pthread_create(&child, NULL, child_func, NULL);


        close(conn_fd);
    }
}

void *child_func(void *data)
{
    int conn_fd;
    char *remote_ip;
    int bytes_received;
    uint16_t remote_port;
    struct sockaddr_in remote_sa;
    char buf[BUF_SIZE];
    char message[BUF_SIZE];
    int i;
    /* announce our communication partner */
    remote_ip = inet_ntoa(remote_sa.sin_addr);
    remote_port = ntohs(remote_sa.sin_port);
    printf("new connection from %s:%d\n", remote_ip, remote_port);

    /* receive and echo data until the other end closes the connection */
    // WE NEED TO CHANGE THIS SO THAT WE CAN HANDLE MULTIPLE CONNECTIONS AT ONCE
    // CLASS NOTES SUGGEST: 
    //      spin off a thread to deal with each incoming connection

    while((bytes_received = recv(conn_fd, buf, BUF_SIZE, 0)) > 0) {

        if(fflush(stdout) != 0){
            perror("fflush");
        }

        snprintf(message, BUF_SIZE, "%s", buf);

        printf("message sent: %s", message);

        /* send it back */
        if((send(conn_fd, message, BUF_SIZE, 0)) == -1){
            perror("send");
        }

        for(i = 0; i < BUF_SIZE; i++){
            message[i] = '\0';
            buf[i] = '\0';
        }

    }
    printf("out of while loop\n");
    return NULL;
}

