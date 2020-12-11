#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#define NUM_SERVERS 4 //practice: make this work for variable number of servers
//todo: import standard io, networking, (maybe) multithreading, and hashing

//todo: write put request
void put(char * filename, char * username, char * password, struct sockaddr_in * server_addrs[NUM_SERVERS]){
    FILE * source_fd;
    FILE * chunk_fd;
    unsigned char digest[MD5_DIGEST_LENGTH];
    char chunkname[43];
    char buf[1028];
    MD5_CTX mdcontext;
    int bytes, bytes_left, filesize, chunksize, hash, sock_fd, partition;
    int partitions[4][2] = {{1,2},{2,3},{3,4},{4,1}}; //practice: come up with partitions dynamically

    
    filesize = 0;
    source_fd = fopen(filename, "r");
    /*error if the file isn't found*/
    if (source_fd == NULL){
        printf("File not found!\n");
    } 
    else{
        /* read file in chunks  */
        MD5_Init(&mdcontext);
        while((bytes = fread(buf, sizeof(char), 1028, source_fd))){
            /* progressivley count file length */
            filesize += bytes;
            MD5_Update(&mdcontext, buf, bytes);
            //todo: progressivley update state of md5 object
        }   
        MD5_Final(digest, &mdcontext);
        hash = (*(long long int *)digest) % NUM_SERVERS;
        chunksize = (filesize - (filesize %  NUM_SERVERS))/ NUM_SERVERS;
        fseek(source_fd, 0, SEEK_SET); //seek to start of file
        strcpy(chunkname+1,filename);
        chunkname[0] = '.';
        chunkname[strlen(filename) + 1] = '.';
        chunkname[strlen(filename) + 3] = '\0';
        for (int i = 1; i <=  NUM_SERVERS; i++){
            chunkname[strlen(filename) + 2] = i + '0';
            printf("%s\n",chunkname);
            chunk_fd = fopen(chunkname, "w");
            bytes_left = chunksize;
            if (i ==  NUM_SERVERS){
                bytes_left += filesize %  NUM_SERVERS;
            }
            while(bytes_left){
                if (bytes_left < 1028){
                    fread(buf, sizeof(char), bytes_left, source_fd);
                    fwrite(buf, sizeof(char), bytes_left, chunk_fd);
                    bytes_left = 0;
                }
                else{
                    fread(buf, sizeof(char), 1028, source_fd);
                    fwrite(buf, sizeof(char), 1028, chunk_fd);
                    bytes_left -= 1028;
                }
            }
            fclose(chunk_fd);
        }
        fclose(source_fd);

        //ec: encrypt chunks with simple encryption (xor password for now)
        for (int i = 0; i < NUM_SERVERS; i++){
            //figure out partition strategy
            partition = i - hash;
            if (partition < 0) {
                partition += NUM_SERVERS;
            }
            if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                perror("socket failed");
            }
            //todo: make connection timeout after 1 second
            printf("%d\n", i);

            if (connect(sock_fd, (struct sockaddr *) server_addrs[i], sizeof(*server_addrs[i])) < 0){
                perror("connect failed");
            }
            else{
                //send initial request with credentials
                bytes = snprintf(buf, 1028, "%s %s put %s", username, password, filename); //practice: look into if this is the right way
                if (send(sock_fd, buf, bytes, 0) < 0){
                    perror("send failed");
                };
                //receive response
                if ((bytes = recv(sock_fd, buf, 1028, 0)) < 0){
                    perror("error on recv initial response");
                }
                //check that credentials where correct
                if (!strncmp(buf, "ok", 2)){
                    //todo: send initial request, get ok back, send appropriate chunks according to partition strategy
                    //send first chunk
                    //send second chunk
                }
                else if (!strcmp(buf, "Invalid Username/Password. Please try again.")){
                    printf("Invalid Username/Password. Please try again.\n"); //todo: only print this once and exit
                }
                else{
                    printf("Something is wrong with server %d\n", i + 1);
                }
            }                
            close(sock_fd);
        }
        //todo: delete local files
    }
}
//practice: write put request multi thread routine (one thread for each server)

