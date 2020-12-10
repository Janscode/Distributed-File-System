#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/md5.h>
#define NUM_SERVERS 4 //practice: make this work for variable number of servers
//todo: import standard io, networking, (maybe) multithreading, and hashing

//todo: write put request
void put(char * filename, char * username, char * password, struct sockaddr_in server_addrs[NUM_SERVERS]){
    FILE * source_fd;
    FILE * chunk_fd;
    unsigned char digest[MD5_DIGEST_LENGTH];
    char chunkname[43];
    char buf[1028];
    MD5_CTX mdcontext;
    int bytes, bytes_left, filesize, chunksize, hash;
    
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
        strcpy(chunkname+1,filename);
        chunkname[0] = '.';
        chunkname[strlen(filename) + 1] = '.';
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
        //todo: compute md5 hash, figure out partition strategy
        //ec: encrypt chunks with simple encryption (xor password for now)
        //todo: for each server, intiate connection, get ok back, send appropriate chunks according to partition strategy
        //todo: delete local files
        fclose(source_fd);
    }
}
//practice: write put request multi thread routine (one thread for each server)

//todo: write get request
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
    struct sockaddr_in server_addrs[NUM_SERVERS];
    char username[45];
    char password[45];
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
                printf("get request for %s\n", filename);
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