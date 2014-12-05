#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
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
            
            //file name from buffer and then get contents
            int filenamelength = strlen(buffer);
            char file_name[filenamelength+1];
            file_name[filenamelength] = '\0';
    
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
                //printf("Contructing packet %i\n",i); 
                if(i == number_of_packets-1 && dangling_bytes){
                    //Special case for handling last packet. 
                    strncpy(packets_to_send[i].payload, requested_file+i*PAYLOAD_SIZE, dangling_bytes);
                    packets_to_send[i].payload[dangling_bytes] = 0;
                }
                else{
                    strncpy(packets_to_send[i].payload, requested_file+i*PAYLOAD_SIZE, PAYLOAD_SIZE);
                    packets_to_send[i].payload[PAYLOAD_SIZE] = 0;
                }
                packets_to_send[i].seqnum = i*PAYLOAD_SIZE;         
                packets_to_send[i].total_size = file_size;
            } //end construction loop
            printf("Packet construction finished.\n"); 

            unsigned int TTL = 5; //time to wait for ACK
            unsigned long highest_ACK_received = -1;
            unsigned window_size = 5;
            unsigned window_base = 0;
    
            /*time_t current_time, ;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            printf("Current time: %s",asctime(timeinfo));
            rawtime+=TTL;
            timeinfo = localtime(&rawtime);
            printf("Timeout at: %s", asctime(timeinfo));*/

            //transmission loop
            int test_var = 1;
            while(test_var){
            //while(highest_ACK_received != file_size){
                unsigned j = window_base;
                for(; j<window_base+window_size && j<number_of_packets; ++j){
                    printf("Sending packet with sequence number: %lu\n", packets_to_send[j].seqnum);
                    sendto(sock, (char*)&packets_to_send[j], PACKET_SIZE, 0, (struct sockaddr*)&other, len);
                    //TODO: Start timer righta after first packet is sent.
                }           


                while(((message_length = recvfrom(sock, buffer, 
                        BUF_SIZE, 0, (struct sockaddr *) &other, &len)) != -1)){
                    
                    packet *ACK_ptr = (packet *)buffer;
                    if(ACK_ptr == NULL)
                        error_and_exit("ACK buffer null\n");
                    printf("ACK packet received with sequence num: %lu\n",ACK_ptr->seqnum); 
                    //break; 
                }
                
                //time_t current_time, timeout;
                //struct tm *timeinfo;
                //time(&current_time);
                //timeout = current_time + TTL;
                //while((difftime(timeout,current_time) > 0) || ((message_length = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &other, &len)) != -1)){
                //    
                //    packet *ACK_ptr = (packet *)buffer;
                //    if(ACK_ptr == NULL)
                //        error_and_exit("ACK buffer null\n");
                //    printf("ACK packet received with sequence num: %lu\n",ACK_ptr->seqnum); 
                //    break; 
                //}
                //End ACK loop
                test_var = 0;
            } //End transmission loop
            printf("Transmission over.\n");            
        }    
    }
    close(sock);
    return 0;
}


