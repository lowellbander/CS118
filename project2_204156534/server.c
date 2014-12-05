#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "packet.h"

#define BUF_SIZE 1024
#define PACKET_SIZE 1024

char* readfile(char* filename) {
    printf("In readFile()\n"); 
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        perror("failed to open file\n");
        return NULL;
    }

    // get the number of bytes
    fseek(f, 0L, SEEK_END);
    long nBytes = ftell(f);
    fseek(f, 0L, SEEK_SET); // move cursor back to beginning

    char* contents = (char*) calloc(nBytes, sizeof(char));
    if (contents == NULL) {
        perror("failed to allocate buffer\n");
        return contents;
    }

    fread(contents, sizeof(char), nBytes, f);
    fclose(f);
    return contents;
}

void error_and_exit(char* message){
    if(message != NULL)
        fputs(message, stderr);
    else
        fputs("Null message", stderr);
    exit(1);
}

int main(int argc, char* argv[]) {
    //TODO: Get rid of old vars
    char buffer[BUF_SIZE];
    struct sockaddr_in self, other;
    int len = sizeof(struct sockaddr_in);
    int message_length, sock, port;
    unsigned long number_of_packets = 0;
    float packet_loss = 0;
    float packet_corruption = 0;

    if (argc != 3 && (argc < 2 || argc > 4)) {
        error_and_exit("Usage: ./server <port> [<packet loss> <packet corruption>]\nPacket loss and packet corruption values must be from 0.0 - 1.0\n");
    }

    //use args
    if(argc == 2){
        port = atoi(argv[1]); 
        //OS might reject some port numbers
    }
    else{        
        packet_loss =  atof(argv[2]);
        packet_corruption = atof(argv[3]);
        if(packet_loss < 0.0 || packet_loss > 1.0)
            error_and_exit("Invalid value for packet_loss.\n");
        if(packet_corruption < 0 || packet_corruption > 1)
            error_and_exit("Invalid value for packet_corruption.\n");
    }    
    //timer setup
    //network to host byte order

    /* initialize socket */
    if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
        return 1;
    }

    /* bind to server port */
    memset((char *) &self, 0, sizeof(struct sockaddr_in));
    self.sin_family = AF_INET;
    self.sin_port = htons(port);
    self.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *) &self, sizeof(self)) == -1) {
        perror("bind");
        return 1;
    }

    memset(buffer, 0, BUF_SIZE);
    while(1){
        if ((message_length = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &other, &len)) != -1) {
            
            int filenamelength = strlen(buffer);
            char file_name[filenamelength+1];
            file_name[filenamelength] = '\0';
    
            /*printf("File name length: %i\n", filenamelength);
            printf("file_name before copying from buffer: '%s'\n",file_name);
            printf("Buffer: '%s'\n", buffer);
            printf("strlen(buffer): %i\n", strlen(buffer));*/
            
            strncpy(file_name, &buffer, filenamelength);
            printf("Request received from %s:%d ", inet_ntoa(other.sin_addr), ntohs(other.sin_port)); 
            printf("for file: '%s'\n", file_name);
            char* requested_file = readfile(file_name);
            if(requested_file == NULL)
                error_and_exit("readFile() failed.\n");
    
            //get file size
            struct stat st;
            stat(file_name, &st);
            int file_size = st.st_size;
            printf("%s size: %i\n", file_name, file_size);
    
            //number of packets
            int number_of_packets = file_size / PAYLOAD_SIZE;
            int dangling_bytes = file_size % PAYLOAD_SIZE;
            if(dangling_bytes) number_of_packets++;
            printf("Number of packets: %i\n", number_of_packets);
            
            //construct array of packets to send
            packet packets_to_send[number_of_packets];
            unsigned long i = 0;
            for(; i<number_of_packets; ++i){
                //Handling final packet if not payload is not full
                if(i == number_of_packets-1 && dangling_bytes){
                    strncpy(packets_to_send[i].payload, requested_file+i*PAYLOAD_SIZE, dangling_bytes);
    
                    packets_to_send[i].seqnum = i*PAYLOAD_SIZE + dangling_bytes;         
                    printf("Last seq sent: %i\n", packets_to_send[i].seqnum);
                    printf("Number of packets: %i\n", number_of_packets);
    
                }
                else{
                    strncpy(packets_to_send[i].payload, requested_file+i*PAYLOAD_SIZE, PAYLOAD_SIZE);
                    packets_to_send[i].seqnum = i*PAYLOAD_SIZE;
                }
                packets_to_send[i].total_size = file_size;
                printf("Sending packet %lu\n",i);
                sendto(sock, (char *)&packets_to_send[i], PACKET_SIZE, 0, (struct sockaddr*) &other, len);
             } 
             break;         
            //sendto(sock, requested_file, strlen(requested_file), 0, (struct sockaddr*) &other, len);
        }    
    }

    close(sock);
    return 0;
}
