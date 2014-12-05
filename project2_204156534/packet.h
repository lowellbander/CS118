//packet struct

#define PAYLOAD_SIZE (1024 - 2*sizeof(long))

typedef struct{
    long seqnum;
    long total_size; 
    char payload[PAYLOAD_SIZE];
} packet;

