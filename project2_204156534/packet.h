//packet struct

#define PACKET_SIZE 1024
#define PAYLOAD_SIZE (PACKET_SIZE - 2*sizeof(long))

typedef struct{
    unsigned long seqnum;
    unsigned long total_size; 
    char payload[PAYLOAD_SIZE];
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
    printf("Payload: %s\n", packet_pointer->payload);            
    printf("strlen(payload): %i\n", strlen(packet_pointer->payload));
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
}
