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
#include <sys/wait.h>

#define BACKLOG 10
#define BUF_SIZE 4096 // can we assume a max message size?

static struct client_info *first_client = NULL;
static struct client_info *last_client = NULL;

void *child_func(void *data);
pthread_mutex_t mutex;

struct client_info
{
    struct sockaddr_in remote_sa;
    int conn_fd;
    char *name;
    struct client_info *next_client;
    struct client_info *prev_client;
};

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
        
        printf("conn_fd 1: %d\n", conn_fd);
        //locking mutex while creating/editing the LL
        if(pthread_mutex_lock(&mutex) != 0){
            printf("pthread_mutex_lock error\n");
        }

        //Creating a client struct
        new_client.remote_sa = remote_sa;
        new_client.conn_fd = conn_fd;
        new_client.name = "Unknown";
        new_client.next_client = NULL;
        new_client.prev_client = last_client;
        printf("conn_fd 2 (in new_client): %d\n", new_client.conn_fd);


        //Updating the linked list
        if(first_client == NULL){
            printf("RUNNING HERE\n");
            first_client = &new_client;
        }
        else{
            (last_client->next_client) = &new_client;
        }

        printf("creating client struct successfully\n");
        last_client = &new_client;

        //unlocking mutex
        if(pthread_mutex_unlock(&mutex) != 0){
            printf("pthread_mutex_lock error\n");
        }

        // Create thread for each child
        if((pthread_create(&child, NULL, child_func, &new_client)) != 0){
            printf("pthread_create error\n");
        }

        // close(conn_fd);
    }
    printf("out of the connecting while loop\n");
    
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

    struct client_info *new_client = (struct client_info *) data;

    conn_fd = new_client->conn_fd;
    remote_sa = new_client->remote_sa;
    printf("conn_fd 3 (in new_client function): %d\n", conn_fd);

    /* announce our communication partner */
    remote_ip = inet_ntoa(remote_sa.sin_addr);
    remote_port = ntohs(remote_sa.sin_port);
    printf("new connection from %s:%d\n", remote_ip, remote_port);

    /* receive and echo data until the other end closes the connection */

    // pthread_mutex_lock(&mutex);
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
    perror("recv");
    printf("bytes recieved %d\n", bytes_received);

    // deleting a client from the LL (need to lock)
    pthread_mutex_lock(&mutex);

    printf("terminating connection\n");
    // deleting only client
    if((first_client == new_client) && (last_client == new_client)){
        first_client = NULL;
        last_client = NULL;
    }
    // deleting first client
    else if(first_client == new_client){
        first_client = new_client->next_client;
        (new_client->next_client)->prev_client = NULL;
    }
    // deleting last client
    else if(last_client == new_client){
        last_client = new_client->prev_client;
        (new_client->prev_client)->next_client = NULL;
    }
    // deleting a middle client in LL
    else {
        (new_client->prev_client)->next_client = new_client->next_client;
        (new_client->next_client)->prev_client = new_client->prev_client;
    }
    
    pthread_mutex_unlock(&mutex);



    printf("out of the recieving while loop\n");
    // pthread_mutex_unlock(&mutex);
    return NULL;
}
