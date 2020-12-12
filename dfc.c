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

//struct to keep track of what chunks of a file exist across all servers
struct filenode{
    char filename[40];
    int chunksreceived[4];
    struct filenode * next;
};

/* search the list for the file, create if it doesn't exist, and mark the appropriate chunk*/
void file_list_add(struct filenode ** node, char * filename, int chunknum){
    int exists = 0;
    while(*node != NULL){
        if (!strcmp((*node)->filename, filename)){
            printf("Marking %d\n", chunknum);
            (*node)->chunksreceived[chunknum - 1] = 1;
            printf("Marked\n");
            exists = 1;
        }
        node = &((*node)->next);
    }
    if (!exists){
        *node = malloc(sizeof(struct filenode));
        printf("Here\n");
        strcpy((*node)->filename, filename);
        (*node)->chunksreceived[0] = 0;
        (*node)->chunksreceived[1] = 0;
        (*node)->chunksreceived[2] = 0;
        (*node)->chunksreceived[3] = 0;
        (*node)->next = NULL;
        printf("Marking %d\n", chunknum);
        (*node)->chunksreceived[chunknum - 1] = 1;
        printf("Marked\n");
    }
}

/*both print and delete the file linked list */
void file_list_consume(struct filenode * node){
    struct filenode * lastnode;
    while (node != NULL){
        if (node->chunksreceived[0] && node->chunksreceived[1] && node->chunksreceived[2] && node->chunksreceived[3]){
            printf("%s\n", node->filename);
        }
        else{
            printf("%s [incomplete]\n", node->filename);
        }
        lastnode = node;
        node = node->next;
        free(lastnode);
    }
}

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
                    //send chunk #'s
                    bytes = snprintf(buf, 1028, "%c%c",  partitions[partition][0] + '0', partitions[partition][1] + '0'); 
                    if (send(sock_fd, buf, bytes, 0) < 0){
                        perror("send failed for chunk #s");
                    }
                    //build chunk 1 name
                    chunkname[strlen(filename) + 2] = partitions[partition][0] + '0';
                    chunk_fd = fopen(chunkname, "r");
                    //receive confirmation
                    if (recv(sock_fd, buf, 1028, 0) < 0){
                        perror("recv failed getting request for chunk 1");
                    }
                    //send content
                    buf[0] = 'c';
                    while ((bytes = fread(buf + 1, sizeof(char), 1027, chunk_fd)) > 0){
                        if (bytes < 1027){
                            buf[0] = 'd'; //if this is the last transmission, let the server know
                        }
                        if (send(sock_fd, buf, bytes+1, 0) < 0){
                            perror("send failed sending chunk 1");
                        }
                    }
                    if (bytes == 1027){
                        //incase the last transmission perfectly filled the buffer, send the end signal
                        buf[0] = 'd';
                        if (send(sock_fd, buf, 1, 0) < 0){
                            perror("send failed on end of chunk 1");
                        }
                    }
                    fclose(chunk_fd);
                    //build chunk 2 name
                    chunkname[strlen(filename) + 2] = partitions[partition][1] + '0';
                    chunk_fd = fopen(chunkname, "r");
                    //receive confirmation
                    if (recv(sock_fd, buf, 1027, 0) < 0){
                        perror("recv failed getting request for chunk 2");
                    }
                    //send content
                    buf[0] = 'c';
                    while ((bytes = fread(buf + 1, sizeof(char), 1027, chunk_fd)) > 0){
                        if (bytes < 1027){
                            buf[0] = 'd'; //if this is the last transmission, let the server know
                        }
                        if (send(sock_fd, buf, bytes+1, 0) < 0){
                            perror("send failed sending chunk 2");
                        }
                    }
                    if (bytes == 1027){
                        //incase the last transmission perfectly filled the buffer, send the end signal
                        buf[0] = 'd';
                        if (send(sock_fd, buf, 1, 0) < 0){
                            perror("send failed on end of chunk 2");
                    }
                    }
                    fclose(chunk_fd);
                }
                else{
                    printf("Invalid Username/Password. Please try again.\n");
                }
            }                
            close(sock_fd);
        }
        //delete local chunk files
        for (int i = 1; i <=  NUM_SERVERS; i++){
            chunkname[strlen(filename) + 2] = i + '0';
            remove(chunkname);
        }
    }
}
//practice: write put request multi thread routine (one thread for each server)

