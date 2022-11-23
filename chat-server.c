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

#define BACKLOG 10
#define BUF_SIZE 4096 // can we assume a max message size?

int main(int argc, char *argv[])
{
    char *listen_port;
    int listen_fd, conn_fd;
    struct addrinfo hints, *res;
    int rc;
    struct sockaddr_in remote_sa;
    uint16_t remote_port;
    socklen_t addrlen;
    char *remote_ip;
    char buf[BUF_SIZE];
    int bytes_received;
    //buffer to compare to /nick
    char is_nick[6];
    char *new_nick;
    char message[BUF_SIZE];

    hints.ai_canonname = "Unknown";

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

        /* announce our communication partner */
        remote_ip = inet_ntoa(remote_sa.sin_addr);
        remote_port = ntohs(remote_sa.sin_port);
        printf("new connection from %s:%d\n", remote_ip, remote_port);

        /* receive and echo data until the other end closes the connection */
        // WE NEED TO CHANGE THIS SO THAT WE CAN HANDLE MULTIPLE CONNECTIONS AT ONCE
        // CLASS NOTES SUGGEST: 
        //      fork off a new process to deal with each incoming connection OR
        //      spin off a thread to deal with each incoming connection

        while((bytes_received = recv(conn_fd, buf, BUF_SIZE, 0)) > 0) {
    
            if(fflush(stdout) != 0){
                perror("fflush");
            }

            strncpy(is_nick, buf, 5);

            // i don't fully understand why we check the nick in the server and not the client...
            if(strcmp(is_nick, "/nick") == 0){
                
                new_nick = strtok(buf + 6, "\n");
                // how do we broadcast this message to all clients?
                snprintf(message, BUF_SIZE, "User %s (%s:%d) is now known as %s.\n", hints.ai_canonname, remote_ip, remote_port, new_nick);
        
                if((puts(message) < 0)){ //he used puts so i used puts- but i belive printf would also work?
                    perror("puts");
                }

                /* send it back */
                if((send(conn_fd, message, BUF_SIZE, 0)) == -1){
                    perror("send");
                }

                hints.ai_canonname = new_nick; 
                //the line above is happening before send and snprintf - 
                //how do we fix this? the only thing i can think of is make it a critical section, but that's used with multiple threads and i don't think that's applicable here

            }
            else{

                snprintf(message, BUF_SIZE+sizeof(hints.ai_canonname), "%s: %s\n", hints.ai_canonname, buf);

                /* send it back */
                if((send(conn_fd, message, BUF_SIZE+sizeof(hints.ai_canonname), 0)) == -1){
                    perror("send");
                }
            }

        }
        printf("\n");

        close(conn_fd);
    }
}
