#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include "packet.h"

#define BUF_SIZE 2048

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    int len = sizeof(struct sockaddr_in);
    char buffer[BUF_SIZE];
    struct hostent *host;
    int message_length, sock, port;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <host> <port> <filename>\n", argv[0]);
        return 1;
    }

    host = gethostbyname(argv[1]);
    if (host == NULL) {
        perror("gethostbyname");
        return 1;
    }

    port = atoi(argv[2]);

    /* initialize socket */
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
        return 1;
    }

    /* initialize server addr */
    memset((char *) &server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr = *((struct in_addr*) host->h_addr);

    char* filename = argv[3];

    /* send filename */
    if (sendto(sock, filename, strlen(filename), 0, (struct sockaddr *) &server, len) == -1) {
        perror("sendto()");
        return 1;
    }
    printf("requesting file: '%s'\n", filename);

    // TODO: delete this deprecated code
    /* receive echo.
    ** for single message, "while" is not necessary. But it allows the client 
    ** to stay in listen mode and thus function as a "server" - allowing it to 
    ** receive message sent from any endpoint.
    */
    // while ((message_length = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &server, &len)) != -1) {
    //     printf("Received from %s:%d: \n",  inet_ntoa(server.sin_addr), ntohs(server.sin_port)); 
    //     fflush(stdout);
    //     write(1, buffer, message_length);
    //     write(1, "\n", 1);
    // }

    // TODO: initialize linked list of packets

    packet* received_packets = NULL;

    while (true) {
        // wait until there is a packet in the buffer
        if ((message_length = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &server, &len)) != -1) {
            printf("message_length: %i\n", message_length);

            packet* packet_pointer = (packet*) buffer;
            printf("Seq num: %lu\n", packet_pointer->seqnum);
            printf("Total size: %lu\n", packet_pointer->total_size);
            printf("Payload: %s\n", packet_pointer->payload);            
            
            // TODO: initialize received_packets if we have not yet done so.
            if (received_packets == NULL) {
                int nPackets = ((packet_pointer->total_size)/PAYLOAD_SIZE)*sizeof(packet);
                received_packets = (packet*)calloc(nPackets, sizeof(packet));
                if (received_packets != NULL)
                    printf("succesfully initialized received_packets.\n");
                else 
                    printf("failed to intitialize received_packets.\n");
            }

            //TODO: add the received packet to received_packets if appropriate

            //if (packet->)
        }
    }

    close(sock);
    return 0;
}
