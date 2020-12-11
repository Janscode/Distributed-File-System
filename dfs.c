#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
//todo: import standard io, networking, and multithreading libraries

//todo: write thread to serve get request
void get(int sock_fd, char * buf, char * username, char * filename){
    //todo ec: report what chunks of file are here, wait for requests
    //todo: send back chunks, use 1028 byte segments labelled with chunk # (1 byte)
        //todo: ec: wait for requests for chunks
    //todo: send back transmission complete byte (invalid chunk #, 5 in this case)
}

void put(int sock_fd, char * buf, char * username, char * filename){
    //todo: receive chunks, use 1028 byte segments labelled with chunk #
        //todo: if new chunk, construct chunk file name with '.' prefix and '.#' suffix
        //todo: receive transmission complete byte to break out of loop, or wait for socket timeout
}

void list(int sock_fd, char * buf, char * username){
    //todo: use ls command with syscall
    //todo: report back file names with chunk #'s format: filename/##filename/##filename/## (numbers are single bytes, no delimiter)
    // send / in lieu of filename to mark the end of transmission
}

//todo: write main thread routine to serve all requests
void * serve(void * connection){
    int sock_fd, bytes;
    sock_fd = *(int *) connection;
    free(connection);
    char buf[1028];
    char username[45];
    char password[45];
    char command[4];
    char filename[40];
    
    bytes = recv(sock_fd, buf, 1028, 0); //practice: limit bytes read so that the string won't overflow the buffers
    if (bytes < 0){
        perror("recv failed");
    }
    //parse socket stream for request type, username, password
    sscanf(buf, "%s %s %s %s", username, password, command, filename); //practice: deal with filenames with spaces
    if (0){
        //todo: check username and password
            //todo: if username is valid but no folder exists, create folder
            //todo: if username is invalid, send back error signal
    }
    else{
        //todo: call appropriate handler, passing request details
        if (!strncmp(command, "put", 3)){
            get(sock_fd, buf, username, filename);
        }
        else if (!strncmp(command, "get", 3)){
            put(sock_fd, buf, username, filename);
        }
        else if (!strncmp(command, "list", 4)){   
            list(sock_fd, buf, username);
        }
    }
    return NULL;
}

//todo: write main
int main(int argc, char ** argv){
    int sock_fd, port, ready, addr_len;
    int *  connection;
    struct sockaddr_in server_addr;
    pthread_t tid;
    fd_set sock_set;
    struct  timeval timeout;
    //todo: read config file
    port = 8080;
    addr_len = sizeof(server_addr);
        //todo: create username password data structure (linked list?) alternativley, just parse through config file each time
            //practice: hash and salt passwords
            //practice: implement a more efficient username lookup
        //todo: parse command line and open appropriate directory
    //create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
    }
    //practice: use setsockopt to make program more robust
    //build server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    //bind socket to port number
    if (bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        perror("bind failed");
    }
    //put socket in passive mode with allowable backlog of 4 connections
    if (listen(sock_fd, 4) < 0){
        perror("listen failed");
    }
    //todo: while loop to listen to socket using select
    while(1){
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_ZERO(&sock_set);
        FD_SET(sock_fd, &sock_set);
        ready = select(sock_fd + 1, &sock_set, NULL, NULL, &timeout);
        if (ready < 0){
            perror("select failed");
        }
        else if (ready){
            connection = malloc(sizeof(int));
            if ((*connection = accept(sock_fd,(struct sockaddr *) &server_addr, (socklen_t *)&addr_len)) < 0){
                perror("accept failed");
            }
            pthread_create(&tid, NULL, serve, (void *) connection);
            pthread_detach(tid);
        }
         //todo: accept connection and spawn new thread
    }
    
    
       
}
    
    
