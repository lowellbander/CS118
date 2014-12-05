//packet struct

#define PACKET_SIZE 1024
#define PAYLOAD_SIZE (PACKET_SIZE - 2*sizeof(long))

typedef struct{
    unsigned long seqnum;
    unsigned long total_size; 
    char payload[PAYLOAD_SIZE];
} packet;