void get(char * filename, char * username, char * password, struct sockaddr_in * server_addrs[NUM_SERVERS]){
    FILE * chunk_fd;
    FILE * dest_fd;
    int bytes, sock_fd;
    int chunksreceived[4] = {0, 0, 0, 0};
    char buf[1028];
    char chunkname[43];
    //build chunkname template
    strcpy(chunkname+1,filename);
    chunkname[0] = '.';
    chunkname[strlen(filename) + 1] = '.';
    chunkname[strlen(filename) + 3] = '\0';
    /*contact each server and download the neccesary chunks*/
    for (int i = 0; i < NUM_SERVERS; i++){
        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("socket failed");
        }
        //todo: make connection timeout after 1 second
        //make initial connection
        printf("Contacting server %d.\n", i);
        if (connect(sock_fd, (struct sockaddr *) server_addrs[i], sizeof(*server_addrs[i])) < 0){
            perror("connect failed");
        }
        else{
            //send initial request with authentication credentials
            bytes = snprintf(buf, 1028, "%s %s get %s", username, password, filename); //practice: look into if this is the right way, make sure this is safe
            if (send(sock_fd, buf, bytes, 0) < 0){
                perror("send failed");
            };
            //receive authentication response
            if ((bytes = recv(sock_fd, buf, 1028, 0)) < 0){
                perror("error on recv initial response");
            }
            printf("Got initial response %s.\n", buf);
            //check that credentials where correct
            if (!strncmp(buf, "ok", 2)){
                //send ready byte
                if (send(sock_fd, buf, 1, 0) < 0){
                    perror("send failed on ready byte");
                }
                printf("Sent ready byte %d.\n", buf[0]);
                while(1){
                     //get chunk #
                    if (recv(sock_fd, buf, 1027, 0) < 0){
                        perror("recv failed getting chunk #");
                    }
                    printf("Got back byte %c.\n", buf[0]);
                    //if instead of a chunk number there is a 'd', there are no more chunks on the server
                    if (buf[0] == 'd'){
                        break;
                    }
                    else{
                        //build chunkname
                        chunkname[strlen(filename) + 2] = buf[0];
                        //try if chunk is already present
                        chunk_fd = fopen(chunkname, "r");
                        if (chunk_fd != NULL){
                            //if chunk is already here, let server know
                            fclose(chunk_fd);
                            buf[0] = 'd';
                            if (send(sock_fd, buf, 1, 0) < 0){
                                perror("send failed rejecting chunk");
                            }
                        }
                        else{
                            chunksreceived[atoi(buf) - 1] = 1; //todo: test
                            //if the chunk is not already saved, request it
                            buf[0] = 'c';
                            if (send(sock_fd, buf, 1, 0) < 0){
                                perror("send failed requesting chunk");
                            }
                            chunk_fd = fopen(chunkname, "w");
                            //download the chunk
                            bzero(buf, 1028);
                            while((bytes = recv(sock_fd, buf, 1028, 0)) > 0){
                                printf("%s\n", buf);
                                //use first byte as transmission over message
                                fwrite(buf + 1, sizeof(char), bytes - 1, chunk_fd);
                                if (buf[0] == 'd'){
                                    break;
                                }
                                bzero(buf, 1028);
                            }
                            buf[0] = 'd';
                            if (send(sock_fd, buf, 1, 0) < 0){
                                perror("send failed on finished byte");
                            }
                            fclose(chunk_fd);
                        }
                    }
                }         
            }
            else{
                printf("Invalid Username/Password. Please try again.\n");
            }
        }                
        close(sock_fd);
    }
    if (chunksreceived[0] && chunksreceived[1] && chunksreceived[2] && chunksreceived[3]){
        //reassemble chunks (decrypt here)
        dest_fd = fopen(filename, "w");
        for (int i = 1; i <=  NUM_SERVERS; i++){
            chunkname[strlen(filename) + 2] = i + '0';
            printf("%s\n",chunkname);
            chunk_fd = fopen(chunkname, "r");
            if (chunk_fd == NULL){
                printf("File Incomplete\n");
            }
            printf("hnnere\n");
            while((bytes = fread(buf, sizeof(char), 1028, chunk_fd))){
                printf("%d\n", bytes);
                fwrite(buf, sizeof(char), bytes, dest_fd);
            }
            fclose(chunk_fd);
            remove(chunkname); //todo: test
        }
        fclose(dest_fd);
    }
    else{
        printf("File incomplete\n");
        for (int i = 1; i <=  NUM_SERVERS; i++){
            if (chunksreceived[i - 1]){
                chunkname[strlen(filename) + 2] = i + '0';
                remove(chunkname); //todo: test
            }
        }
    }
    
}

//todo: write list request
void list (char * username, char * password, struct sockaddr_in * server_addrs[NUM_SERVERS]){
    int sock_fd, bytes;
    char buf[1028];
    int chunknum;
    struct filenode * filelist;

    filelist = NULL;
    //initialize linked list
    for (int i = 0; i < NUM_SERVERS; i++){
        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("socket failed");
        }
        //todo: make connection timeout after 1 second
        //make initial connection
        if (connect(sock_fd, (struct sockaddr *) server_addrs[i], sizeof(*server_addrs[i])) < 0){
            perror("connect failed");
        }
        else{
            //send initial request with authentication credentials
            bytes = snprintf(buf, 1028, "%s %s list", username, password); //practice: look into if this is the right way, make sure this is safe
            if (send(sock_fd, buf, bytes, 0) < 0){
                perror("send failed");
            };
            //receive authentication response
            if ((bytes = recv(sock_fd, buf, 1028, 0)) < 0){
                perror("error on recv initial response");
            }
            //check that credentials where correct
            if (!strncmp(buf, "ok", 2)){
                //send ready byte
                if (send(sock_fd, buf, 1, 0) < 0){
                    perror("send failed on ready byte");
                }
                //get chunk details line by line
                while(1){
                    if ((bytes = recv(sock_fd, buf, 1028, 0)) < 0){
                        perror("recv failed getting chunk info");
                    }
                    //check if the transmission is done
                    if (buf[0] == 'd'){
                        break;
                    }
                    buf[bytes - 2] = '\0';
                    buf[bytes] = '\0';
                    sscanf(buf + bytes - 1, "%d", &chunknum);
                    printf("here\n");
                    printf("%s\n", buf);
                    file_list_add(&filelist, buf + 1, chunknum);
                    if (send(sock_fd, buf, 1, 0) < 0){
                        perror("send failed on confirmation byte");
                    }
                }   
            }
            else{
                printf("Invalid Username/Password. Please try again.\n");
            }
        }
        //todo: consume linked list                
        close(sock_fd);
    }
    file_list_consume(filelist);
}
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
        printf("ip:%s port:%s\n", ip, port);
        
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
               list(username, password, server_addrs);
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