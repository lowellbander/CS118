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
int seqnum_to_packetnum(long seqnum){
    return seqnum/PAYLOAD_SIZE-1;
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
    char buffer[PACKET_SIZE];
    struct sockaddr_in self, other;
    int len = sizeof(struct sockaddr_in);
    int message_length, sock, port;
    unsigned long number_of_packets = 0;
    float packet_loss = 0;
    float packet_corruption = 0;

    if (argc != 4) {
        error_and_exit("Usage: ./server <port> <packet loss> <packet corruption>\nPacket loss and packet corruption values must be from 0.0 - 1.0\n");
    }

    //use args
    port = atoi(argv[1]);
    packet_loss =  atof(argv[2]);
    packet_corruption = atof(argv[3]);
    if(packet_loss < 0.0 || packet_loss > 1.0)
       error_and_exit("Invalid value for packet_loss.\n");
    if(packet_corruption < 0 || packet_corruption > 1)
       error_and_exit("Invalid value for packet_corruption.\n");

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

    memset(buffer, 0, PACKET_SIZE);
    while(1){
        if ((message_length = recvfrom(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *) &other, &len)) != -1) {
            
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
            long highest_ACK_received = 0;
            int highest_packet_sent = -1;
            int highest_ACKed_pkt = -1;
            unsigned window_size = 5;
            unsigned window_base = 0;
            time_t timeout;
    
            //transmission loop
            int j = window_base;
            while(highest_ACK_received != file_size){
                printf("\nCurrent window base: %i\n",window_base);
                for(j=window_base; j<window_base+window_size && j<number_of_packets; ++j){
                    printf("j: %i and highest_packet_sent: %i and highest_ACKed_pkt: %i\n", j, highest_packet_sent, seqnum_to_packetnum(highest_ACK_received));
                    if(j > seqnum_to_packetnum(highest_ACK_received)){
                        printf("Sending packet %i with sequence number: %lu\n", j, packets_to_send[j].seqnum);
                        sendto(sock, (char*)&packets_to_send[j], PACKET_SIZE, 0, (struct sockaddr*)&other, len);
                        highest_packet_sent = j;
                        //TODO: Start timer righta after first packet is sent.
                        if(j == window_base){
                              timeout = time(0) + TTL; 
                              printf("Timeout reseti\n"); 
                        }
                    }
                }
                           
                    
                //last ack for current window
                long window_last_seqnum = -1;
                        if(window_base+window_size-1 > number_of_packets-1)
                            window_last_seqnum = packets_to_send[number_of_packets-1].seqnum;
                        else
                            window_last_seqnum = packets_to_send[window_base+window_size-1].seqnum;
                
                
                const int FLAGS = MSG_DONTWAIT;
                
                //handle ACKs
                while(1){
                    time_t now = time(0);
                    if(difftime(timeout, now) <= 0){
                        printf("TIMEOUT\n");
                       return;
                        break;
                    }

                    message_length = recvfrom(sock, buffer, PACKET_SIZE, FLAGS, (struct sockaddr*)&other, &len);
                    if(message_length == -1){
                        continue;
                    }
        
                    packet *ACK_ptr = (packet *)buffer;
                    if(ACK_ptr == NULL)
                        error_and_exit("ACK buffer null\n");
                    printf("ACK packet received with sequence num: %lu\n",ACK_ptr->seqnum); 

                    if(use_packet(packet_corruption))
                    {
                        printf("ACK was corrupted\n");                            
                    }   
                    else if(use_packet(packet_loss)){
                        printf("ACK was lost\n");
                    }
                    else{
                            
                        if(ACK_ptr->seqnum > packets_to_send[window_base].seqnum){
                            highest_ACK_received = ACK_ptr->seqnum;
                            
                            int k = window_base;
                            for(;k<window_base+window_size && k<number_of_packets; ++k){
                                if(packets_to_send[k].seqnum == ACK_ptr->seqnum)
                                    break;
                            }
                            window_base = k;
                            printf("New window base: %i\n",window_base);
                            
                            int window_end = window_base+window_size;             
                            if(window_end>highest_packet_sent)
                            {
                                int l=highest_packet_sent+1;
                                for(;l<window_end && l<number_of_packets;l++){
                                    printf("Window shifted. Sending packet %i with sequence number: %lu\n\n",l, packets_to_send[l].seqnum);
                                    sendto(sock, (char*)&packets_to_send[l],PACKET_SIZE,0,(struct sockaddr*)&other,len);
                                    highest_packet_sent = l;           
                                    timeout = time(0) + TTL;
                                }
                            }
                        }
                        else{
                            printf("ACK repeated: %lu\n",ACK_ptr->seqnum);
                        }
                        if(highest_ACK_received == file_size){
                            printf("Final ACK received.\n");
                            break;
                        }
                        if(highest_ACK_received == window_last_seqnum){
                            //printf("Last ack of current window received\n");
                            break;
                        }
                    }  
                } //End ACK handler loop
                printf("highest_ACK_received: %lu\n", highest_ACK_received);
            
            } //End transmission loop
            printf("Transmission over.\n");            
        }    
    }
    close(sock);
    return 0;
}


