#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "packet.h"

#define BUF_SIZE 1024
#define PACKET_SIZE 1024

char* readfile(char* filename) {
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

int main(int argc, char* argv[]) {
    char buffer[BUF_SIZE];
    struct sockaddr_in self, other;
    int len = sizeof(struct sockaddr_in);
    int message_length, sock, port;

    if (argc != 2 && argc != 4) {
        fprintf(stderr, "Usage: %s <port> [<packet loss> <packet corruption>]\n", argv[0]);
        exit(1);
    }

    //use args
    
    //timer setup
    //network to host byte order

    /* initialize socket */
    if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
        return 1;
    }

    /* bind to server port */
    port = atoi(argv[1]);
    memset((char *) &self, 0, sizeof(struct sockaddr_in));
    self.sin_family = AF_INET;
    self.sin_port = htons(port);
    self.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *) &self, sizeof(self)) == -1) {
        perror("bind");
        return 1;
    }

    memset(buffer, 0, BUF_SIZE);
    while ((message_length = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &other, &len)) != -1) {
        
        int filenamelength = strlen(buffer);
        char filename[filenamelength];
        memcpy(filename, &buffer, filenamelength);
        printf("Requested file from %s:%d: ", inet_ntoa(other.sin_addr), ntohs(other.sin_port)); 
        printf("'%s'\n", filename);
        //char* requested_file = readfile(filename);
        packet test_packet;
        test_packet.seqnum = 0;
        char* first_message = "lo and behold";
        test_packet.total_size = strlen(first_message);
        strcpy(test_packet.payload, first_message);
        sendto(sock, (char *)&test_packet, PACKET_SIZE, 0, (struct sockaddr*) &other, len);
        //sendto(sock, requested_file, strlen(requested_file), 0, (struct sockaddr*) &other, len);
    }

    close(sock);
    return 0;
}
