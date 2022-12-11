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
#define BUF_SIZE 4096

static struct client_info *first_client = NULL;
static struct client_info *last_client = NULL;
pthread_mutex_t mutex;

void *client_thread_func(void *data);
void send_to_all_clients(char *message);


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
    struct client_info *new_client;
    
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
        
        // create a new client struct
        new_client = malloc(sizeof(struct client_info));
        new_client->remote_sa = remote_sa;
        new_client->conn_fd = conn_fd;
        new_client->name = "unknown";
        new_client->next_client = NULL;
        new_client->prev_client = last_client;
        
        // lock mutex since added a new client to the linked list
        if(pthread_mutex_lock(&mutex) != 0){
            printf("pthread_mutex_lock error\n");
        }

        //Updating the linked list
        if(first_client == NULL){
            first_client = new_client;
        }
        else{
            last_client->next_client = new_client;
        }

        last_client = new_client;
    
        pthread_mutex_unlock(&mutex);

        // create thread for each new client
        pthread_create(&child, NULL, client_thread_func, new_client);
    }
   
}


void *client_thread_func(void *data)
{
    int conn_fd;
    char *remote_ip;
    int bytes_received;
    uint16_t remote_port;
    struct sockaddr_in remote_sa;
    char buf[BUF_SIZE];
    char message[BUF_SIZE];
    // counter for clearing the buf
    int i;
    // variables relevant for time formatting
    time_t timing;
    char date[14];
    struct tm *curr_time;
    // variables relevant for checking if /nick provided
    char nick_str[7];
    char *nick_value;

    struct client_info *new_client = (struct client_info *) data;

    conn_fd = new_client->conn_fd;
    remote_sa = new_client->remote_sa;

    /* announce our communication partner */
    remote_ip = inet_ntoa(remote_sa.sin_addr);
    remote_port = ntohs(remote_sa.sin_port);
    printf("new connection from %s:%d\n", remote_ip, remote_port);

    /* receive and echo data until the other end closes the connection */
    while((bytes_received = recv(conn_fd, buf, BUF_SIZE, 0)) > 0) {

        if(fflush(stdout) != 0){
            perror("fflush");
        }

        // get the time
        if((timing = time(NULL)) == (time_t)-1){
            perror("time");
        }
        curr_time = localtime(&timing);
        strftime(date, BUF_SIZE, "%H:%M:%S", curr_time);

        // check if client changes name
        strncpy(nick_str, buf, 6);
        if(strcmp(nick_str, "/nick ") == 0){

            nick_value = strtok(buf, "\n");
            strtok(nick_value, " ");
            nick_value = strtok(NULL, " ");
           
            snprintf(message, BUF_SIZE, "%s: User %s (%s:%d) is now known as %s.\n", date, new_client->name, remote_ip, remote_port, nick_value);
            printf("User %s (%s:%d) is now known as %s.\n", new_client->name, remote_ip, remote_port, nick_value);
            
            new_client->name = strdup(nick_value);
        }
        //client has send a message
        else{
            snprintf(message, BUF_SIZE + sizeof(date) + sizeof(new_client->name), "%s: %s: %s", date, new_client->name, buf);
        }
        
        send_to_all_clients(message);
       
        // clear the message and buf array
        for(i = 0; i < BUF_SIZE; i++){
            message[i] = '\0';
            buf[i] = '\0';
        }

    }

    // client has terminated connection from server 
    printf("Lost connection from %s.\n", new_client->name);
    snprintf(message, BUF_SIZE, "%s: User %s (%s:%d) has disconnected.\n", date, new_client->name, remote_ip, remote_port);
    send_to_all_clients(message);

    //deleting a client from the linked list (need to lock)
    pthread_mutex_lock(&mutex);
    // CASE 0: deleting only client
    if((first_client == new_client) && (last_client == new_client)){
        first_client = NULL;
        last_client = NULL;
    }
    // CASE 1: deleting first client
    else if(first_client == new_client){
        first_client = new_client->next_client;
        new_client->next_client->prev_client = NULL;
    }
    // CASE 2: deleting last client
    else if(last_client == new_client){
        last_client = new_client->prev_client;
        new_client->prev_client->next_client = NULL;
    }
    // CASE 3: deleting a middle client in LL
    else {
        new_client->prev_client->next_client = new_client->next_client;
        new_client->next_client->prev_client = new_client->prev_client;
    }

    // need to free the new_client since it was malloc-ed
    free(new_client);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/* 
* Iterate through all clients in the linked list and send them a message
*/
void send_to_all_clients(char *message)
{
    struct client_info *curr_client;
    //iterate through all clients and send messages back
    curr_client = first_client;
    while(curr_client != NULL){

        pthread_mutex_lock(&mutex);

        /* send it back */
        if((send(curr_client->conn_fd, message, BUF_SIZE, 0)) == -1){
            perror("send");
        }

        curr_client = curr_client->next_client;
        pthread_mutex_unlock(&mutex);
    }
}
