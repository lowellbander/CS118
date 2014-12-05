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

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    int len = sizeof(struct sockaddr_in);
    char buffer[PACKET_SIZE];
    struct hostent *host;
    int message_length, sock, port;

    if (argc < 6) {
        fprintf(stderr, "Usage: %s <host> <port> <filename> <p_loss> <p_corrupt>\n", argv[0]);
        return 1;
    }

    float p_loss = atof(argv[4]);
    if (p_loss < 0 || p_loss > 1) {
        printf("p_loss out of bounds. Must be 0 <= p_loss <= 1\n");
        return 1;
    }

    float p_corrupt = atof(argv[5]);
    if (p_corrupt < 0 || p_corrupt > 1) {
        printf("p_corrupt out of bounds. Must be 0 <= p_corrupt <= 1\n");
        return 1;
    }

    /*  Intializes random number generator */
    srand(time(NULL));

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

    // this will store all the packets that we receive
    packet* received_packets = NULL;
    unsigned long expected_sequence_number = 0;
    unsigned total_size;
    int nReceivedPackets = 0;

    while (true) {
        // wait until there is a packet in the buffer
        if ((message_length = recvfrom(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *) &server, &len)) != -1) {

            packet* packet_pointer = (packet*) buffer;
            //printf("just received packet:\n");
            //print_packet(packet_pointer);
            
            // initialize received_packets if we have not yet done so.
            if (received_packets == NULL) {
                total_size = packet_pointer->total_size;
                int nPackets = ((packet_pointer->total_size)/PAYLOAD_SIZE)*sizeof(packet);
                received_packets = (packet*)calloc(nPackets, sizeof(packet));
                if (received_packets != NULL)
                    printf("succesfully initialized received_packets.\n");
                else 
                    printf("failed to intitialize received_packets.\n");
            }

            //TODO: ignore dropped and corrupted packets
            if (use_packet(p_loss)) {
                printf("packet with seqnum %lu was lost\n", packet_pointer->seqnum);
            }
            else if (use_packet(p_corrupt)) {
                printf("packet with seqnum %lu was corrupt\n", packet_pointer->seqnum);
            }
            // add the received packet to received_packets if appropriate
            else if (packet_pointer->seqnum == expected_sequence_number) {
                expected_sequence_number += strlen(packet_pointer->payload);
                printf("expected_sequence_number was %lu and is now %lu\n", 
                        packet_pointer->seqnum, expected_sequence_number);
                received_packets[nReceivedPackets] = *packet_pointer;
                printf("received_packets[%i]:\n", nReceivedPackets);
                //print_packet(&received_packets[nReceivedPackets]);
                ++nReceivedPackets;
            }
            else 
                printf("was expecting seqnum of %lu but got %lu\n", expected_sequence_number, packet_pointer->seqnum);

            //construct an ACK and send it to the server
            packet ACK;
            ACK.seqnum = expected_sequence_number;
            ACK.total_size = packet_pointer->total_size;
            if (sendto(sock, (char*)&ACK, sizeof(packet), 0, (struct sockaddr*) &server, len) != -1)
                printf("succesfully sent ACK with seqnum %lu\n", ACK.seqnum);
            else 
                printf("failed to send ACK with seqnum %lu\n", ACK.seqnum);

            //if we just received the last packet, we're done
            if (expected_sequence_number == packet_pointer->total_size)
                break;
        }
    }
    printf("\nDone. Received %i packets.\n", nReceivedPackets);

    // print received_packets;
    int i;
    for (i = 0; i < nReceivedPackets; ++i)
        print_packet(&received_packets[i]);

    // write the received file to disk
    char file_contents[total_size + 1];
    // copy contents from packets to our temporary buffer
    for (i = 0; i < nReceivedPackets; ++i)
        strcat(file_contents, received_packets[i].payload);
    FILE* f = fopen("received.txt", "w");
    if (f == NULL)
        printf("failed to open file\n");
    fwrite(file_contents, sizeof(char), total_size, f);
    fclose(f);

    close(sock);
    return 0;
}
