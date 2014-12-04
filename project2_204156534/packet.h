//packet struct

#DEFINE PAYLOAD_SIZE 800
typedef enum {DATA, ACK, END} packetType;

typedef struct{
    packetType type;
    char seqnum[16];
    char payload[PAYLOAD_SIZE];
} packet;