void get(char * filename, char * username, char * password, struct sockaddr_in * server_addrs[NUM_SERVERS]){
    FILE * chunk_fd;
    FILE * dest_fd;
    int bytes;
    char buf[1028];
    char chunkname[43];

    dest_fd = fopen(filename, "w");
    strcpy(chunkname+1,filename);
    chunkname[0] = '.';
    chunkname[strlen(filename) + 1] = '.';
    chunkname[strlen(filename) + 3] = '\0';
    for (int i = 1; i <=  NUM_SERVERS; i++){
        chunkname[strlen(filename) + 2] = i + '0';
        printf("%s\n",chunkname);
        chunk_fd = fopen(chunkname, "r");
        if (chunk_fd == 0){
            printf("here\n");
        }
        printf("hnnere\n");
        while((bytes = fread(buf, sizeof(char), 1028, chunk_fd))){
            printf("%d\n", bytes);
            fwrite(buf, sizeof(char), bytes, dest_fd);
        }
        fclose(chunk_fd);
    }
    fclose(dest_fd);
}
    //todo: write get request main thread
         //todo: for each server, intiate connection, get ok back
            //ec: check if the chunks are still needed, request if they are
            //todo: save server chunks locally
        //todo: reassemble chunks into main file ec: decrypt chunks
        //todo: delete local files

    //practice: write get request multi thread routine

//todo: write list request
    //todo: write list request main thread
        //todo: for each server, initiate connection
            //todo: traverse or add to e a linked list that stores file names, and mark a byte for each chunk
        //todo: read through linked list to print available files
    //practice: write list request multi thread routine

int main(int argc, char ** argv){
    struct sockaddr_in * server_addrs[NUM_SERVERS];
    struct sockaddr_in server_addr1;
    struct sockaddr_in server_addr2;
    struct sockaddr_in server_addr3;
    struct sockaddr_in server_addr4;
    server_addrs[0] = &server_addr1;
    server_addrs[1] = &server_addr2;
    server_addrs[2] = &server_addr3;
    server_addrs[3] = &server_addr4;

    char username[45];
    char password[45];
    FILE * config_fd;
    char line[128]; //practice: revisit size limitations for everything
    char ip[20];
    char port[8];
    struct hostent *server;
    int portno;

    if (argc != 2){
        printf("Usage: dfc <config>");
        exit(1);
    }
    config_fd = fopen(argv[1], "r");
    //parse file and build addresses
    for (int i = 0; i < NUM_SERVERS; i++){ //practice: recognize which lines are servers and which are username/password
        fgets(line, 128, config_fd);
        sscanf(line, "%*s %*s %s", line);
        sscanf(line, "%[^':']:%s", ip, port);
        sscanf(line, "%[^':']:%*s", ip);
        printf("ip%s port%s\n", ip, port);
        
        bzero((char *) server_addrs[i], sizeof(*server_addrs[i]));
        server_addrs[i]->sin_family = AF_INET;
        server = gethostbyname("127.0.0.1"); //todo: fix this
        bcopy((char *)server->h_addr, (char *)&(server_addrs[i]->sin_addr.s_addr), server->h_length);
        portno = atoi(port);
        server_addrs[i]->sin_port = htons(portno);
    }

    fgets(line, 128, config_fd); //practice: handle invalid config files
    sscanf(line, "%*s %s", username);
    fgets(line, 128, config_fd); //practice: handle invalid config files
    sscanf(line, "%*s %s", password);
    fclose(config_fd);
    //todo: read dfc.config
        //todo: store server information / build address structs
        //todo: store username / password info or request it if it doesn't exist

    /* parse input and call corresponding function
        input stores entire line
        command stores the first word
        filename stores the second word
        numargs stores number of words
    */
    char input[45];
    char command[4];
    char filename[40];
    int numargs;

    while(1){
        fgets(input, 45, stdin); //todo: test if input was longer than buffer
        numargs = sscanf(input, "%s %s %s", command, filename, filename);
        if (!strncmp(command, "put", 3)){
            if (numargs == 2){
                put(filename, username, password, server_addrs);
            }
            else if (numargs == 1){
                printf("put requires an additional argument\n");
            }
            else{
                printf("put only takes one argument\n");  
            } 
        }
        else if (!strncmp(command, "get", 3)){
            if (numargs == 2){
                get(filename, username, password, server_addrs);
            }
            else if (numargs == 1){
                printf("get requires an additional argument\n");
            }
            else{
                printf("get only takes one argument\n");  
            }        
        }
        else if (!strncmp(command, "list", 4)){
            if (numargs == 1){
               printf("list request\n");
            }
            else{
                printf("list does not take any arguments\n");
            }
        }
        else{
            printf("%s : unknown command\n", command);
        }
    }
        //todo: call appropriate functions
}