#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h> 
//todo: import standard io, networking, and multithreading libraries
char * dir;
//practice: use named semaphores to lock off user directories to prevent race conditions

void checkdir(char * username){ //todo: check if username directory exists, if it doesn't, create it
    
}

int checkcreds(char * candidate_username, char* candidate_password){
    FILE * config_fd;
    char line[128]; //practice: handle longer lines / make sure config file is properly formated
    char username[45];
    char password[45];
    config_fd = fopen("dfs.config", "r");
    int authenticated = 0;
    
    if (config_fd < 0){
        perror("fopen failed on config");
    }
    else{
        while(fgets(line, 128, config_fd) != NULL){
            sscanf(line, "%s %s", username, password);
            if (!strcmp(username, candidate_username)){
                checkdir(username);
                if (!strcmp(password, candidate_password)){
                    authenticated = 1;
                    break;
                }
            }
        }
        fclose(config_fd);
    }
    return authenticated;
}

void get(int sock_fd, char * buf, char * username, char * filename){
    char chunkname[150];
    snprintf(chunkname, sizeof(chunkname), "%s%s/.%s.#", dir, username, filename); //practice: construct chunkname more intelligently
    //todo ec: report what chunks of file are here, wait for requests
    
    //todo: respond to each chunk request
        //todo: receive chunk #
        //todo: build name
        //todo: open file
        //send chunk
        //todo: close file
    //practice: make all sockets able to timeout after a while
}

void put(int sock_fd, char * buf, char * username, char * filename){
    char chunkname[150];
    char chunk1;
    char chunk2;
    int namesize;
    namesize = snprintf(chunkname, sizeof(chunkname), "%s%s/.%s.#", dir, username, filename); //practice: construct chunkname more intelligently

    if (recv(sock_fd, buf, 1028, 0) < 0){
        printf("recv failed recieving chunk numbers");
    }
    chunk1 = buf[0];
    chunk2 = buf[1];
    chunkname[namesize - 1] = chunk1;
    //todo: receive chunk #s
    //todo: request each chunk
        //todo: build name
        //todo: open file
        //todo: send chunk #
        //todo: close file
    //practice: make all sockets able to timeout after a while
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
    char * inval_cred_message = "Invalid Username/Password. Please try again.";
    char * ok_message = "ok";
    //receive request  
    bytes = recv(sock_fd, buf, 1028, 0); //practice: limit bytes read so that the string won't overflow the buffers
    if (bytes < 0){
        perror("recv failed");
    }
    //parse socket stream for request type, username, password
    sscanf(buf, "%s %s %s %s", username, password, command, filename); //practice: deal with filenames with spaces
    if (checkcreds(username, password) == 0){
        //send invalid credential message
        if (send(sock_fd, inval_cred_message, sizeof(inval_cred_message), 0) < 0){
            perror("send failed on invalid credential message");
        }
    }
    else{
        //send ok message
        if (send(sock_fd, ok_message, sizeof(ok_message), 0) < 0){
            perror("send failed on ok message");
        }
        //call appropriate handler, passing details
        if (!strncmp(command, "put", 3)){
            put(sock_fd, buf, username, filename);
        }
        else if (!strncmp(command, "get", 3)){
            get(sock_fd, buf, username, filename);
        }
        else if (!strncmp(command, "list", 4)){   
            list(sock_fd, buf, username);
        }
    }
    close(sock_fd);
    return NULL;
}

//todo: write main
int main(int argc, char ** argv){
    int sock_fd, portno, ready, addr_len;
    int *  connection;
    struct sockaddr_in server_addr;
    pthread_t tid;
    fd_set sock_set;
    struct  timeval timeout;
    
    addr_len = sizeof(server_addr);
    //store command line args todo: mkdir if dir dne
    if (argc != 3){
        printf("Usage: dfs <dir> <config>\n");
        exit(1);
    }
    dir = argv[1]; //practice: if dir does not end with /, append it
    portno = atoi(argv[2]);
    //create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
    }
    //practice: use setsockopt to make program more robust
    //build server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);
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
            printf("1\n");
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
    
    
