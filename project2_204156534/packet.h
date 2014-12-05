//packet struct
#include <stdbool.h>
#include <stdlib.h>

#define PACKET_SIZE 1024
#define NULL_TERMINATOR 1
#define PAYLOAD_SIZE (PACKET_SIZE - 2*sizeof(long) - NULL_TERMINATOR)

typedef struct{
    unsigned long seqnum;
    unsigned long total_size; 
    char payload[PAYLOAD_SIZE + NULL_TERMINATOR];
} packet;

void print_packet(packet* packet_pointer) {
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    if(packet_pointer == NULL){
        printf("Null packet pointer");
        return;
    }
    printf("printing packet information: \n");
    printf("Seq num: %lu\n", packet_pointer->seqnum);
    printf("Total size: %lu\n", packet_pointer->total_size);
    //printf("Payload: %s\n", packet_pointer->payload);            
    printf("strlen(payload): %i\n", strlen(packet_pointer->payload));
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
}

bool use_Packet(float probability){

    int random = rand() % 100;
    if(random > probability*100)
        return true;
    else 
        return false;
}
